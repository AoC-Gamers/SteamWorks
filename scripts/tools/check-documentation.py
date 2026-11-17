#!/usr/bin/env python3
"""Check stable docs, relative links/anchors, local source references and Mermaid.

Offline by default. --render requires the pinned npm toolchain already installed;
it writes diagrams outside docs and neither publishes nor operates a server.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import unicodedata
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[2]
_spec = importlib.util.spec_from_file_location('artifact_contract', ROOT / 'scripts/package/artifact_contract.py')
_contract = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_contract)



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
    errors, diagrams = [], []
    pages = [root / 'README.md', root / 'AGENTS.md']
    for directory in ('docs', 'tests', 'scripts'):
        pages += [p for p in (root / directory).rglob('*.md') if 'node_modules' not in p.parts]
    for path in (root / 'docs').rglob('*'):
        if not path.is_file():
            continue
        rel = path.relative_to(root / 'docs')
        if not (str(rel) == 'AGENTS.md' or _contract.documentation('docs/' + rel.as_posix())):
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
                # Pinned historical source links must resolve in the migration baseline.
                marker='/blob/295e274de754fb65835188f115a2fd4dd917e289/'
                if marker in target and (root / '.build/docs-refactor/source-before').is_dir():
                    historical=unquote(target.split(marker,1)[1].split('#')[0])
                    if not (root / '.build/docs-refactor/source-before' / historical).exists():
                        errors.append(f'{page}: missing pinned historical source {historical}')
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
        for index, block in enumerate(re.findall(r'```mermaid\s*\n(.*?)```',text,re.S)):
            diagrams.append((page.relative_to(root).as_posix(),index,block))
    if len(diagrams) < 6:
        errors.append('Missing required editable Mermaid diagrams')
    return errors, diagrams


def invalid_svg_layout(svg):
    # Mermaid embeds fonts as base64. Encoded bytes may contain "NaN" without
    # representing an invalid SVG coordinate; keep checking all remaining markup.
    markup = re.sub(r'data:[^;,\s]+;base64,[A-Za-z0-9+/=]+', '', svg)
    return bool(re.search(r'NaN|undefined', markup))


def render(diagrams, output):
    output = Path(output).resolve()
    if output.is_relative_to(ROOT / 'docs') or output.is_relative_to(ROOT / 'tests') or output.is_relative_to(ROOT / 'scripts'):
        raise ValueError('Render output must be outside versioned documentation/tools/tests')
    tool = ROOT / 'scripts/tools/node_modules/.bin/mmdc'
    version=json.loads((ROOT / 'scripts/tools/package.json').read_text())['devDependencies']['@mermaid-js/mermaid-cli']
    if not tool.exists():
        raise ValueError('Install pinned Mermaid first: npm ci --prefix scripts/tools')
    actual=subprocess.check_output([str(tool),'--version'],text=True).strip()
    if actual != version:
        raise ValueError(f'Mermaid CLI mismatch: expected {version}, got {actual}')
    output.mkdir(parents=True,exist_ok=True)
    for name,index,body in diagrams:
        stem=name.replace('/','-').replace('.md','')+f'-{index}'
        source=output/(stem+'.mmd');source.write_text(body)
        target=output/(stem+'.svg')
        result=subprocess.run([str(tool),'-i',str(source),'-o',str(target),
                        '-p',str(ROOT/'scripts/tools/puppeteer-config.json')],check=True,
                        capture_output=True,text=True)
        svg=target.read_text()
        if invalid_svg_layout(svg) or 'Error:' in result.stderr:
            raise ValueError('Invalid Mermaid layout: '+stem+'; check renderer and local fonts')
        if re.search(r'<foreignObject[^>]*(?:width|height)="0"',svg):
            # Zero-size labels with text can indicate broken host font selection.
            import xml.etree.ElementTree as ET
            for label in ET.fromstring(svg).iter('{http://www.w3.org/2000/svg}foreignObject'):
                if ''.join(label.itertext()).strip() and (label.get('width')=='0' or label.get('height')=='0'):
                    raise ValueError('Invisible Mermaid label: '+stem+'; check local fonts')
    print(f'MERMAID_RENDER_OK {len(diagrams)} CLI={version}')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=ROOT)
    parser.add_argument('--render',type=Path)
    args=parser.parse_args()
    errors,diagrams=check(args.root)
    if errors:
        raise SystemExit('\n'.join(errors))
    if args.render:
        if args.root.resolve()!=ROOT: raise SystemExit('--render uses the installed toolchain of this source root')
        render(diagrams,args.render)
    print(f'DOCUMENTATION_OK {len(diagrams)} editable diagrams; rendering '+('checked' if args.render else 'not requested'))


if __name__=='__main__':
    main()
