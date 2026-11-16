"""Distribution allowlist shared by staging, ZIP preparation and CI."""
from pathlib import Path, PurePosixPath
import re

ROOT_FILES = ("README.md", "CHANGELOG.md", "LICENSE", "plugin-package-map.json")
FEATURES = {"installation", "build", "architecture", "runtime", "authentication",
            "identity", "server", "stats", "http", "game-coordinator", "diagnostics", "maintenance"}
RUNTIME = {"addons/sourcemod/extensions/steamworks.ext.so",
           "addons/sourcemod/extensions/steamworks.ext.dll",
           "addons/sourcemod/scripting/include/steamworks.inc"}


def documentation(name):
    parts = PurePosixPath(name).parts
    if re.search(r"(?:^|[-_])(p[0-9]+|r[0-9]+|results?|logs?|snapshots?|acceptance|execution|reports?|checkpoints?)(?:[-_.]|$)", parts[-1].lower()):
        return False
    return (name == "docs/README.md" or
            len(parts) == 3 and parts[0] == "docs" and parts[1] in FEATURES
            and parts[2].endswith(".md") and parts[2] != "AGENTS.md")


def allowed(name):
    return name in ROOT_FILES or name in RUNTIME or documentation(name)


def validate_tree(root):
    root = Path(root)
    if not root.is_dir():
        raise ValueError("Missing artifact directory: " + str(root))
    directories = {"docs", *("docs/" + feature for feature in FEATURES)}
    for name in RUNTIME:
        directories.update(str(p) for p in PurePosixPath(name).parents if str(p) != ".")
    for path in sorted(Path(root).rglob("*")):
        name = path.relative_to(root).as_posix()
        if (path.is_symlink() or path.is_file() and not allowed(name)
                or path.is_dir() and name not in directories):
            raise ValueError("Unexpected artifact payload: " + name)
