# Software Setup Strategy

How this project keeps a developer machine reproducible. Step-by-step instructions are in the [README](../README.md#setup).

## Goal

A new developer gets from a fresh clone to the app running on a Quest using only this repository and the pinned versions below. The repository stays small, public-safe, and free of generated files.

## Pinned baseline

Last verified working set. Change these together, in one commit, and update this table.

| Component | Version | Source of truth |
| --- | --- | --- |
| Unreal Engine | 5.8 (5.8.4 in use) | `HandsTrainSample.uproject` |
| Meta XR plugin | 1.207.0 (v207) | `Plugins/MetaXR/OculusXR.uplugin` (local) |
| Horizon OS SDK | 207 | `Build/Android/ManifestRequirementsAdditions.txt` |
| Android NDK | 27.2.12479018 (r27c) | Engine `Config/Android/Android_SDK.json` |
| Android SDK | build API 36, min 32, target 34 | Engine file above, `Config/DefaultEngine.ini` |
| Package name | `com.samples.HandsTrainSample` | `Config/DefaultEngine.ini` |

## What lives where

| Category | Examples | Handling |
| --- | --- | --- |
| Project source | `Source/`, `Config/`, `Content/`, docs | Committed |
| Licensed SDKs | Meta XR plugin, Interaction SDK | Downloaded by each developer into `Plugins/`, never committed |
| Generated output | `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, most of `Build/` | Ignored, rebuilt locally |
| Secrets and identity | Meta app IDs and tokens, keystores, passwords | Never committed |

## Setup order

Install in dependency order, so each step can be checked before the next one:

1. **Host tools:** Git, Visual Studio with the Unreal C++ workload.
2. **Engine:** Unreal Engine 5.8.
3. **Android toolchain:** Android Studio, then the engine's `SetupAndroid.bat`, so versions match the engine.
4. **SDK plugin:** Meta XR plugin v207 in `Plugins/MetaXR`.
5. **Project:** clone, build the editor target, open the project.
6. **Device:** developer mode on the Quest, package, install and launch with `hzdb`.

## Rules

- **The engine decides toolchain versions.** If external docs disagree, follow the engine's `Android_SDK.json` and `Windows_SDK.json`.
- **Keep the engine and plugin matched.** The plugin declares the engine version it supports in `EngineVersion`.
- **Upgrade on a branch.** Update the engine or plugin, rebuild, test on a headset, then update the table above.
- **Keep publishing credentials out of `Config/`.** Enter them only for an upload, and clear them before committing. Check `git diff Config/` before every commit, since the editor rewrites these files.
- **Use Epic's OpenXR path.** The project relies on OpenXR hand tracking; don't revert to the legacy path.

## Definition of done

Setup is complete when a fresh clone compiles the `Development Editor` target, `HandsInteractionTrainMap` opens in the editor, and a packaged build launches on a Quest with hand tracking working.
