set -eu
export SOURCE_DATE_EPOCH=1794700800
export PYTHONPATH=/opt/sm-deps/ambuild
mkdir -p .build/review12
mkdir .build/review12/build
cd .build/review12/build
python3 /workspace/configure.py --sm-path /opt/sm-deps/sourcemod-1.12 --mms-path /opt/sm-deps/mmsource-1.12 --hl2sdk-root /opt/sm-deps --steamworks-path /workspace/sdk_158a --sdks l4d2 --target x86 --enable-optimize
python3 -c 'from ambuild2 import run; run.cli_run()'
