"""Offline strict CXX parser regression; requires the installed parser's classpath."""
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
classpath = os.environ.get("CXX_PARSER_CLASSPATH")
if not classpath:
    raise SystemExit("Set CXX_PARSER_CLASSPATH to matching sonar-cxx libraries and scanner API")
output = Path(os.environ.get("TEST_BUILD_DIR", ROOT / ".build/tests-cxx-parser")).resolve()
output.mkdir(parents=True, exist_ok=True)
subprocess.run(["javac", "-cp", classpath, "-d", str(output),
                str(ROOT / "tests/integration/CxxParserTest.java")], check=True)
subprocess.run(["java", "-cp", str(output) + os.pathsep + classpath, "CxxParserTest", str(ROOT)], check=True)

# Compare real production export spelling with the former spelling on i386,
# including builds that hide every symbol unless explicitly exported.
sdk = (ROOT / "extension/sdk/smsdk_ext.cpp").read_text()
export = re.search(r'^#define SMEXT_DLL_EXPORT\s+(.*gnu::visibility.*)$', sdk, re.M).group(1)
signatures = re.findall(r'^SMEXT_DLL_EXPORT (.+)$', sdk, re.M)
declarations = 'struct IExtensionInterface; struct METAMOD_PLUGIN; struct MetamodVersionInfo; struct MetamodLoaderInfo;\n'
expected = {"GetSMExtAPI", "CreateInterface_MMS"}
tables = []
for name, spelling in (("current", export), ("legacy", export.replace('[[gnu::visibility("default")]]', '__attribute__((visibility("default")))'))):
    source = output / (name + '.cpp')
    source.write_text(declarations + '\n'.join(spelling + ' ' + signature +
                      (' { (void)mvi; (void)mli; return nullptr; }' if 'CreateInterface_MMS' in signature else ' { return nullptr; }')
                      for signature in signatures))
    binary = output / (name + '.so')
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++14', '-m32', '-Wall', '-Wextra', '-Werror',
                    '-pedantic', '-fvisibility=hidden', '-shared', '-fPIC', str(source), '-o', str(binary)], check=True)
    if binary.read_bytes()[:5] != b'\x7fELF\x01': raise ValueError('Expected ELF32 exports')
    table = subprocess.check_output(['readelf', '--dyn-syms', '-W', str(binary)], text=True)
    exports = {parts[-1]: tuple(parts[3:7]) for line in table.splitlines()
               if len(parts := line.split()) >= 8 and parts[-1] in expected}
    if set(exports) != expected or any(row[:3] != ('FUNC', 'GLOBAL', 'DEFAULT') or row[3] == 'UND' for row in exports.values()):
        raise ValueError('Factory linkage or default export visibility changed')
    tables.append(exports)
if tables[0] != tables[1]: raise ValueError('GNU attribute spellings changed export ABI')
print('PASS: both factory exports preserve unmangled GLOBAL DEFAULT functions on GCC x86')
