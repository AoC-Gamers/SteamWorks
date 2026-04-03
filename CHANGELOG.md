# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Added `ISteamFriends` support to the extension.
- Added persona/profile natives:
  - `SteamWorks_RequestUserInformation`
  - `SteamWorks_RequestUserInformationAuthID`
  - `SteamWorks_GetFriendPersonaName`
  - `SteamWorks_GetFriendPersonaNameAuthID`
  - `SteamWorks_GetFriendPersonaState`
  - `SteamWorks_GetFriendPersonaStateAuthID`
  - `SteamWorks_GetFriendRelationship`
  - `SteamWorks_GetFriendRelationshipAuthID`
  - `SteamWorks_GetPlayerNickname`
  - `SteamWorks_GetPlayerNicknameAuthID`
  - `SteamWorks_GetFriendGamePlayed`
- Added clan-related natives:
  - `SteamWorks_RequestClanOfficerList`
  - `SteamWorks_GetClanOfficerCount`
  - `SteamWorks_GetClanOfficerByIndex`
- Added forwards:
  - `SteamWorks_OnPersonaStateChange`
  - `SteamWorks_OnUserInformationRequested`
- Added `Pawn/steamwork_test.sp` as a local test plugin for grouped SteamWorks validation.
- Added Linux build helper scripts and top-level `Makefile`.
- Added support for selecting Steamworks SDK directories such as `sdk_158a`.

### Changed
- Reworked the GitHub Actions workflow to build from `AoC-Gamers/Steamworks-SDK`.
- Limited the release artifact to:
  - `addons/sourcemod/extensions/SteamWorks.ext.so`
  - `addons/sourcemod/scripting/include/SteamWorks.inc`
- Updated the extension bootstrap/build integration for SourceMod 1.12 and Metamod:Source 1.12 compatibility.
- Updated repository documentation to describe the Linux L4D2 build flow.

### Removed
- Removed the old Travis CI workflow.
- Removed legacy sample plugins that are no longer part of the release package:
  - `Pawn/swag.sp`
  - `Pawn/UpdateCheck.sp`

## [1.2.4]

### Notes
- Existing upstream AoC fork baseline before the current local modernization work.
