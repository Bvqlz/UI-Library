# Task Validator: Install and Demo Guide

A complete walkthrough, from a fresh project to a recorded demo. It's written for UE 5.8, Visual Studio 2026 and a Quest 2 over Link.

- **Part 1:** Install the plugin into a project
- **Part 2:** Build it and confirm it loaded
- **Part 3:** Set up a demo level
- **Part 4:** Test on desktop (no headset)
- **Part 5:** Run it in the headset and record the demo
- **Part 6:** Read the results
- **Part 7:** Troubleshooting

How it works and why it's built this way: see `DESIGN.md` in this folder. How it connects to the hand tracker and the sample, and how to turn an existing sample interaction into a task: see [../PLUGIN_ARCHITECTURE.md](../PLUGIN_ARCHITECTURE.md).

---

## Part 1: Install the plugin into a project

> **Already done for this repository.** The plugin is committed at `Plugins/TaskValidator`, so a fresh clone has it. Skip to Part 2. Parts 1 and the rest of this guide also apply to any other project you want to add it to.

**Requirement:** the project must be a **C++ project**, meaning it has a `Source` folder and a `.sln`. If it's Blueprint-only, open it and use **Tools → New C++ Class → None → Create Class** once. That converts it.

1. **Close the Unreal Editor** for that project.
2. Copy the plugin folder. Take it from this repository's `Plugins/TaskValidator`. It goes to `<YourProject>\Plugins\TaskValidator`. Create the `Plugins` folder if it doesn't exist.

   When you're done, this file must exist (check the path, it's easy to end up with one folder too many):
   ```
   <YourProject>\Plugins\TaskValidator\TaskValidator.uplugin
   ```
3. Don't copy any `Binaries` or `Intermediate` folders from another project. They get rebuilt.

The plugin has three code modules. You don't need to do anything with them; this is for reference:

| Module | Loaded in | Purpose |
| --- | --- | --- |
| `TaskValidator` | Editor and game | The validator itself |
| `TaskValidatorDemo` | Editor and game | Placeholder pinch grab and cube for demos |
| `TaskValidatorEditor` | Editor only | Adds the **Task Validator** tab to the Place Actors panel |

---

## Part 2: Build it and confirm it loaded

Unreal only loads a C++ plugin after it has been compiled. **The editor must be closed while you build.** Live Coding blocks builds while it's open.

### Option A: Visual Studio (most reliable)

1. Close the Unreal Editor.
2. In File Explorer, right-click `HandsTrainSample.uproject` and choose **Generate Visual Studio project files**. On Windows 11 this may be under **Show more options**. This makes the plugin's files show up in Visual Studio.
3. Open `HandsTrainSample.sln`.
4. In the top toolbar, set the dropdowns to **Development Editor** and **Win64**.
5. In Solution Explorer, right-click **Games → HandsTrainSample** and choose **Build**.
6. Wait for `Build: 1 succeeded` in the Output window. After the first full build, this takes about 1 minute.
7. Open `HandsTrainSample.uproject`.

### Option B: Let the editor build it

Open the `.uproject`. If it says *"The following modules are missing or built with a different engine version… Would you like to rebuild them now?"*, click **Yes**. If it then says it *could not be compiled*, use Option A, which shows the actual error.

### Option C: Command line

With the editor closed:
```bash
"<UE 5.8 install>/Engine/Build/BatchFiles/Build.bat" HandsTrainSampleEditor Win64 Development "-Project=<repo>/HandsTrainSample.uproject" -WaitMutex
```
It succeeded if it ends with `Result: Succeeded`.

### Confirm it loaded

1. **Edit → Plugins**, then search `Task Validator`. It should be listed under **Project → Testing** with its checkbox **ticked**. If it's unticked, tick it and restart the editor.
2. **Window → Place Actors.** In the left column of category tabs, there should be a **Task Validator** tab (a test-tube style icon) with five actors:
   - Task Validator Director
   - Target Zone Task
   - Event Task
   - Demo Pinch Grabber
   - Demo Grabbable Cube

**If you don't see the Task Validator tab**, you're running an older build from before that tab existed. Close the editor and do Option A again. In the meantime, you can still find the actors either of these ways:
- **Content Browser:** click **Settings** (top-right of the Content Browser) and tick **Show C++ Classes** and **Show Plugin Content**. Then in the folder tree, open **TaskValidator C++ Classes → TaskValidator → Public** and drag a class into the viewport. The demo actors are under **TaskValidatorDemo → Public**.
- **Place Actors search box:** type `Target Zone` and similar names. The search covers all classes.

---

## Part 3: Set up a demo level

Everything here happens in the editor's Details panel. No Blueprints.

> **Shortcut:** Part 3 builds a demo with the placeholder cube. To use the sample's own content instead (select the windmill, place a chair), follow [section 5 of PLUGIN_ARCHITECTURE.md](../PLUGIN_ARCHITECTURE.md#5-tutorial-implement-your-initial-test-task-as-an-existing-task-within-the-sample). The Director and level steps are the same.

### 3.1 Make a copy of the level

1. In the Content Browser, open `Content/Usage/HandsTrainExample/Maps/HandsInteractionTrainMap` (double-click it).
2. **File → Save Current Level As…** Name it `ValidatorDemoMap` and save it in the same folder. Now you're editing the copy, and the sample map stays clean.

### 3.2 Find where the player stands

In the **Outliner** (top right), search `PlayerStart` and double-click it. The viewport flies to it. You'll place everything about an arm's length in front of it, at chest height. The Player Start's arrow shows which way the player faces.

### 3.3 Place the actors

Open **Window → Place Actors → Task Validator** and drag each actor into the viewport. Then select it and edit its settings in the **Details** panel.

**A. Task Validator Director** (one per level; its position doesn't matter)
| Setting | Value |
| --- | --- |
| Validator → Participant Id | `P01` |
| Validator → Show Admin Overlay | ticked |

**B. Demo Pinch Grabber** (one per level; its position doesn't matter). Leave the defaults.

**C. Demo Grabbable Cube**
- Place it about **60 cm in front of the Player Start** and about **120 cm above the floor**. In the Details panel under **Transform → Location**, take the Player Start's location and add roughly 60 in the facing direction and 120 to Z.
- Leave **Simulate Physics** unticked for the demo, so the cube floats where you let go of it.

**D. Target Zone Task** (the invisible "put it here" box)
- Place it about **40 cm to the left or right of the cube**, at the same height.
- In the Details panel:

| Setting | Value |
| --- | --- |
| Validator → Task Group | `Manipulation` |
| Validator → Task Name | `CubeToMarker` |
| Validator → Order | `1` |
| Validator → Instruction | `In this task, move the cube into the box. Begin when you're ready.` |
| Validator → Zone → Target Actor | the cube. Click the dropdown and pick `DemoGrabbableCube`, or click the **eyedropper** and then click the cube in the viewport. |
| Validator → Zone → Rotation Check | `Ignore` |
| Validator → Zone → Dwell Seconds | `0.5` |
| Validator → Zone → Show Zone In Game | ticked |

- **Box size:** at the top of the Details panel, click the **Zone** component, then under **Shape** set **Box Extent** to `8, 8, 8`. That makes a 16 cm box, the position tolerance. Bigger is easier.

### 3.4 Optional second task (shows ordering and the rotation check)

1. Select both the cube and the zone, hold **Alt** and drag to duplicate them a little further along.
2. On the new zone: set **Task Name** to `CubeToMarker_Upright`, **Order** to `2`, **Rotation Check** to `Upright Only`, and **Target Actor** to the *new* cube.
3. Rotate the new cube about 45 degrees so the participant has to straighten it.

### 3.5 Save

**Ctrl+S** (or File → Save All).

---

## Part 4: Test on desktop (no headset)

Do this before recording, to confirm the setup is right.

1. In the toolbar, click the **⋮** (three dots) next to the Play button and choose **Selected Viewport**. Then click **Play**.
2. In the top-left of the viewport you should see the overlay:
   ```
   [Validator] P01   log: P01_validator.csv
   NEXT  Manipulation / CubeToMarker   (1 left)   read instruction, then press N
   "In this task, move the cube into the box. Begin when you're ready."
   ```
3. Click inside the viewport, then press **N**. The overlay switches to `RUNNING … 0.0s attempts 0` and the zone outline turns **yellow**.
4. Press **F8** to "eject" from the player. Now you can click objects in the level.
5. Click the cube and drag it with the move gizmo into the box. The outline turns **green**. After half a second, the task completes and the overlay moves to the next task, or says *All tasks done*.
6. Press **Esc** to stop playing.
7. Open `<repo>\Saved\ValidatorLogs\P01_validator.csv`. It should have one row.

Attempts will be 0 here, because there's no hand grab on desktop. To fake one, open the console with the **`** key (backquote) and type `Validator.FakeInteraction`.

Every action has a console command, useful if keyboard focus is a problem:

| Command | Same as |
| --- | --- |
| `Validator.Next` | N |
| `Validator.Intervention` | H |
| `Validator.Abandon` | X |
| `Validator.Session P02` | start a new session/CSV for participant P02 |
| `Validator.ForceComplete` | debug: complete the running task |
| `Validator.FakeInteraction` | debug: count one attempt |

---

## Part 5: Run it in the headset and record the demo

### 5.1 Headset and PC checklist
- **Quest 2:** connected with the Link cable. Hand tracking is on (Settings → Movement tracking → Hand and body tracking).
- **Meta Horizon Link app** on the PC: the headset shows as connected.
  - **Settings → General → OpenXR Runtime:** set Meta Horizon Link as active.
  - **Settings → Beta → Developer Runtime Features:** on. Hand tracking over Link won't work without this.
- In the headset, start **Quest Link** from the quick settings.

### 5.2 Start the session
1. In the editor, click **⋮** next to Play and choose **VR Preview**. If it's greyed out, Link isn't connected or isn't the OpenXR runtime.
2. Put the controllers down and hold your hands in front of the headset until the virtual hands appear.
3. **Click once on the editor/VR Preview window on the PC monitor** so the keyboard keys reach the game.

### 5.3 Record
Start a screen recorder on the PC. **Win+Alt+R** (Xbox Game Bar) works, or use OBS. The PC window mirrors the headset view and should show the overlay. If the overlay isn't visible in VR, the same information is in the **Output Log** (filter `LogTaskValidator`), so keep that panel on screen.

### 5.4 Demo script (about 2 minutes)
1. **Intro:** "This is the validator. It times each task and counts attempts automatically, and writes a CSV per participant." Point at the overlay.
2. **Task 1:** read the instruction aloud, then press **N**. Pinch the cube (thumb to index fingertip; a small sphere at your fingertips turns green while pinching), move it into the box and let go. Point out that the attempts counter went up when you grabbed, the box turned green, and the task completed.
3. **Task 2:** press **N**. Press **H** once ("the participant asked for help, so it's logged and the task is marked as assisted"). Then press **X** ("the participant chose to stop, so it's recorded as not completed").
4. Stop play, open the CSV in Excel, and walk through the columns (Part 6).
5. **Closing points:**
   - It's a standalone plugin, and hooking up any interaction is one line of C++: `UTaskValidatorSubsystem::ReportInteraction(this, Actor);`
   - Tasks are set up in the editor, not hardcoded. Target-zone tasks cover move and place; event tasks cover press, select and menus.
   - The CSV uses the participant/group/task labels from the study plan.
   - The pinch grab is a placeholder until the team chooses the interaction method.

---

## Part 6: Read the results

File: `<Project>\Saved\ValidatorLogs\P01_validator.csv`. A second run for the same participant creates `P01_validator_2.csv`. Nothing is ever overwritten.

| Column | Meaning |
| --- | --- |
| ParticipantId, TaskGroup, Task | Labels that link to the study spreadsheet |
| TaskOrder | The task's Order value |
| Result | `Completed` or `Abandoned` (the participant stopped) |
| Assisted | 1 if any intervention was logged |
| Interventions | Number of times H was pressed |
| Attempts | Grabs or selections during the task |
| TimeToFirstInteraction_s | Seconds from N to the first grab. Blank if there was none. |
| TimeOnTask_s | Seconds from N to completed or abandoned |
| StartUtc, EndUtc | Timestamps |
| PositionError_cm | Distance from the object to the zone centre at the end |
| RotationError_deg | Rotation error at the end. Blank when Rotation Check is Ignore. |

---

## Part 7: Troubleshooting

| Problem | Fix |
| --- | --- |
| No **Task Validator** tab in Place Actors | Close the editor and rebuild (Part 2, Option A). Use the Content Browser route meanwhile. |
| Build says *Unable to build while Live Coding is active* | Close the editor completely, then build again. |
| Plugin missing from Edit → Plugins | Check that the path is exactly `Plugins\TaskValidator\TaskValidator.uplugin`. |
| Overlay doesn't appear | The level needs a **Task Validator Director**. Check that Show Admin Overlay is ticked. |
| Overlay says *All tasks done* immediately | No task actors are in the level, or they were all already run. Stop and Play again. |
| N / H / X do nothing | Click the game window first. In desktop PIE, **Shift+F1** frees the mouse. Or use the console commands. |
| Zone never turns green | Check that **Target Actor** is set on the zone, and that the cube's centre is actually inside the box. |
| Turns green but never completes | The cube is still moving or still held. Let go and keep it still for 0.5 s. |
| Can't grab the cube in VR | The pinch sphere must be within about 6 cm of the cube. The level needs a **Demo Pinch Grabber**. |
| No hands in VR Preview | Turn on Developer Runtime Features and make Link the OpenXR runtime (Part 5.1). |
| VR Preview is greyed out | The headset isn't in Quest Link, or Link isn't the active OpenXR runtime. |
| Low FPS on Quest 2 | A known sample issue. Run the console command `r.AllowOcclusionQueries 0`. |

### Removing the demo module for a real study build
Delete `Plugins/TaskValidator/Source/TaskValidatorDemo`. Remove its entry from `TaskValidator.uplugin`, and remove `"TaskValidatorDemo"` and the two demo lines from `Source\TaskValidatorEditor` (the Build.cs and the `Add(ADemo…)` lines). Then rebuild.
