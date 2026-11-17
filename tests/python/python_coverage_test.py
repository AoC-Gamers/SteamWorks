"""Canonical Python coverage scope and source-bound report validation."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('python_coverage', ROOT / 'tests/runners/test-python-coverage.py')
runner = importlib.util.module_from_spec(spec); spec.loader.exec_module(runner)


class PythonCoverageTests(unittest.TestCase):
    def test_only_canonical_sources_contribute_and_missing_lines_stay_zero(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            files = {
                'scripts/tool.py': {'executed_lines': [1, 3], 'missing_lines': [2]},
                'tests/test.py': {'executed_lines': [1], 'missing_lines': []},
                str(root.parent / 'temporary-copy/scripts/tool.py'): {'executed_lines': [2], 'missing_lines': []},
            }
            self.assertEqual(runner.product_lines(root, files),
                             {'scripts/tool.py': {1: True, 2: False, 3: True}})

    def test_python_receipt_rejects_wrong_kind_failed_suite_and_modified_source(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); (root / 'scripts').mkdir()
            source = root / 'scripts/tool.py'; source.write_text('print(1)\nprint(2)\n')
            report = root / 'coverage.xml'
            runner.provenance.write_report(report, {'scripts/tool.py': {1: True, 2: False}})
            receipt = {'schema': 1, 'kind': 'python', 'coverage_version': '7.13.5',
                       'source_state': runner.provenance.source_state(root), 'all_suites_passed': True,
                       'report': report.name, 'report_sha256': runner.provenance.digest(report)}
            path = root / 'receipt.json'; path.write_text(json.dumps(receipt))
            self.assertEqual(runner.provenance.validate_receipt(root, path, kind='python'), report)
            with self.assertRaises(ValueError): runner.provenance.validate_receipt(root, path)
            for change in ({'all_suites_passed': False}, {'coverage_version': 'unknown'}):
                path.write_text(json.dumps({**receipt, **change}))
                with self.assertRaises(ValueError): runner.provenance.validate_receipt(root, path, kind='python')
            path.write_text(json.dumps(receipt)); source.write_text('print(3)\n')
            with self.assertRaises(ValueError): runner.provenance.validate_receipt(root, path, kind='python')


if __name__ == '__main__': unittest.main(verbosity=2)
