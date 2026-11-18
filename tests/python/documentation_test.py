"""Regression checks for documentation paths and distribution boundaries."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('documentation',ROOT/'scripts/tools/check-documentation.py')
docs=importlib.util.module_from_spec(spec)
spec.loader.exec_module(docs)


class DocumentationTests(unittest.TestCase):
    def test_current_links_structure_and_code_references(self):
        errors=docs.check(ROOT)
        self.assertEqual(errors,[])

    def test_negative_links_anchors_and_generated_result(self):
        with tempfile.TemporaryDirectory(prefix='steamworks documentation ') as temp:
            root=Path(temp)
            # Copy just the code/documentation tree, never SDK, binaries or Git metadata.
            for area in ('docs','tests','scripts','extension','addons'):
                shutil.copytree(ROOT/area,root/area,ignore=shutil.ignore_patterns('node_modules','__pycache__'))
            for name in ('README.md','AGENTS.md','LICENSE'):
                shutil.copy2(ROOT/name,root/name)
            self.assertEqual(docs.check(root),[])
            page=root/'docs/http/README.md'
            page.write_text(page.read_text()+'\n[bad](missing.md)\n[bad](#nonexistent-anchor)\n`extension/missing.cpp`\n`addons/sourcemod/scripting/include/missing.inc`\n')
            (root/'docs/http/results.json').write_text('{}')
            (root/'docs/http/run-report.md').write_text('# P5-R100: execution diary')
            errors=docs.check(root)
            for expected in ('missing.md','broken anchor','missing code reference','Unexpected docs payload','Execution diary','addons/sourcemod/scripting/include/missing.inc'):
                self.assertTrue(any(expected in error for error in errors),expected)

    def test_makefile_root_from_other_cwd_and_path_with_spaces(self):
        with tempfile.TemporaryDirectory(prefix='steamworks Makefile ') as temp:
            root=Path(temp)/'sources with spaces';root.mkdir()
            shutil.copy2(ROOT/'Makefile',root/'Makefile')
            result=subprocess.run(['make','--no-print-directory','-n','-f',str(root/'Makefile'),'package-exts-linux'],
                                  cwd=temp,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            command=shlex.split(result.stdout.strip())
            self.assertEqual(command[1],str(root/'scripts/package/stage-artifact.py'))
            self.assertEqual(command[2],str(root))

    def test_fixture_hashes_and_transitive_sdk_provenance(self):
        fixture=ROOT/'tests/fixtures'
        manifest=json.loads((fixture/'manifest.json').read_text())
        for name,expected in manifest['files'].items():
            self.assertEqual(hashlib.sha256((fixture/name).read_bytes()).hexdigest(),expected,name)
        profile=fixture/'runtime-profiles/linux-x86-candidate.json'
        data=json.loads(profile.read_text())
        self.assertFalse(data['enabled'])
        self.assertTrue((profile.parent/data['sdk']['evidence']).is_file())
        self.assertEqual(manifest['source_revision'],'295e274de754fb65835188f115a2fd4dd917e289')

    def test_anchor_collisions_and_unicode(self):
        self.assertEqual(docs.anchors('# Autenticación\n## A `b`\n## A b\n'),{'autenticación','a-b','a-b-1'})

    def test_no_generated_files_or_diagnostic_payload_in_platform_packages(self):
        with tempfile.TemporaryDirectory(prefix='steamworks package ') as temp:
            temp=Path(temp)
            for platform,suffix in (('linux','so'),('windows','dll')):
                build=temp/platform/'build';artifact=temp/platform/'artifact'
                extension=build/f'addons/sourcemod/extensions/steamworks.ext.2.l4d2.{suffix}'
                extension.parent.mkdir(parents=True)
                elf=bytearray(52);elf[:7]=b'\x7fELF\x01\x01\x01';elf[16:20]=b'\x03\x00\x03\x00'
                extension.write_bytes(elf if platform=='linux' else b'layout fixture, not a compiled DLL')
                include=build/'addons/sourcemod/scripting/include/steamworks.inc'
                include.parent.mkdir(parents=True);shutil.copy2(ROOT/'addons/sourcemod/scripting/include/steamworks.inc',include)
                staged=subprocess.run([sys.executable,str(ROOT/'scripts/package/stage-artifact.py'),str(ROOT),str(build),str(artifact)],capture_output=True,text=True,cwd=temp)
                self.assertEqual(staged.returncode,0,staged.stderr)
                env=os.environ.copy();env.update(EXTS_ARTIFACT_DIR=str(artifact),EXTS_PLATFORM=platform)
                command=['bash',str(ROOT/'scripts/ci/ci-validate-artifact.sh')]
                good=subprocess.run(command,env=env,capture_output=True,text=True,cwd=temp)
                self.assertEqual(good.returncode,0,good.stderr)
                self.assertFalse(list(artifact.rglob('AGENTS.md')))
                for bad in ('docs/http/README.md','docs/http/results.json','docs/http/run-report.md','docs/runtime-compatibility/p5/log.md',
                            'tests/fixtures/input.json','addons/sourcemod/extensions/diagnostic.so',
                            'plugin-package-map.json','steamworks-sdk.lock.json',
                            'scripts/package/package-map.json','scripts/build/steamworks-sdk.lock.json',
                            f'addons/sourcemod/extensions/steamworks.ext.{suffix}',
                            'sdk/public/steam.h','addons/sourcemod/gamedata/profile.json'):
                    path=artifact/bad;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('forbidden fixture')
                    result=subprocess.run(command,env=env,capture_output=True,text=True,cwd=temp)
                    self.assertNotEqual(result.returncode,0,bad)
                    self.assertIn('Unexpected artifact payload',result.stderr,bad)
                    path.unlink()
                    parent=path.parent
                    while parent != artifact and not any(parent.iterdir()):
                        previous=parent.parent;parent.rmdir();parent=previous
                release=subprocess.run([sys.executable,str(ROOT/'scripts/package/package-release.py'),
                                        '--root',str(temp/platform),'--artifact-dir',str(artifact),'--basename','fixture'],capture_output=True,text=True,cwd=temp)
                self.assertEqual(release.returncode,0,release.stderr)
                with zipfile.ZipFile(temp/platform/'dist/release/fixture.zip') as archive:
                    for name in ('README.md', 'LICENSE', 'CHANGELOG.md'):
                        self.assertEqual(archive.read(name), (ROOT / name).read_bytes())
                    self.assertFalse(any(name.startswith('docs/') for name in archive.namelist()))
                    self.assertFalse(any(name.endswith('.json') for name in archive.namelist()))
                    self.assertIn(f'addons/sourcemod/extensions/steamworks.ext.2.l4d2.{suffix}', archive.namelist())
                    self.assertNotIn(f'addons/sourcemod/extensions/steamworks.ext.{suffix}', archive.namelist())
                    self.assertFalse(any('/fixtures/' in name or name.endswith('AGENTS.md') for name in archive.namelist()))
                # Stage must reject a diagnostic object even before CI inspects its output.
                (extension.parent/'diagnostic.so').write_bytes(b'not product')
                bad=subprocess.run([sys.executable,str(ROOT/'scripts/package/stage-artifact.py'),str(ROOT),str(build),str(temp/platform/'bad')],capture_output=True,text=True)
                self.assertNotEqual(bad.returncode,0)
                self.assertIn('Unexpected build payload',bad.stderr)

    def test_review_and_release_reject_injected_documentation_fixture(self):
        spec=importlib.util.spec_from_file_location('review_doc_negative',ROOT/'scripts/package/prepare-review-artifacts.py')
        review=importlib.util.module_from_spec(spec);spec.loader.exec_module(review)
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);candidate=root/'candidate'
            bad=candidate/'docs/http/fixture.json';bad.parent.mkdir(parents=True);bad.write_text('{}')
            with self.assertRaisesRegex(ValueError,'Unexpected candidate payload'):
                review.prepare(candidate,root/'baseline',root/'output')
            result=subprocess.run([sys.executable,str(ROOT/'scripts/package/package-release.py'),
                                   '--root',str(root),'--artifact-dir',str(candidate),'--basename','bad'],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertFalse((root/'dist/release/bad.zip').exists())

    def test_stage_requires_declared_root_documentation(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'source';source.mkdir()
            for name in ('README.md', 'CHANGELOG.md'):
                (source/name).write_text('documentation fixture')
            (source/'LICENSE').touch()
            build=root/'build';(build/'addons').mkdir(parents=True)
            result=subprocess.run([sys.executable,str(ROOT/'scripts/package/stage-artifact.py'),str(source),str(build),str(root/'artifact')],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertIn('Missing or empty source documentation: LICENSE',result.stderr)



if __name__=='__main__':
    unittest.main(verbosity=2)
