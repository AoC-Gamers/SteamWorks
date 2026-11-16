# Local diagnostic toolchain. The base image/revisions are recorded in P0;
# capture the resulting image digest before using it as validation evidence.
FROM sourcetvmanager-builder:local
USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends clang libclang-rt-dev \
    && rm -rf /var/lib/apt/lists/*
