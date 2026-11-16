set -eu
export PYTHONPATH=/opt/sm-deps/ambuild
mkdir .build/auth-ticket-opt/build-canonical
cd .build/auth-ticket-opt/build-canonical
python3 /workspace/configure.py --sm-path /opt/sm-deps/sourcemod-1.12 --mms-path /opt/sm-deps/mmsource-1.12 --hl2sdk-root /opt/sm-deps --steamworks-path /workspace/sdk_158a --sdks l4d2 --target x86
python3 -c 'from ambuild2 import run; run.cli_run()'
