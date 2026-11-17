"""Exercise the real channel publication shell with offline gh/date doubles."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[2]
SHA = '0123456789abcdef0123456789abcdef01234567'
STAMP = '2026-01-02T03:04:05Z'


class ReleaseNotesTests(unittest.TestCase):
    def run_channel(self, branch, exists=True, fail_upload=False, fail_list=False, fail_delete=False, missing=False):
        workflow = (ROOT / '.github/workflows/main.yml').read_text()
        step = workflow.split('      - name: Publish channel release\n', 1)[1]
        step = step.split('      - name: Publish tagged release\n', 1)[0]
        script = textwrap.dedent(step.split('        run: |\n', 1)[1])
        channel, title = ('latest', 'Latest') if branch == 'main' else ('develop', 'Develop')
        values = {'release-tag': 'channel/' + channel, 'release-name': title,
                  'basename': 'steamworks-' + channel + '-linux',
                  'prerelease': 'false' if branch == 'main' else 'true'}
        for key, value in values.items():
            script = script.replace('${{ needs.build-exts-linux.outputs.' + key + ' }}', value)
        script = script.replace('${{ needs.build-exts-windows.outputs.basename }}',
                                'steamworks-' + channel + '-windows')
        self.assertNotIn('${{', script)
        with tempfile.TemporaryDirectory(prefix='release notes with spaces ') as temp:
            root = Path(temp)
            tools = root / 'bin'
            tools.mkdir()
            assets = root / 'dist/release'
            assets.mkdir(parents=True)
            if not missing:
                for platform in ('linux', 'windows'):
                    (assets / f'steamworks-{channel}-{platform}.zip').write_bytes(b'fixture')
            gh = tools / 'gh'
            gh.write_text('''#!/usr/bin/env python3
import json,os,sys
from pathlib import Path
args=sys.argv[1:]
with open(os.environ['CALLS'], 'a') as output:
    output.write(json.dumps(args)+'\\n')
if args[:2]==['release','list']:
    if os.environ['FAIL_LIST']=='1': sys.exit(31)
    if os.environ['EXISTS']=='1': print(os.environ['TAG'])
if args[:2]==['release','delete'] and os.environ['FAIL_DELETE']=='1':
    sys.exit(32)
if args[:2]==['release','upload'] and os.environ['FAIL_UPLOAD']=='1':
    sys.exit(23)
if '--notes-file' in args:
    Path(os.environ['NOTES']).write_bytes(Path(args[args.index('--notes-file')+1]).read_bytes())
''')
            date = tools / 'date'
            date.write_text('#!/bin/sh\n[ "$1" = -u ] && [ "$2" = +%Y-%m-%dT%H:%M:%SZ ] || exit 29\nprintf "%s\\n" "' + STAMP + '"\n')
            for tool in (gh, date):
                tool.chmod(0o755)
            env = dict(os.environ, PATH=str(tools) + os.pathsep + os.environ['PATH'],
                       GH_TOKEN='offline-fixture', GITHUB_SHA=SHA,
                       GITHUB_SERVER_URL='https://github.com', GITHUB_REPOSITORY='example/project',
                       GITHUB_RUN_ID='12345', GITHUB_RUN_ATTEMPT='2',
                       CALLS=str(root / 'calls.jsonl'), NOTES=str(root / 'notes.md'), TMPDIR=temp,
                       EXISTS=str(int(exists)), FAIL_UPLOAD=str(int(fail_upload)),
                       FAIL_LIST=str(int(fail_list)), FAIL_DELETE=str(int(fail_delete)),
                       TAG='channel/' + channel)
            result = subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', script],
                                    cwd=temp, env=env, text=True, capture_output=True)
            calls_file = root / 'calls.jsonl'
            calls = [json.loads(line) for line in calls_file.read_text().splitlines()] if calls_file.exists() else []
            notes = (root / 'notes.md').read_text() if (root / 'notes.md').exists() else None
            return result, calls, notes

    def test_latest_and_develop_creation_and_update(self):
        for branch in ('main', 'develop'):
            for exists in (False, True):
                with self.subTest(branch=branch, exists=exists):
                    result, calls, notes = self.run_channel(branch, exists)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    channel = 'latest' if branch == 'main' else 'develop'
                    self.assertTrue(all(call[2] == 'channel/' + channel for call in calls if call[1] != 'list'))
                    self.assertEqual(sum(call[1] == 'create' for call in calls), 1)
                    upload = next(i for i, call in enumerate(calls) if call[1] == 'upload')
                    create = next(i for i, call in enumerate(calls) if call[1] == 'create')
                    edit = next(i for i, call in enumerate(calls) if '--draft=false' in call)
                    self.assertLess(create, upload)
                    self.assertIn('--draft', calls[create])
                    self.assertIn('--verify-tag', calls[create])
                    self.assertIn('--notes-file', calls[create])
                    self.assertEqual(calls[create][calls[create].index('--title') + 1],
                                     'Latest' if branch == 'main' else 'Develop')
                    self.assertIn('--prerelease=false' if branch == 'main' else '--prerelease', calls[edit])
                    self.assertGreater(edit, upload)
                    self.assertEqual(calls[upload][3:5], [f'dist/release/steamworks-{channel}-linux.zip',
                                                         f'dist/release/steamworks-{channel}-windows.zip'])
                    self.assertNotIn('--clobber', calls[upload])
                    self.assertIn('--latest' if branch == 'main' else '--latest=false', calls[edit])
                    self.assertIn(STAMP, notes)
                    self.assertIn(f'[{SHA}](https://github.com/example/project/commit/{SHA})', notes)
                    self.assertIn('https://github.com/example/project/actions/runs/12345/attempts/2', notes)
                    self.assertIn('GC remoto excluido de la aceptación global de L4D2 dedicado; API conservada por compatibilidad, sin recepción remota acreditada.', notes)
                    self.assertNotIn('GC remoto positivo pendiente', notes)
                    self.assertNotIn('Esta publicación no implica despliegue en servidores.', notes)
                    self.assertEqual(sum(call[1] == 'delete' for call in calls), int(exists))
                    if exists:
                        delete = next(i for i, call in enumerate(calls) if call[1] == 'delete')
                        self.assertLess(delete, create)
                        self.assertNotIn('--cleanup-tag', calls[delete])

    def test_failed_upload_keeps_release_unpublished(self):
        for branch in ('main', 'develop'):
            with self.subTest(branch=branch):
                result, calls, notes = self.run_channel(branch, fail_upload=True)
                self.assertEqual(result.returncode, 23)
                self.assertIsNotNone(notes)
                self.assertFalse(any('--draft=false' in call for call in calls))

    def test_preflight_and_delete_failures_stop_publication(self):
        for branch in ('main', 'develop'):
            for option, code in (('fail_list', 31), ('fail_delete', 32), ('missing', 1)):
                with self.subTest(branch=branch, option=option):
                    result, calls, notes = self.run_channel(branch, **{option: True})
                    self.assertEqual(result.returncode, code, result.stderr)
                    self.assertIsNone(notes)
                    self.assertFalse(any(call[1] in ('create', 'upload', 'edit') for call in calls))
                    if option != 'fail_delete':
                        self.assertFalse(any(call[1] == 'delete' for call in calls))


if __name__ == '__main__':
    unittest.main(verbosity=2)
