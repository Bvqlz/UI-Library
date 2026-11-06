# Validator test results

CSV output copied from `Saved/ValidatorLogs/`, which git ignores, so reviewers can see what a run produces.

## 2026-11-05_P01_PressButtonPanel_desktop.csv

- **Map:** `Content/Usage/HandsTrainExample/Maps/HandsInteractionTestTask.umap`, a copy of `HandsInteractionTrainMap` with a Task Validator Director (`P01`) and one Event Task.
- **Task:** `Controls / PressControllerButton`, Order 0. It completes when the participant presses any button on the **ButtonPanel**, which is listed in *Complete On Interaction With*.
- **How it was run:** desktop Play, no headset. In the console: `Validator.Next`, then `ke * PressButton 0`. That calls the panel's test function, which broadcasts the same `OnButtonPressed` event a real poke does.
- **Result:** `Completed`, Attempts 1. The interaction went ButtonPanel → `ValidatorBridgeSubsystem` → `ReportInteraction` → Event Task.
- **Note:** the ~0.003 s times aren't meaningful. The press was simulated right after the start, so this run checks the plumbing, not participant timing. A real VR Preview run over Link is still to do.

The task's name says "ControllerButton", but it uses the ButtonPanel. The ControllerBox's buttons no longer drive anything in this map.
