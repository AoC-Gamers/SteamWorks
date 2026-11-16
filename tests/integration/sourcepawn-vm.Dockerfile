# Diagnostic toolchain: pass a prepared build image with --build-arg BASE_IMAGE.
# Record its immutable digest and dependency revisions for each validation.
ARG BASE_IMAGE
FROM ${BASE_IMAGE}
USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends clang libclang-rt-dev \
    && rm -rf /var/lib/apt/lists/*
