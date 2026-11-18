"""Distribution allowlist shared by staging, ZIP preparation and CI."""
import json
from pathlib import Path, PurePosixPath

PACKAGE_MAP = Path(__file__).with_name("package-map.json")
ROOT_FILES = tuple(json.loads(PACKAGE_MAP.read_text(encoding="utf-8"))["artifact"]["documentation"])
RUNTIME = {"addons/sourcemod/extensions/steamworks.ext.2.l4d2.so",
           "addons/sourcemod/extensions/steamworks.ext.2.l4d2.dll",
           "addons/sourcemod/scripting/include/steamworks.inc"}


def allowed(name):
    return name in ROOT_FILES or name in RUNTIME


def validate_tree(root):
    root = Path(root)
    if not root.is_dir():
        raise ValueError("Missing artifact directory: " + str(root))
    for name in ROOT_FILES:
        path = root / name
        if path.is_symlink() or not path.is_file() or path.stat().st_size == 0:
            raise ValueError("Missing or empty packaged documentation: " + name)
    directories = set()
    for name in RUNTIME:
        directories.update(str(p) for p in PurePosixPath(name).parents if str(p) != ".")
    for path in sorted(Path(root).rglob("*")):
        name = path.relative_to(root).as_posix()
        if (path.is_symlink() or path.is_file() and not allowed(name)
                or path.is_dir() and name not in directories):
            raise ValueError("Unexpected artifact payload: " + name)
