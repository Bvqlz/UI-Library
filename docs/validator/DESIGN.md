# Task Validator: How It Works and Why

The validator records how participants do on study tasks. It's an Unreal C++ plugin (`TaskValidator`) that you drop into any project. In this repository it lives at `Plugins/TaskValidator`, next to the `HandTrackingTools` plugin, and the sample connects the two (see [../PLUGIN_ARCHITECTURE.md](../PLUGIN_ARCHITECTURE.md)). It covers the "recorded by the validator" metrics in the study strategy section 8: **task success, attempts, time on task, and time to first interaction**.

## The big picture

```
 Admin keyboard (N / H / X)          Interaction code (grab, select, poke)
            │                                       │
            ▼                                       ▼  ReportInteraction(this, Actor)
  ATaskValidatorDirector ──────►  UTaskValidatorSubsystem  ◄──── AValidatorTask actors in the level
   (session start, overlay)        (timer, counters, CSV)         (decide when the goal is reached)
                                            │
                                            ▼
                          Saved/ValidatorLogs/P01_validator.csv
```

| Piece | What it does |
| --- | --- |
| **`UTaskValidatorSubsystem`** | The core. One per game. Holds the participant ID, which task is running, the timer and counters. Writes one CSV row each time a task ends. |
| **`AValidatorTask`** (base) | One study task placed in a level. Has `TaskGroup`, `TaskName`, `Order` and `Instruction`. Subclasses decide what "done" means. |
| **`ATargetZoneTask`** | "Put the object here." An invisible box with no collision. Done when the target object is inside it, within the rotation tolerance, and has stayed still for `DwellSeconds`. |
| **`AEventTask`** | The "boolean" template. Done when C++ code calls `MarkConditionMet()`, or when the participant interacts with one of its `CompleteOnInteractionWith` actors (selection tasks). |
| **`ATaskValidatorDirector`** | Placed once per study level. Starts the session and gives the administrator keys and an on-screen status readout. |
| **`TaskValidatorEditor` module** | Editor only. Adds the **Task Validator** tab to the Place Actors panel. Without it, the actors don't show up anywhere obvious. |
| **`TaskValidatorDemo` module** | **Placeholder only.** A simple pinch grab plus a 10 cm cube, so there's something to test with. In this repository the sample's real interactions replace it: `HandTrackingTools` raises the events and the sample's `UValidatorBridgeSubsystem` reports them. It can be deleted. |

## Lifecycle of one task

1. The overlay shows **NEXT: Manipulation / CubeToMarker** and the instruction text. The administrator reads it aloud (Script A).
2. The administrator presses **N**. The timer starts, the target resets to its start position, and the zone turns yellow.
3. Each grab or selection calls `ReportInteraction`. Attempts go up by one, and the first one also sets time to first interaction.
4. **H** logs an intervention, which marks the task as assisted (the study strategy section 7).
5. The task ends in one of two ways:
   - **Completed:** the task's own condition is met (the zone turns green, then the task finishes after the dwell time).
   - **Abandoned:** the administrator presses **X** because the participant chose to stop. It's recorded as not completed (section 5).
6. One row is appended to the CSV, and the overlay moves on to the next task.

## CSV columns

`ParticipantId, TaskGroup, Task, TaskOrder, Result, Assisted, Interventions, Attempts, TimeToFirstInteraction_s, TimeOnTask_s, StartUtc, EndUtc, PositionError_cm, RotationError_deg`

A blank cell means "not applicable", for example no interaction ever happened, or the task has no rotation check. The first three columns match the linking labels in the study strategy section 11.

## Design choices (and why)

1. **A plugin, not code hardcoded into one project.** It's the same amount of work, and it moves to the UI library unchanged.
2. **Only C++, with no Blueprint dependency.** Everything is configured in the Details panel and code. Blueprint exposure (`UFUNCTION(BlueprintCallable)`) is easy to add later if the team wants it.
3. **The core module has no XR dependency.** The validator doesn't know about hands. Interaction code reports to it with one line: `UTaskValidatorSubsystem::ReportInteraction(this, Actor)`. That lets it validate any interaction method (pinch, ray, distance grab) without changes, which matters because the team hasn't chosen one yet.
4. **The subsystem lives on the GameInstance.** A session survives level loads, so task groups can live in separate levels later.
5. **The target zone uses geometry, not physics or collision.** It checks each tick whether the object's origin is inside the box. There are no collision channels to set up, and it never pushes objects around.
6. **There's a dwell time plus a "settled" speed.** The object has to stay in the zone, nearly still (under 10 cm/s), for 0.5 s. This stops a fling through the zone, or holding it there in passing, from counting. Both values are tunable per task, and the study plans to tune tolerances in the pilot (section 5).
7. **Timing uses the wall clock** (`FPlatformTime::Seconds`), not game time, so pauses and frame hitches don't distort the times.
8. **Attempts count every reported interaction during the task**, with any object. That follows the definition in section 8: "how many times the participant grabbed or selected an object".
9. **A row is appended as each task ends.** If the app crashes mid-session, you lose at most the task that was running.
10. **Earlier logs are never overwritten.** If `P01_validator.csv` exists, the new one becomes `P01_validator_2.csv`. Pilot reruns can't destroy data.
11. **Keys are polled directly** rather than going through Enhanced Input mappings, so the plugin works in any project with no input setup. The keys can be changed in the Details panel.
12. **There are console commands for everything** (`Validator.Next`, and so on), so you can test without a headset.
13. **Task order comes from the `Order` field** (the study requires a fixed order within a group). Randomizing order between participants would be a small future addition.

## Known limitations / next steps

- The admin overlay uses engine debug text. It's fine for Development builds but doesn't show in Shipping builds. That's OK for a study build.
- There's no per-task "reset" key yet. If the administrator makes a mistake, press X (abandon) and note it.
- TLX and admin notes stay in the study spreadsheet by design. The validator doesn't collect them.
- The demo grab is deliberately crude: it has no hand poses, and it uses the palm's rotation for the held offset.
- We could add an extra metric later: path length or hand travel distance during a task.
