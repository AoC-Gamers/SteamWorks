from __future__ import annotations

import argparse
import shutil
from pathlib import Path


import importlib.util
_contract_spec = importlib.util.spec_from_file_location("artifact_contract", Path(__file__).with_name("artifact_contract.py"))
_contract = importlib.util.module_from_spec(_contract_spec)
_contract_spec.loader.exec_module(_contract)

def copy_tree(src: Path, dst: Path) -> None:
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root")
    parser.add_argument("build_dir")
    parser.add_argument("output_dir", nargs="?")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    build_dir = Path(args.build_dir).resolve()
    output_dir = (
        Path(args.output_dir).resolve()
        if args.output_dir
        else root / "dist" / "sourcemod" / "artifact"
    )

    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    addons_src = build_dir / "addons"
    if not addons_src.is_dir():
        raise SystemExit(f"Missing addons directory in build output: {addons_src}")

    # Validate before copy so an unexpected SDK, fixture or symlink cannot enter staging.
    for src in addons_src.rglob("*"):
        name = "addons/" + src.relative_to(addons_src).as_posix()
        if src.is_symlink() or src.is_file() and name not in _contract.RUNTIME:
            raise SystemExit("Unexpected build payload: " + name)
    copy_tree(addons_src, output_dir / "addons")

    for file_name in _contract.ROOT_FILES:
        src = root / file_name
        if src.is_symlink() or not src.is_file() or src.stat().st_size == 0:
            raise SystemExit("Missing or empty source documentation: " + file_name)
        shutil.copy2(src, output_dir / file_name)

    _contract.validate_tree(output_dir)

    print(f"SteamWorks artifacts generated in {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
