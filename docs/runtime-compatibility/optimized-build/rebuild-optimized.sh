set -eu
export PYTHONPATH=/opt/sm-deps/ambuild
cd .build/auth-ticket-opt/build-optimized
python3 -c 'from ambuild2 import run; run.cli_run()'
