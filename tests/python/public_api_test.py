"""Preserve the independent original inventory while approving only named additions."""
from copy import deepcopy
import importlib.util
import json
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[2]


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


contract = module('contract', ROOT/'tests/helpers/api_contract.py')
inventory = module('inventory', ROOT/'scripts/tools/inventory-steamworks.py')


class PublicAPITests(unittest.TestCase):
    def setUp(self):
        self.baseline = json.loads((ROOT/'tests/fixtures/api/api-inventory-p0.json').read_text())
        self.current = inventory.api_inventory(ROOT)

    def test_original_contract_and_explicit_additions(self):
        self.assertEqual(contract.drift(self.current, self.baseline), [])

    def test_unapproved_or_missing_additions_fail(self):
        for key in contract.ADDITIONS:
            for operation in ('missing', 'extra', 'arity', 'duplicate'):
                with self.subTest(key=key, operation=operation):
                    value = deepcopy(self.current)
                    index = next(i for i, entry in enumerate(value[key]) if entry['name']=='SteamWorks_OnUserStatsReceived')
                    if operation == 'missing': value[key].pop(index)
                    elif operation == 'extra':
                        extra = deepcopy(value[key][index]); extra['name']='SteamWorks_Unapproved'; value[key].append(extra)
                    elif operation == 'duplicate': value[key].append(deepcopy(value[key][index]))
                    elif key == 'declarations': value[key][index]['parameters'].pop()
                    else: value[key][index]['registration_arguments']='ET_Ignore, 2, nullptr, Param_Cell, Param_Cell'
                    self.assertTrue(contract.drift(value, self.baseline))

    def test_legacy_changes_are_not_hidden_by_additions(self):
        for key in ('declarations','native_registrations','forward_registrations','optional_natives'):
            with self.subTest(key=key):
                value = deepcopy(self.current)
                value[key].pop(0)
                self.assertTrue(contract.drift(value, self.baseline))
        value = deepcopy(self.current)
        value['extension_binding']['file']='different.ext'
        self.assertTrue(contract.drift(value,self.baseline))


if __name__ == '__main__': unittest.main(verbosity=2)
