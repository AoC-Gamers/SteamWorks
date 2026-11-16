set -eu
export PYTHONPATH=/opt/sm-deps/ambuild
mkdir .build/auth-ticket-opt/build-optimized
cd .build/auth-ticket-opt/build-optimized
python3 /workspace/configure.py --sm-path /opt/sm-deps/sourcemod-1.12 --mms-path /opt/sm-deps/mmsource-1.12 --hl2sdk-root /opt/sm-deps --steamworks-path /workspace/sdk_158a --sdks l4d2 --target x86 --enable-optimize
python3 -c 'from ambuild2 import run; run.cli_run()'
