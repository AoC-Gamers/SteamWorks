"""Exercise Make/script orchestration without a Sonar server or real credentials."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("make_sonar", ROOT / "scripts/tools/make_sonar.py")
sonar = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sonar)


class SonarToolingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="sonar tooling ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "sources with spaces"
        self.root.mkdir()
        for relative in ("Makefile", "scripts/tools/make_sonar.py", "sonar-project.properties"):
            dest = self.root / relative
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / relative, dest)
        (self.root / "extension").mkdir()
        self.source = self.root / "extension/unit.cpp"
        self.source.write_text("int value;\n")
        self.db = self.root / "build/compile_commands.json"
        self.db.parent.mkdir()
        self.entry = {"directory": str(self.root), "file": str(self.source),
                      "arguments": ["g++", "-m32", "-DTEST=1", "-c", str(self.source)]}
        self.db.write_text(json.dumps([self.entry]))
        (self.root / ".env").write_text('SONAR_HOST_URL=https://sonar.example.invalid\nSONAR_PROJECT_KEY=SteamWorks-test\n')
        (self.root / "env.secret").write_text('SONAR_TOKEN="test-only-token"\n')
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.trace = self.root / "calls.jsonl"
        for tool in ("cppcheck", "sonar-scanner"):
            script = self.bin / tool
            script.write_text(f'''#!{sys.executable}
import json,os,sys
tool={tool!r}
with open(os.environ['TEST_TRACE'],'a') as f:
 f.write(json.dumps({{'tool':tool,'argv':sys.argv[1:],'cwd':os.getcwd(),'token':os.environ.get('SONAR_TOKEN')}})+'\\n')
if tool=='cppcheck':
 print('bad xml' if os.environ.get('BAD_XML') else '<results version="2"><errors/></results>',file=sys.stderr)
 sys.exit(int(os.environ.get('CPPCHECK_CODE','0')))
print('scanner saw '+os.environ.get('SONAR_TOKEN',''))
sys.exit(int(os.environ.get('SCANNER_CODE','0')))
''')
            script.chmod(0o755)

    def make(self, target="sonar", **extra):
        env = {k: v for k, v in os.environ.items()
               if not k.startswith("SONAR_") and k not in {"MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES"}}
        env.update(PATH=str(self.bin) + os.pathsep + os.defpath, TEST_TRACE=str(self.trace))
        env.update(extra)
        return subprocess.run(["make", "--no-print-directory", "-s", "-f", str(self.root / "Makefile"),
                               target, f"LINUX_BUILD_DIR={self.db.parent}"], cwd=self.temp.name,
                              env=env, capture_output=True, text=True)

    def calls(self):
        return [json.loads(line) for line in self.trace.read_text().splitlines()] if self.trace.exists() else []

    def test_make_scans_in_source_root_and_keeps_token_out_of_argv_and_report(self):
        result = self.make()
        self.assertEqual(result.returncode, 0, result.stderr)
        local, remote = self.calls()
        self.assertEqual([local['tool'], remote['tool']], ['cppcheck', 'sonar-scanner'])
        self.assertIsNone(local['token'])
        self.assertEqual(remote['token'], 'test-only-token')
        self.assertEqual(remote['cwd'], str(self.root))
        self.assertNotIn('test-only-token', result.stdout + result.stderr + ' '.join(remote['argv']))
        self.assertIn('-Dsonar.projectKey=SteamWorks-test', remote['argv'])
        self.assertIn('-Dsonar.qualitygate.wait=true', remote['argv'])
        self.assertIn('--platform=unix32', local['argv'])
        database = Path(next(a.split('=', 1)[1] for a in local['argv'] if a.startswith('--project=')))
        self.assertIn('-DTEST=1', json.loads(database.read_text())[0]['arguments'])
        self.assertNotIn('test-only-token', database.read_text())

    def test_local_cppcheck_needs_no_server_or_token(self):
        (self.root / '.env').unlink()
        (self.root / 'env.secret').unlink()
        result = self.make('sonar-cppcheck')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([c['tool'] for c in self.calls()], ['cppcheck'])

    def test_explicit_environment_overrides_files(self):
        result = self.make(SONAR_PROJECT_KEY='override', SONAR_TOKEN='environment-token')
        self.assertEqual(result.returncode, 0, result.stderr)
        remote = self.calls()[-1]
        self.assertEqual(remote['token'], 'environment-token')
        self.assertIn('-Dsonar.projectKey=override', remote['argv'])
        self.assertNotIn('environment-token', result.stdout + result.stderr)

    def test_environment_files_are_data_not_shell(self):
        with (self.root / '.env').open('a') as f:
            f.write('SONAR_PROJECT_NAME="$(touch injected-marker)"\n')
        result = self.make()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('-Dsonar.projectName=$(touch injected-marker)', self.calls()[-1]['argv'])
        self.assertFalse((self.root / 'injected-marker').exists())

    def test_incomplete_or_unsafe_context_stops_before_tools(self):
        for override in ({'SONAR_TOKEN': ''}, {'SONAR_HOST_URL': 'https://user:secret@example.invalid'},
                         {'SONAR_HOST_URL': 'http://example.invalid'}, {'SONAR_QUALITY_GATE_TIMEOUT': '0'}):
            with self.subTest(override=override):
                result = self.make(**override)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.calls(), [])
                self.assertNotIn('test-only-token', result.stdout + result.stderr)

    def test_nonsecret_file_cannot_supply_token(self):
        with (self.root / '.env').open('a') as f:
            f.write('SONAR_TOKEN=misplaced-secret\n')
        result = self.make()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.calls(), [])
        self.assertNotIn('misplaced-secret', result.stdout + result.stderr)

    def test_x64_or_incomplete_database_rejected_before_tools(self):
        for entries in ([], [{**self.entry, 'arguments': ['g++', '-m64']} ]):
            self.db.write_text(json.dumps(entries))
            result = self.make()
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(self.calls(), [])

    def test_failed_cppcheck_or_invalid_xml_never_runs_scanner(self):
        for override in ({'CPPCHECK_CODE': '2'}, {'BAD_XML': '1'}):
            if self.trace.exists():
                self.trace.unlink()
            result = self.make(**override)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual([c['tool'] for c in self.calls()], ['cppcheck'])
            self.assertFalse(list((self.root / '.build/sonar').glob('*/cppcheck.xml')))

    def test_scanner_failure_propagates_without_exposing_token(self):
        result = self.make(SCANNER_CODE='7')
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn('test-only-token', result.stdout + result.stderr)

    def test_external_compilation_units_are_not_indexed(self):
        entries = [self.entry, {'directory': str(self.root), 'file': '/sdk/vendor.cpp', 'arguments': ['g++']}]
        self.db.write_text(json.dumps(entries))
        result = self.make()
        self.assertEqual(result.returncode, 0, result.stderr)
        local = self.calls()[0]
        path = Path(next(a.split('=', 1)[1] for a in local['argv'] if a.startswith('--project=')))
        self.assertEqual(len(json.loads(path.read_text())), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
