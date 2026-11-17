"""Packaging and rollback contract tests; no runtime or deployment."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("review", ROOT / "scripts/package/prepare-review-artifacts.py")
review = importlib.util.module_from_spec(spec)
spec.loader.exec_module(review)


class ReviewArtifactsTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.candidate, self.baseline = self.root / "candidate", self.root / "baseline"
        self.output = self.root / "output"
        self.elf = bytearray(52)
        self.elf[:7] = b"\x7fELF\x01\x01\x01"
        self.elf[16:20] = b"\x03\x00\x03\x00"
        for name in ("LICENSE", "README.md", "CHANGELOG.md"):
            self.write(self.candidate / name, b"public fixture")
        for base, prefix in ((self.candidate, "addons/sourcemod/"), (self.baseline, "")):
            self.write(base / (prefix + review.OWNED[0]), self.elf)
            self.write(base / (prefix + review.OWNED[1]), b"include fixture")

    def write(self, path, data):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def prepare(self):
        return review.prepare(self.candidate, self.baseline, self.output)

    def test_roundtrip_absence_and_no_private_config(self):
        self.write(self.baseline / "configs/private.cfg", b"must not be bundled")
        result = self.prepare()
        self.assertEqual(result["rollback_absent_paths"], [review.OWNED[2], review.LEGACY_BINARY])
        self.assertFalse(result["release_approved"])
        with zipfile.ZipFile(self.output / "steamworks-p5-rollback.zip") as saved:
            self.assertEqual(len(saved.namelist()), 2)
            self.assertNotIn("addons/sourcemod/configs/private.cfg", saved.namelist())

    def test_existing_gamedata_preserved(self):
        self.write(self.baseline / review.OWNED[2], b"old gamedata\x00")
        result = self.prepare()
        self.assertEqual(result["rollback_absent_paths"], [review.LEGACY_BINARY])
        self.assertEqual(result["rollback_files"][review.OWNED[2]], review.sha(b"old gamedata\x00"))

    def test_legacy_baseline_restores_original_name_and_removes_new_name(self):
        (self.baseline / review.OWNED[0]).rename(self.baseline / review.LEGACY_BINARY)
        result = self.prepare()
        self.assertEqual(result["rollback_absent_paths"], [review.OWNED[0], review.OWNED[2]])
        self.assertIn(review.LEGACY_BINARY, result["rollback_files"])
        with zipfile.ZipFile(self.output / "steamworks-p5-rollback.zip") as saved:
            self.assertEqual(saved.read("addons/sourcemod/" + review.LEGACY_BINARY), self.elf)
            self.assertNotIn("addons/sourcemod/" + review.OWNED[0], saved.namelist())

    def test_baseline_with_both_names_preserves_each_file(self):
        legacy = self.elf + b"legacy bytes"
        self.write(self.baseline / review.LEGACY_BINARY, legacy)
        result = self.prepare()
        self.assertEqual(result["rollback_files"][review.LEGACY_BINARY], review.sha(legacy))
        with zipfile.ZipFile(self.output / "steamworks-p5-rollback.zip") as saved:
            self.assertEqual(saved.read("addons/sourcemod/" + review.LEGACY_BINARY), legacy)
            self.assertEqual(saved.read("addons/sourcemod/" + review.OWNED[0]), self.elf)

    def test_candidate_legacy_name_is_rejected_even_beside_new_name(self):
        self.write(self.candidate / ("addons/sourcemod/" + review.LEGACY_BINARY), self.elf)
        with self.assertRaisesRegex(ValueError, "Unexpected candidate payload"):
            self.prepare()

    def test_review_rejects_build_json_in_installable_candidate(self):
        for name in ('plugin-package-map.json', 'steamworks-sdk.lock.json',
                     'scripts/package/package-map.json', 'scripts/build/steamworks-sdk.lock.json'):
            with self.subTest(name=name):
                path = self.candidate / name
                self.write(path, b'{}')
                with self.assertRaisesRegex(ValueError, 'Unexpected candidate payload'):
                    self.prepare()
                path.unlink()

    def test_unexpected_candidate_refused(self):
        self.write(self.candidate / ".env", b"not public")
        with self.assertRaisesRegex(ValueError, "Unexpected"):
            self.prepare()

    def test_candidate_symlink_refused(self):
        (self.candidate / "docs").symlink_to(self.baseline, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "Symlink"):
            self.prepare()

    def test_baseline_symlink_refused(self):
        path = self.baseline / review.OWNED[0]
        path.unlink()
        path.symlink_to(self.candidate / ("addons/sourcemod/" + review.OWNED[0]))
        with self.assertRaisesRegex(ValueError, "Symlink"):
            self.prepare()

    def test_missing_baseline_refused(self):
        (self.baseline / review.OWNED[0]).unlink()
        with self.assertRaisesRegex(ValueError, "Baseline"):
            self.prepare()

    def test_wrong_architecture_refused(self):
        self.elf[4] = 2
        self.write(self.candidate / ("addons/sourcemod/" + review.OWNED[0]), self.elf)
        with self.assertRaisesRegex(ValueError, "not Linux x86"):
            self.prepare()

    def test_ci_validator_accepts_only_x86_for_linux_l4d2(self):
        env = dict(os.environ, EXTS_ARTIFACT_DIR=str(self.candidate), EXTS_PLATFORM="linux")
        command = ["bash", str(ROOT / "scripts/ci/ci-validate-artifact.sh")]
        accepted = subprocess.run(command, env=env, capture_output=True, text=True)
        self.assertEqual(accepted.returncode, 0, accepted.stderr)
        for elf_class, machine in ((2, 3), (1, 62)):
            header = bytearray(self.elf)
            header[4] = elf_class
            header[18:20] = machine.to_bytes(2, "little")
            self.write(self.candidate / ("addons/sourcemod/" + review.OWNED[0]), header)
            rejected = subprocess.run(command, env=env, capture_output=True, text=True)
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn("Linux x86 ELF32", rejected.stderr)

    def test_existing_output_never_overwritten(self):
        self.prepare()
        with self.assertRaisesRegex(ValueError, "refusing to overwrite"):
            self.prepare()

    def test_archives_reproducible(self):
        first = self.prepare()
        self.output = self.root / "second"
        self.assertEqual(first["archives"], self.prepare()["archives"])

    def test_checkpoint_label_names_archives(self):
        result = review.prepare(self.candidate, self.baseline, self.output, "p5-r2")
        self.assertEqual(result["checkpoint"], "p5-r2")
        self.assertTrue((self.output / "steamworks-p5-r2-linux-x86-candidate.zip").is_file())
        self.assertTrue((self.output / "steamworks-p5-r2-rollback.zip").is_file())

    def test_label_cannot_escape_output(self):
        with self.assertRaisesRegex(ValueError, "Invalid artifact label"):
            review.prepare(self.candidate, self.baseline, self.output, "../escape")
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
