SHELL := /usr/bin/env bash

ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
DEPS_DIR ?= $(ROOT_DIR)/.deps
BUILD_DIR ?= $(ROOT_DIR)/.build/linux-l4d2
STEAMWORKS_SDK_NAME ?= sdk

.PHONY: deps build build-l4d2 build-sdk2013 clean distclean

deps:
	./scripts/fetch-linux-deps.sh

build: build-l4d2

build-l4d2:
	STEAMWORKS_SDK_NAME="$(STEAMWORKS_SDK_NAME)" STEAMWORKS_SDK_DIR="$(STEAMWORKS_SDK_DIR)" ./scripts/build-linux-l4d2.sh

build-sdk2013:
	TARGET_SDK=sdk2013 STEAMWORKS_SDK_NAME="$(STEAMWORKS_SDK_NAME)" STEAMWORKS_SDK_DIR="$(STEAMWORKS_SDK_DIR)" ./scripts/build-linux-l4d2.sh

clean:
	rm -rf "$(BUILD_DIR)"

distclean: clean
	rm -rf "$(DEPS_DIR)"
