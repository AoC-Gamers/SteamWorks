#!/usr/bin/env python3
"""Generate two minimal regressions from current sources, never a Git checkout."""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def generate(output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    cases = (
        ("extension/hooks/swgchooks.cpp", "old-gc.cpp",
         "std::memcpy(snapshot.get(), pubDest, validBytes);",
         "std::memcpy(snapshot.get(), pubDest, cubDest);"),
        ("extension/http/swhttprequest.cpp", "old-http.cpp",
         "pHTTP->GetHTTPResponseBodyData(pRequest->request, body.get(), size)",
         "pHTTP->GetHTTPResponseBodyData(pRequest->request, body.get(), "
         "size < static_cast<uint32>(params[3]) ? size : static_cast<uint32>(params[3] - 1))"),
    )
    for source, name, old, new in cases:
        text = (ROOT / source).read_text()
        if text.count(old) != 1:
            raise ValueError("Negative-control mutation anchor changed: " + source)
        (output / name).write_text(text.replace(old, new))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    generate(parser.parse_args().output)
