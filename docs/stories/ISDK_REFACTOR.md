# Project Story: Move the Validator and Bridge to the Meta Interaction SDK

| | |
| --- | --- |
| **Status** | In progress (phase 1 in this branch) |
| **Area** | `Plugins/TaskValidator`, `Source/HandsTrainSample/ValidatorBridgeSubsystem.*` |
| **Branch** | `refactor/isdk-validator-bridge` |

## Story

**As** the study team,
**we want** the validator to count attempts from the Meta Interaction SDK's built-in interactions (poke, ray, grab) instead of our own hand-written gestures,
**so that** the interaction a participant uses is the same tested, documented interaction Meta ships, the study measures that interaction and not our gesture code, and we keep less code.

## Why

- **Our gestures repeat what the SDK already does.** `HandTrackingTools` has its own pinch detection (`PinchStateModule`), poke tool (`AFingerTipPokeTool`) and ray tool (`ARayTool`). `TaskValidatorDemo` has another pinch grab (`ADemoPinchGrabber`). The Interaction SDK ships poke, ray, hand grab and distance grab, tuned and validated by Meta, and they work with both hands and controllers.
- **One event shape for every interaction.** Every Interaction SDK interaction emits the same pointer events (Hover, Unhover, Select, Unselect, Move, Cancel) through `IIsdkIPointable`. The bridge can listen to that one contract instead of one delegate per custom tool, so a new interaction method needs no bridge change.
- **The study's definition of an attempt matches the SDK.** The study strategy (section 8) defines an attempt as "how many times the participant grabbed or selected an object". In the SDK, that is exactly a `Select` pointer event, whether it came from a poke, a ray or a grab.
- **The demo code was placeholder only.** `TaskValidatorDemo` (`ADemoPinchGrabber`, `ADemoGrabbableCube`) existed so the validator had something to test with before real interactions existed. It is a third custom gesture and must go.

## Scope

### Phase 1: validator and bridge (this branch)

1. **Remove the demo code.** Delete the `TaskValidatorDemo` module, its `TaskValidator.uplugin` entry, the `XRBase` plugin dependency it needed, and its two entries in the Place Actors tab.
2. **Refactor the validator** (`TaskValidator`, still no XR dependency):
   - `ReportInteraction(WorldContext, Actor, Method)` takes an optional interaction method name (`Poke`, `Ray`, `Grab`, ...). It is callable from Blueprint, because Interaction SDK setups are mostly Blueprint.
   - The CSV gains one trailing column, `InteractionMethods`: the distinct methods used during the task, such as `Ray` or `Poke;Ray`. Existing columns keep their order.
   - One physical action counts as one attempt. If two sources report the same actor in the same frame (for example a ray `Select` on a product and the placer's "product selected" event), the second report is ignored.
   - `AEventTask` gains **Complete On Interaction With Tag**, so a task can complete on actors that only exist at runtime without writing a task class.
   - `MarkConditionMet` is callable from Blueprint, task classes can be Blueprint subclasses, and tasks get **On Task Begin** and **On Interaction** Blueprint events.
   - The administrator overlay shows the methods used so far (`via Ray`).
3. **Make the bridge general** (`UValidatorBridgeSubsystem`):
   - It binds automatically to every component that implements the Interaction SDK's `IIsdkIPointable` (`IsdkPokeInteractable`, `IsdkRayInteractable`, grab interactables, interactable widgets), on actors present at start and actors spawned later.
   - A `Select` pointer event counts as an attempt. Hover, Move, Unselect and Cancel do not.
   - The method name is taken from the interactable's class (`IsdkPokeInteractable` → `Poke`).
   - It reports the placed owner (the root of the attachment chain), unless an actor in the chain carries the tag `ValidatorTarget`, which then wins.
   - It no longer listens to our custom gestures (`AInteractable`, `AWristToolkit`, `AButtonPanel`) and includes no `HandTrackingTools` header.
   - It keeps two sample-content sources that are not gestures: product selection (`AProductPlacer`) and product edits (`UProductHistorySubsystem`), so the sample's first test task (`AProductPlacementTask`) still works.
   - Any other source reports with one call: `UValidatorBridgeSubsystem::Report(this, Actor, Method)` (C++ or Blueprint).
4. **Project setup.** The Interaction SDK plugin (`OculusInteraction`) becomes required: enabled in `HandsTrainSample.uproject` and listed in the sample's `Build.cs`. Docs are updated.

### Phase 2: migrate the sample scene (in progress, branch `refactor/isdk-scene-phase2`)

The sample scene is only there to test the validator. It is changed to use the Interaction SDK.

**Done in code:**

1. **`USelectRelayComponent`** (new, in the sample). Add it next to any Interaction SDK interactables on an actor. It listens to their pointer events and raises `OnSelected(Method)`, `OnUnselected`, `OnHoverStarted` and `OnHoverEnded`, counting pointers so two hands or a hand and a controller don't double-fire. Gameplay code reacts to it and never needs to know which interaction was used. It replaces the `AInteractable` / `AInteractableTool` state machine for scene objects.
2. **`IsdkPointables`** (new). The one file that names the Interaction SDK pointer headers, with helpers shared by the bridge and the relay (find a pointable, get the method).
3. **Windmill and crossing.** They no longer spawn the custom `ACollidableInteractable`. Each has a `SelectRelay`: selecting the windmill starts or stops the blades, and selecting the crossing rings it. The selection cylinder looks the same as before: shown on hover, brighter while pinching.
4. **Controller box.** The eight custom `AInteractableButton` actors are replaced by Meta's rounded poke buttons (`UIsdkRoundedButtonComponent`), created at BeginPlay on the same FBX anchors, with labels. A poke `Select` runs the button's action. A new `PressButton` runs an action from C++ or Blueprint. The editor keyboard shortcuts still work. The buttons are components of the controller box, so the validator reports the **ControllerBox**, as before.

**Still to do in the editor** (Blueprint and map assets, which can't be edited from code):

1. **Pawn.** Give the pawn the Interaction SDK rigs, as in Meta's *Getting Started with Interaction SDK*: a left and a right `MotionControllerComponent`, each with an `IsdkHandRigComponent` and an `IsdkControllerRigComponent`, plus the `IMC_IsdkHand` and `IMC_IsdkControllerAnimation` input mapping contexts. Remove `BP_InteractableToolsManager` from the level.
2. **Windmill (`BP_Windmill`) and crossing (`BP_XingPost`).** Add an `IsdkRayInteractable`, with its pointable surface, at `CollidableInteractAnchor`. Delete the now-unused `CollidableInteractableBP` default if the editor flags it.
3. **Controller box (`BP_ControllerBox`).** Play once and check the buttons face the user. If they don't, set **Buttons → Button Rotation Offset** (and **Button Size**). Delete the now-unused `InteractableButtonBP` default if the editor flags it.
4. **Check.** Re-run the two README test tasks (select the windmill, place a chair) on desktop and on a Quest, and add the CSVs to `docs/validator/test-results`.

**Still to do in code:**

1. **Furniture tools.** `AWristMenu`, `AProductPlacer` and `AProductEditTools` still do their own fingertip poke, pinch and pointing ray through `HandPointerUtils`, and the wrist toolkit (`AWristToolkit`, in `HandTrackingTools`) has its own poke items. Move these to Interaction SDK poke buttons, ray interactables and grabbables. This is the largest piece (about 1,900 lines), so it gets its own branch.
2. **Hand visuals.** The controller box's hand-style button still uses `AHandsVisualizationSwitcher` from `HandTrackingTools`. Decide whether the SDK's hand visuals replace it.

### Phase 3: retire our gestures (after phase 2)

Remove the custom gesture code from `HandTrackingTools` once no content uses it (`PinchStateModule`, `AFingerTipPokeTool`, `ARayTool`, `AInteractableToolsManager`, `InteractableToolsInputRouter`, `AInteractable` and its zones), along with the `[CoreRedirects]` that point at them. Keep anything the SDK does not provide.

## Out of scope

- Hand pose recognition (thumbs up, and so on). The study's methods are pinch grab, ray + pinch, whole-hand grab and distance grab, which are all pointer interactions.
- Changing what the CSV already records, or the administrator keys.
- Changing `HandTrackingTools` in phase 1.

## Acceptance criteria

Phase 1 is done when:

- [ ] The project builds in `Development Editor | Win64` with the Meta XR plugin and the Interaction SDK plugin installed.
- [ ] `TaskValidatorDemo` no longer exists, and nothing references it.
- [ ] `grep -r "HandTrackingTools\|OculusXR\|Isdk" Plugins/TaskValidator/Source` finds nothing outside comments (the validator stays independent).
- [ ] `ValidatorBridgeSubsystem.*` includes no `HandTrackingTools` header.
- [ ] An actor with an `IsdkRayInteractable`, listed in an Event Task's **Complete On Interaction With**, completes the task when selected with a ray. The CSV row shows `Attempts` ≥ 1, `TimeToFirstInteraction_s` filled in and `InteractionMethods` = `Ray`.
- [ ] The same with an `IsdkPokeInteractable` gives `Poke`.
- [ ] A cube with an `IsdkGrabbableComponent`, used as a Target Zone Task's target, gives `Grab` when pinched (validator guide, Part 3). If grabs are not counted, the grabbable does not raise pointer events in this SDK release: bind its grab delegate in the bridge instead.
- [ ] Hovering without selecting adds no attempt.
- [ ] `Validator.FakeInteraction` still counts an attempt (method `Console`).
- [ ] The Product Placement Task still completes from the desktop keys (`M`, `1`–`5`, `Space`).

## Risks and notes

- **Not compiled yet.** Phase 1 was written without an Unreal build. The bridge uses three Interaction SDK names: the interface `IIsdkIPointable` / `UIsdkIPointable` and its `GetInteractionPointerEventDelegate()`, the struct `FIsdkInteractionPointerEvent` (with `Type` and `Interactable`), and the enum `EIsdkPointerEventType`. These names come from Meta's pointer events documentation and from Meta's Interaction SDK sample. The two include paths for them, at the top of `Source/HandsTrainSample/IsdkPointables.h`, are the first thing to check on the first build.
- **Existing selection tasks break until phase 2 is finished.** After phase 1, the windmill, crossing and controller box no longer counted. After the phase 2 code, the controller box counts again (its buttons are SDK buttons); the windmill and crossing count once their Blueprints get an `IsdkRayInteractable` (editor step 2). The furniture placement task is not affected.
- **Phase 2 also uses Meta's prebuilt rounded button** (`IsdkRoundedButton.h`, module `OculusInteractionPrebuilts`). Its API (`UIsdkRoundedButtonComponent`, `ButtonSize`, `SetLabelText`, `SetLabelSize`, `GetPokeInteractable`) is taken from Meta's Interaction SDK sample, which builds its menu buttons the same way. Creating the buttons at runtime with `NewObject` and `RegisterComponent`, instead of in a constructor as Meta's sample does, is not yet tested.
- **Attempt counts can drop slightly in the placement task.** A product that is placed and selected in the same frame used to count twice; it now counts once.
- **The Interaction SDK is licensed and not committed.** Each developer installs it at `Plugins/MetaXRInteraction/OculusInteraction.uplugin`, as with the Meta XR plugin.
