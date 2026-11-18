#!/usr/bin/env python3
"""Check stable docs, relative links/anchors and local source references offline.

Uses only Python's standard library; does not publish or operate a server.
"""
import argparse
import importlib.util
from pathlib import Path, PurePosixPath
import re
import unicodedata
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[2]
_spec = importlib.util.spec_from_file_location('artifact_contract', ROOT / 'scripts/package/artifact_contract.py')
_contract = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_contract)


FEATURES = {"installation", "build", "architecture", "runtime", "authentication",
            "identity", "server", "stats", "http", "game-coordinator", "diagnostics", "maintenance"}


def documentation(name):
    parts = PurePosixPath(name).parts
    if re.search(r"(?:^|[-_])(p[0-9]+|r[0-9]+|results?|logs?|snapshots?|acceptance|execution|reports?|checkpoints?)(?:[-_.]|$)", parts[-1].lower()):
        return False
    return (name == "docs/README.md" or
            len(parts) == 3 and parts[0] == "docs" and parts[1] in FEATURES
            and parts[2].endswith(".md") and parts[2] != "AGENTS.md")


def anchors(text):
    seen = {}
    result = set()
    text = re.sub(r"```.*?```", "", text, flags=re.S)
    for title in re.findall(r"^#{1,6}\s+(.+?)\s*#*\s*$", text, re.M):
        title = re.sub(r"\[([^]]+)\]\([^)]*\)", r"\1", title).replace("`", "").lower()
        slug = ''.join(c for c in title if c in '-_ ' or unicodedata.category(c)[0] in 'LN').replace(' ', '-')
        n = seen.get(slug, 0)
        seen[slug] = n + 1
        result.add(slug + (f'-{n}' if n else ''))
    result.update(re.findall(r'<a\s+(?:id|name)=[\"\']([^\"\']+)', text))
    return result


def check(root=ROOT):
    root = Path(root).resolve()
    errors = []
    pages = [root / 'README.md', root / 'AGENTS.md']
    for directory in ('docs', 'tests', 'scripts'):
        pages += [p for p in (root / directory).rglob('*.md') if 'node_modules' not in p.parts]
    for path in (root / 'docs').rglob('*'):
        if not path.is_file():
            continue
        rel = path.relative_to(root / 'docs')
        if not (str(rel) == 'AGENTS.md' or documentation('docs/' + rel.as_posix())):
            errors.append('Unexpected docs payload: ' + str(rel))
    for page in pages:
        if not page.is_file():
            errors.append('Missing page: ' + str(page))
            continue
        text = page.read_text()
        if page.is_relative_to(root / 'docs') or page == root / 'README.md':
            if re.search(r'/home/|/media/|sin (?:commit|push)|\bPID\s*[=:]\s*\d', text):
                errors.append('Private path or checkpoint status: ' + str(page))
            if re.search(r'^#\s+(?:P[0-9]+|R[0-9]+)(?:\b|[-:])', text, re.M):
                errors.append('Execution diary instead of stable contract: ' + str(page))
        for target in re.findall(r'\[[^\]\n]*\]\(([^)]+)\)', text):
            target = target.strip().split(' "')[0]
            url = urlsplit(target)
            if url.scheme or url.netloc:
                continue
            dest = (page.parent / unquote(url.path)).resolve() if url.path else page
            if not dest.is_relative_to(root) or not dest.exists():
                errors.append(f'{page}: broken link {target}')
            elif url.fragment and dest.suffix == '.md' and unquote(url.fragment) not in anchors(dest.read_text()):
                errors.append(f'{page}: broken anchor {target}')
        # Exact repo code paths in prose should not silently drift.
        for target in re.findall(r'`((?:addons|extension|scripting|scripts|tests)/[^`\s]+)`',text):
            if any(c in target for c in '*{}<>') or target.endswith('/'):
                continue
            # Native binaries are build outputs; only the declared extension paths
            # may be documented before a build has populated them.
            if target in _contract.RUNTIME and Path(target).suffix in ('.so', '.dll'):
                continue
            if not (root / target).exists():
                errors.append(f'{page}: missing code reference {target}')
    return errors


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=ROOT)
    args=parser.parse_args()
    errors=check(args.root)
    if errors:
        raise SystemExit('\n'.join(errors))
    print('DOCUMENTATION_OK links, anchors, source references and distribution boundaries')


if __name__=='__main__':
    main()
