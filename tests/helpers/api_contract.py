"""Approved additive API contracts, authored separately from the source inventory.

The original fixture stays immutable. Every original field remains exact; only
these explicitly reviewed forward declarations/registrations may be added.
"""

ADDITIONS = {
    "declarations": [
        {"kind": "forward", "name": "SteamWorks_OnUserStatsReceived", "return_type": "void",
         "parameters": ["int authid", "EResult result", "int appid"],
         "signature": "forward void SteamWorks_OnUserStatsReceived(int authid, EResult result, int appid);"},
        {"kind": "forward", "name": "SteamWorks_OnUserStatsUnloaded", "return_type": "void",
         "parameters": ["int authid"],
         "signature": "forward void SteamWorks_OnUserStatsUnloaded(int authid);"},
    ],
    "forward_registrations": [
        {"name": "SteamWorks_OnUserStatsReceived", "file": "extension/core/swstats.cpp",
         "registration_arguments": "ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell"},
        {"name": "SteamWorks_OnUserStatsUnloaded", "file": "extension/core/swstats.cpp",
         "registration_arguments": "ET_Ignore, 1, nullptr, Param_Cell"},
    ],
}


def without_lines(value):
    if isinstance(value, dict):
        return {k: without_lines(v) for k, v in value.items() if k != "line"}
    if isinstance(value, list):
        return [without_lines(v) for v in value]
    return value


def drift(current, baseline):
    errors = []
    for key in sorted(set(baseline) - {"schema_version", "baseline_commit", "source_sha256"}):
        actual = without_lines(current.get(key))
        expected = without_lines(baseline[key])
        if key in ADDITIONS:
            original_names = {entry["name"] for entry in expected}
            if original_names & {entry["name"] for entry in ADDITIONS[key]}:
                raise ValueError("An additive contract cannot override an original API entry")
            additions = [entry for entry in actual if entry["name"] not in original_names]
            if additions != ADDITIONS[key]:
                errors.append(key + ": additive contract mismatch")
            actual = [entry for entry in actual if entry["name"] in original_names]
        if actual != expected:
            errors.append(key + ": original contract changed")
    return errors
