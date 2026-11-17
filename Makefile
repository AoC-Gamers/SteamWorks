SHELL := /usr/bin/env bash
.DEFAULT_GOAL := help

ROOT_DIR := $(shell cd "$$(dirname "$(MAKEFILE_LIST)")" && pwd)
ifeq ($(OS),Windows_NT)
PYTHON ?= python
else
PYTHON ?= $(shell command -v python3 >/dev/null 2>&1 && echo python3 || echo python)
endif
LINUX_DEPS_DIR ?= $(ROOT_DIR)/.deps/exts-linux
WINDOWS_DEPS_DIR ?= $(ROOT_DIR)/.deps/exts-windows
LINUX_BUILD_DIR ?= $(ROOT_DIR)/.build/linux-l4d2
WINDOWS_BUILD_DIR ?= $(ROOT_DIR)/.build/windows-l4d2
LINUX_PACKAGE_DIR ?= $(ROOT_DIR)/.build/package-exts-linux
WINDOWS_PACKAGE_DIR ?= $(ROOT_DIR)/.build/package-exts-windows
LINUX_ARTIFACT_DIR ?= $(ROOT_DIR)/dist/linux/artifact
WINDOWS_ARTIFACT_DIR ?= $(ROOT_DIR)/dist/windows/artifact
STEAMWORKS_SDK_NAME ?= sdk_155
LINUX_RELEASE_BASENAME ?= steamworks-local-linux
WINDOWS_RELEASE_BASENAME ?= steamworks-local-windows

# Public entrypoints share the same runner scripts locally and in CI.
STEAMWORKS_SDK_DIR ?= $(ROOT_DIR)/$(STEAMWORKS_SDK_NAME)
TEST_CXXFLAGS ?= -m32
SOURCEMOD_DIR ?= $(LINUX_DEPS_DIR)/sourcemod-1.12
MMSOURCE_DIR ?= $(LINUX_DEPS_DIR)/mmsource-1.12
AMTL_DIR ?= $(SOURCEMOD_DIR)/public/amtl
HL2SDK_ROOT ?= $(LINUX_DEPS_DIR)
AMBUILD_PYTHON ?= $(LINUX_DEPS_DIR)/.venv-linux/bin/python
export STEAMWORKS_SDK_DIR TEST_CXXFLAGS

CORE_SUITES := stats-natives detours init-coexistence public-ip steamid-string typed-boundary resource-lifecycle restart-forward-semantics binary-evidence debug-log http-stream-context
NATIVE_TEST_TARGETS := $(addprefix test-,$(CORE_SUITES) auth-binding auth-forward sourcepawn-environment sourcepawn-vm-asan)
.PHONY: test-release-notes test test-ci check-ci check-docs validate-sdk test-documentation test-sdk-selection test-review-artifacts test-make-interface test-sdk-incremental validate-artifact-linux validate-artifact-windows $(NATIVE_TEST_TARGETS)

test:
	@test -n "$(TEST_OUTPUT)" || { echo 'Set TEST_OUTPUT to a new results directory' >&2; exit 2; }
	SOURCEMOD_DIR="$(SOURCEMOD_DIR)" MMSOURCE_DIR="$(MMSOURCE_DIR)" AMTL_DIR="$(AMTL_DIR)" $(PYTHON) "$(ROOT_DIR)/tests/runners/test-integration.py" --output "$(TEST_OUTPUT)" $(TEST_OPTIONS)

# This preserves CI coverage without adding the local documentation suite to CI.
test-ci: test-sdk-selection test-review-artifacts test-make-interface test-release-notes $(addprefix test-,$(CORE_SUITES)) test-auth-binding

$(NATIVE_TEST_TARGETS):
	SOURCEMOD_DIR="$(SOURCEMOD_DIR)" MMSOURCE_DIR="$(MMSOURCE_DIR)" AMTL_DIR="$(AMTL_DIR)" bash "$(ROOT_DIR)/tests/runners/$@.sh"

test-sdk-selection:
	$(PYTHON) "$(ROOT_DIR)/tests/python/sdk_selection_test.py"

test-review-artifacts:
	$(PYTHON) "$(ROOT_DIR)/tests/python/review_artifacts_test.py"

test-make-interface:
	$(PYTHON) "$(ROOT_DIR)/tests/python/make_interface_test.py"

test-release-notes:
	$(PYTHON) "$(ROOT_DIR)/tests/python/release_notes_test.py"

test-documentation:
	$(PYTHON) "$(ROOT_DIR)/tests/python/documentation_test.py"

check-docs:
	$(PYTHON) "$(ROOT_DIR)/scripts/tools/check-documentation.py"

check-ci:
	actionlint "$(ROOT_DIR)/.github/workflows/main.yml"

validate-sdk:
	$(PYTHON) "$(ROOT_DIR)/scripts/build/validate-steamworks-sdk.py" "$(STEAMWORKS_SDK_DIR)"

test-sdk-incremental:
	@test -n "$(TEST_OUTPUT)" || { echo 'Set TEST_OUTPUT to a new results directory' >&2; exit 2; }
	"$(AMBUILD_PYTHON)" "$(ROOT_DIR)/tests/runners/test-sdk-incremental.py" --output "$(TEST_OUTPUT)" --sdk "$(STEAMWORKS_SDK_DIR)" --sm-path "$(SOURCEMOD_DIR)" --mms-path "$(MMSOURCE_DIR)" --hl2sdk-root "$(HL2SDK_ROOT)"

validate-artifact-linux:
	EXTS_ARTIFACT_DIR="$(LINUX_ARTIFACT_DIR)" EXTS_PLATFORM=linux bash "$(ROOT_DIR)/scripts/ci/ci-validate-artifact.sh"

validate-artifact-windows:
	EXTS_ARTIFACT_DIR="$(WINDOWS_ARTIFACT_DIR)" EXTS_PLATFORM=windows bash "$(ROOT_DIR)/scripts/ci/ci-validate-artifact.sh"

.PHONY: help deps-exts-linux deps-exts-windows build-exts-linux build-exts-windows package-exts-linux package-exts-windows release-linux release-windows clean-linux clean-windows clean-all

help:
	@printf '%s\n' \
		'Available targets:' \
		'  make test TEST_OUTPUT=<new-dir> [TEST_OPTIONS=...]  Run integrated x86 tests' \
		'  make test-ci              Run CI contract/native suites (SourceHook required)' \
		'  make test-<suite>         Run one named suite; see tests/README.md' \
		'  make test-sdk-incremental TEST_OUTPUT=<new-dir>  Test real optimized builds' \
		'  make validate-sdk        Validate the pinned SDK' \
		'  make check-docs          Check local documentation' \
		'  make check-ci            Lint the workflow (actionlint required)' \
		'  make test-release-notes  Check channel publication with offline doubles' \
		'  make test-documentation  Test documentation and packaging boundaries' \
		'  make validate-artifact-linux|validate-artifact-windows  Validate staged payload' \
		'  make help                 Show this help message' \
		'  make deps-exts-linux      Fetch Linux extension dependencies into .deps/exts-linux/' \
		'  make deps-exts-windows    Fetch Windows extension dependencies into .deps/exts-windows/' \
		'  make build-exts-linux     Build the Linux extension package' \
		'  make build-exts-windows   Build the Windows extension package' \
		'  make package-exts-linux   Stage the Linux package tree' \
		'  make package-exts-windows Stage the Windows package tree' \
		'  make release-linux        Create the Linux release zip' \
		'  make release-windows      Create the Windows release zip' \
		'  make clean-linux          Remove Linux build outputs' \
		'  make clean-windows        Remove Windows build outputs' \
		'  make clean-all            Remove all build outputs and dependencies'

deps-exts-linux:
	DEPS_DIR="$(LINUX_DEPS_DIR)" bash "$(ROOT_DIR)/scripts/deps/fetch-linux-deps.sh"

deps-exts-windows:
	DEPS_DIR="$(WINDOWS_DEPS_DIR)" pwsh -File "$(ROOT_DIR)/scripts/deps/fetch-windows-deps.ps1"

build-exts-linux:
	DEPS_DIR="$(LINUX_DEPS_DIR)" BUILD_DIR="$(LINUX_BUILD_DIR)" STEAMWORKS_SDK_NAME="$(STEAMWORKS_SDK_NAME)" STEAMWORKS_SDK_DIR="$(STEAMWORKS_SDK_DIR)" bash "$(ROOT_DIR)/scripts/build/build-linux-l4d2.sh"

build-exts-windows:
	DEPS_DIR="$(WINDOWS_DEPS_DIR)" BUILD_DIR="$(WINDOWS_BUILD_DIR)" STEAMWORKS_SDK_NAME="$(STEAMWORKS_SDK_NAME)" STEAMWORKS_SDK_DIR="$(STEAMWORKS_SDK_DIR)" pwsh -File "$(ROOT_DIR)/scripts/build/build-windows-l4d2.ps1"

package-exts-linux:
	$(PYTHON) "$(ROOT_DIR)/scripts/package/stage-artifact.py" "$(ROOT_DIR)" "$(LINUX_BUILD_DIR)/package" "$(LINUX_PACKAGE_DIR)"

package-exts-windows:
	$(PYTHON) "$(ROOT_DIR)/scripts/package/stage-artifact.py" "$(ROOT_DIR)" "$(WINDOWS_BUILD_DIR)/package" "$(WINDOWS_PACKAGE_DIR)"

release-linux:
	$(PYTHON) "$(ROOT_DIR)/scripts/package/stage-artifact.py" "$(ROOT_DIR)" "$(LINUX_PACKAGE_DIR)" "$(LINUX_ARTIFACT_DIR)"
	$(PYTHON) "$(ROOT_DIR)/scripts/package/package-release.py" --root "$(ROOT_DIR)" --artifact-dir "$(LINUX_ARTIFACT_DIR)" --basename "$(or $(RELEASE_BASENAME),$(LINUX_RELEASE_BASENAME))"

release-windows:
	$(PYTHON) "$(ROOT_DIR)/scripts/package/stage-artifact.py" "$(ROOT_DIR)" "$(WINDOWS_PACKAGE_DIR)" "$(WINDOWS_ARTIFACT_DIR)"
	$(PYTHON) "$(ROOT_DIR)/scripts/package/package-release.py" --root "$(ROOT_DIR)" --artifact-dir "$(WINDOWS_ARTIFACT_DIR)" --basename "$(or $(RELEASE_BASENAME),$(WINDOWS_RELEASE_BASENAME))"

clean-linux:
	rm -rf "$(LINUX_BUILD_DIR)" "$(LINUX_PACKAGE_DIR)" "$(ROOT_DIR)/dist/linux"

clean-windows:
	rm -rf "$(WINDOWS_BUILD_DIR)" "$(WINDOWS_PACKAGE_DIR)" "$(ROOT_DIR)/dist/windows"

clean-all:
	rm -rf "$(ROOT_DIR)/.build" "$(ROOT_DIR)/dist" "$(ROOT_DIR)/.deps"
