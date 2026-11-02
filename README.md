# Hands Train Sample


## Setup

For the reasoning and version policy behind these steps, see the [Software Setup Strategy](docs/SOFTWARE_SETUP_STRATEGY.md).

### Requirements

| Tool | Version |
| --- | --- |
| Windows | Windows 10 or 11, with Windows SDK 10.0.22621 or a compatible version |
| Git | Any current version |
| Visual Studio | 2022 17.14, or 2026 with MSVC 14.50.35723 or later (the engine's Windows SDK config flags earlier 14.50 builds for compiler errors), with the **Game development with C++** workload |
| Unreal Engine | 5.8 (developed on 5.8.4) |
| Meta XR plugin | v207 (`1.207.0`) |
| Android | NDK 27.2.12479018, SDK platforms `android-36` and `android-34`, build-tools 36.0.0, CMake 3.22.1 |
| JDK | The JDK bundled with Android Studio (its `jbr` folder) |
| Headset | Quest 2, Quest Pro, Quest 3, or Quest 3S in developer mode |

### 1. Clone the repository

```sh
git lfs install
git clone https://github.com/Bvqlz/UI-Library
cd UI-Library
```

The project's assets are currently stored in plain Git, not LFS. The `git lfs install` step is harmless and is kept in case assets move to LFS later.

### 2. Install Unreal Engine 5.8

1. Install the [Epic Games Launcher](https://www.epicgames.com/store/en-US/download).
2. Under **Unreal Engine > Library**, install version 5.8.
3. In the install options, include **Android** as a target platform.

### 3. Install the Android toolchain

1. Install [Android Studio](https://developer.android.com/studio) and open it once so it finishes setting up the SDK.
2. Run `Engine\Extras\Android\SetupAndroid.bat` from your Unreal Engine install. It installs the NDK, platform, build-tools, and CMake versions that the engine expects.
3. In Android Studio's SDK Manager, also install **Android 14 (API 34)**, the project's target API level.
4. Check that `ANDROID_HOME`, `NDKROOT`, and `JAVA_HOME` are set, then sign out and back in or restart your terminal.

> Meta's docs list NDK 25.1.8937393, but Unreal Engine 5.8 requires r27c or later. Use the engine's version, 27.2.12479018.

### 4. Install the Meta XR plugin

The plugin is not included in this repository, and the project won't compile without it.

1. Download **Meta XR plugin v207** from the [Unreal Engine 5 Integration page](https://developers.meta.com/vr/downloads/package/unreal-engine-5-integration/). Select version 207 if a newer one is shown.
2. Extract it into the project so that this file exists:

   ```text
   Plugins/MetaXR/OculusXR.uplugin
   ```

The Meta Interaction SDK is optional. No project code or content currently references it. If you want it, install it at `Plugins/MetaXRInteraction/OculusInteraction.uplugin`.

### 5. Build and open the project

1. Right-click `HandsTrainSample.uproject` and select **Generate Visual Studio project files**.
2. Open the generated solution and set `HandsTrainSample` as the startup project.
3. Select the `Development Editor` configuration and `Win64` platform.
4. Press `F5` to build and launch the editor. The first build can take a while.

The editor opens on `HandsInteractionTrainMap`. If the Meta XR Project Setup Tool flags anything, review it under **Edit > Project Settings > Plugins > Meta XR**. It may report the NDK version noted in step 3.

### 6. Run on a Quest

1. Enable developer mode for your headset, connect it over USB, and accept the debugging prompt inside the headset.
2. In the editor, select **Platforms > Android > Package Project** and choose an output folder.
3. Install and launch the build, replacing the path below with your packaged APK:

   ```sh
   npx -y @meta-quest/hzdb device list
   npx -y @meta-quest/hzdb app install <path to your .apk>
   npx -y @meta-quest/hzdb app launch com.samples.HandsTrainSample
   ```

### Building from the Meta fork (alternative)

The [Meta fork of Unreal Engine](https://github.com/Oculus-VR/UnrealEngine) already includes the Meta XR plugin and supports CPU occlusion, but you must build it from source. You need linked Epic and GitHub accounts, as described in [Accessing Unreal Engine source code on GitHub](https://www.unrealengine.com/en-US/ue-on-github). Use a branch that matches Unreal Engine 5.8, then generate project files from the engine root:

```sh
.\GenerateProjectFiles.bat -Game HandsTrainSample -Engine <full path to this repository>\HandsTrainSample.uproject
```

Open the generated `HandsTrainSample.sln`, then follow steps 3 and 4 of [Build and open the project](#5-build-and-open-the-project).

### Troubleshooting

| Problem | Fix |
| --- | --- |
| `Plugin 'OculusXR' failed to load` or missing `OculusXR*` modules | Check that `Plugins/MetaXR/OculusXR.uplugin` exists (step 4). |
| The editor asks to rebuild modules | Expected on a fresh clone. Accept, or build from Visual Studio (step 5). |
| Android packaging can't find the SDK or NDK | Check the environment variables (step 3), then restart the editor. |
| Low FPS on Quest 2 | See [Known Issues](#known-issues). |

# Licenses
The [Meta License](./LICENSE) applies to the SDK and supporting material. The MIT License applies to only certain, clearly marked documents. If an individual file does not indicate which license it is subject to, then the Meta License applies.

# Changelog
## November 2026
- Add wrist menu, button panel, and furniture product placement code
- Update setup instructions for Unreal Engine 5.8 and Meta XR plugin v207

## April 2025
- Update to use OpenXR from Epic

# Known Issues
- If running on a Quest2 device, you may experience low FPS when using GPU occlusion. You can either switch to the [Meta Fork of Unreal Engine](https://github.com/Oculus-VR/UnrealEngine) and use CPU Occlusions (UE5.5.4 or later)
or add `r.AllowOcclusionQueries 0` over a [blueprint node](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Development/ExecuteConsoleCommand). See LevelBlueprint as example.

## AI coding agents

This repo is wired up for AI coding agents — `AGENTS.md`, `.vscode/extensions.json`, `.mcp.json`, `.cursor/rules/`, and a few client-specific dotfiles surface the **Meta Horizon** VS Code/Cursor extension, the `hzdb` MCP server, and the Meta Quest skill set automatically.

Full toolchain, including Unreal skills and per-client install instructions: [github.com/meta-quest/agentic-tools](https://github.com/meta-quest/agentic-tools).
