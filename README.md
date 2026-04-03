SteamWorks
==========

Exposing SteamWorks functions to SourcePawn.

Linux L4D2 build flow
---------------------

This repo includes helper scripts to fetch the build dependencies used for the
L4D2 Linux extension build and then compile the extension with AMBuild.

Prerequisites:
- `bash`
- `git`
- `python3`
- a working 32-bit Linux build toolchain

Commands:

```bash
make deps
make build-l4d2
make build-sdk2013
make build-l4d2 STEAMWORKS_SDK_NAME=sdk_158a
```

Artifacts:
- `.build/linux-l4d2/package/addons/sourcemod/extensions/SteamWorks.ext.so`
- `.build/linux-sdk2013/package/addons/sourcemod/extensions/SteamWorks.ext.so`

CI workflow
-----------

GitHub Actions builds the Linux L4D2 extension by:

- checking out this repository
- checking out `AoC-Gamers/Steamworks-SDK`
- selecting an SDK directory such as `sdk_158a`
- running `make deps`
- running `make build-l4d2`

The workflow artifact intentionally contains only:

- `addons/sourcemod/extensions/SteamWorks.ext.so`
- `addons/sourcemod/scripting/include/SteamWorks.inc`

Test sources such as `Pawn/steamwork_test.sp` are not compiled into the release
artifact and are not packaged by CI.

Paths can be overridden with environment variables such as `DEPS_DIR`,
`BUILD_DIR`, `HL2SDK_DIR`, `SOURCEMOD_DIR`, `MMSOURCE_DIR`,
`STEAMWORKS_SDK_DIR`, `STEAMWORKS_SDK_NAME`, and `VENV_DIR`.

`STEAMWORKS_SDK_NAME` defaults to `sdk`, so if you keep multiple extracted
SDK folders in the repo you can switch between them without editing scripts.
For example:

```bash
make build-l4d2 STEAMWORKS_SDK_NAME=sdk_158a
```
