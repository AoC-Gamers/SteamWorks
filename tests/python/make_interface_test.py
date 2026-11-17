"""Exercise Make entrypoints from another cwd and a source path with spaces."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class MakeInterfaceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='make entrypoint ')
        self.addCleanup(self.temp.cleanup)
        self.caller = Path(self.temp.name)
        self.root = self.caller / 'sources with spaces'
        self.root.mkdir()
        shutil.copy2(ROOT / 'Makefile', self.root / 'Makefile')

    def write(self, relative, text):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def make_env(self, **extra):
        # These are separate source-tree fixtures, not recursive production makes.
        # Do not inherit the invoking target's options or dependency directories.
        env = dict(os.environ)
        for name in ('MAKEFLAGS', 'MFLAGS', 'GNUMAKEFLAGS', 'MAKELEVEL', 'MAKEOVERRIDES',
                     'ROOT_DIR', 'TEST_OUTPUT', 'TEST_OPTIONS', 'LINUX_DEPS_DIR',
                     'WINDOWS_DEPS_DIR', 'SOURCEMOD_DIR', 'MMSOURCE_DIR', 'AMTL_DIR'):
            env.pop(name, None)
        env.update(extra)
        return env

    def make(self, *args, **env):
        return subprocess.run(['make', '--no-print-directory', '-s', '-f',
                               str(self.root / 'Makefile'), *args], cwd=self.caller,
                              env=self.make_env(**env), capture_output=True, text=True)

    def test_native_suite_preserves_paths_flags_and_failure(self):
        self.write('tests/runners/test-public-ip.sh',
                   'printf "%s\\n" "$STEAMWORKS_SDK_DIR" "$TEST_CXXFLAGS"\nexit "${FIXTURE_EXIT:-0}"\n')
        args = ('test-public-ip', 'STEAMWORKS_SDK_DIR=/SDK with spaces', 'TEST_CXXFLAGS=-m32 -O1')
        good = self.make(*args)
        self.assertEqual(good.returncode, 0, good.stderr)
        self.assertEqual(good.stdout.splitlines(), ['/SDK with spaces', '-m32 -O1'])
        failed = self.make(*args, FIXTURE_EXIT='17')
        self.assertNotEqual(failed.returncode, 0)
        self.assertIn('17', failed.stderr)

    def test_integration_options_and_required_output(self):
        self.write('tests/runners/test-integration.py',
                   'import json,sys\nprint(json.dumps(sys.argv[1:]))\n')
        missing = self.make('test')
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn('Set TEST_OUTPUT', missing.stderr)
        output = str(self.caller / 'new results')
        result = self.make('test', 'PYTHON=' + sys.executable, 'TEST_OUTPUT=' + output,
                           'TEST_OPTIONS=--with-auth-binding --with-auth-forward')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout),
                         ['--output', output, '--with-auth-binding', '--with-auth-forward'])

    def test_build_keeps_backend_dependency_defaults(self):
        self.write('scripts/build/build-linux-l4d2.sh',
                   'printf "%s\\n" "$DEPS_DIR" "${SOURCEMOD_DIR:-backend-default}"\n')
        env = self.make_env()
        result = subprocess.run(['make', '--no-print-directory', '-s', '-f',
                                 str(self.root / 'Makefile'), 'build-exts-linux'],
                                cwd=self.caller, env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.splitlines(),
                         [str(self.root / '.deps/exts-linux'), 'backend-default'])

    def test_platform_validation_uses_selected_artifact_tree(self):
        self.write('scripts/ci/ci-validate-artifact.sh',
                   'printf "%s\\n" "$EXTS_PLATFORM" "$EXTS_ARTIFACT_DIR"\n')
        for platform in ('linux', 'windows'):
            with self.subTest(platform=platform):
                directory = str(self.caller / (platform + ' payload'))
                result = self.make('validate-artifact-' + platform,
                                   platform.upper() + '_ARTIFACT_DIR=' + directory)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.splitlines(), [platform, directory])


if __name__ == '__main__':
    unittest.main(verbosity=2)
