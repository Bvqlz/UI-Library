# Design Doc: Object Lifecycle (Add and Remove Objects)

## 1. Introduction

### 1.1 Purpose

This doc designs the **Object Lifecycle** feature module (`HTUIObjects`): how an object enters the room from a catalog, and how it leaves the room again.

### 1.2 Stories

**Story 1: Add and position objects.** As a user, I want to browse the available objects and add one to my room, so I can plan how the space will look.

- I can identify and select an available object.
- I can position it in the room and tell whether the placement is valid.
- I can adjust its placement after adding it.

**Story 4: Remove an object.** As a user, I want to remove an object from the room, so I can correct mistakes or change my design.

- I can remove a selected object.
- I have a way to recover from an accidental removal.

### 1.3 Scope

**In scope:**

- Browsing a catalog and choosing an object
- Dragging a preview into place, with live valid/invalid feedback
- Confirming or cancelling a placement
- Removing the selected object, with confirmation
- Undoing a placement or a removal
- The events these flows publish

**Out of scope:**

- Catalog contents and product models
- The rules that decide whether a placement is valid
- Moving, rotating or resizing an object once it's placed
- Saving the room

## 2. Constraints, Assumptions and Open Questions

### 2.1 Constraints

- The module depends only on the library core.
- The application supplies the catalog and decides whether a placement is valid; the module holds no application types.

### 2.2 Assumptions

- Users are mostly surgeons new to VR, so being easy to discover and hard to get wrong matters more than speed.
- One person edits the room at a time.
- Every catalog entry has an id, a display name, a thumbnail and an actor class.

### 2.3 Open questions

- [ ] Should the user drag an entry straight out of the catalog, instead of a preview appearing in the room first?
- [ ] Should the Confirm/Cancel panel sit near the object instead of in front of the user?
- [ ] Should the undo history live in the core or in this module? (Section 3.5)
- [ ] Is Redo needed?

## 3. Approach

### 3.1 The application owns the catalog and the rules

The module never decides what an object is or where it may go. The application implements a small host interface: it lists the catalog, checks a proposed placement, and spawns, removes and restores objects. A different app supplies its own catalog and rules with no library changes.

The interface keeps *checking* separate from *spawning*. `CheckPlacement` has no side effects and runs while the user drags, so feedback is live. `SpawnObject` runs once, on confirm, and can still fail if the room changed since the last check.

For example, An application rejects a placement that overlaps a wall, with the reason "Overlaps the wall." A showroom app might accept anything on the floor. The module behaves the same in both.

### 3.2 Drag a preview, then confirm on a separate panel

Choosing a catalog entry creates a **preview**: a ghost of the object where the hand's aim ray meets the floor, or 1.5 m in front of the user if it meets nothing. The user pinches the preview and drags it; while the pinch is held, the preview sits where the aim ray meets a permitted surface, so far spots are as easy to reach as near ones. Releasing leaves the preview in place, and the user can drag it again as often as they like.

A small **Confirm/Cancel panel** in front of the user holds a green check and a red X. Confirm places the object and is disabled while the placement is invalid. Cancel discards the preview and creates nothing.

Choosing, dragging and confirming are three separate intents, so neither choosing an entry nor dragging can place an object by accident. The cost is one extra step per placement.

### 3.3 Select first, then act

A newly placed object is **not** selected automatically. It behaves like every other object in the room: the user selects it, then acts on it. Remove follows the same rule and acts on the currently selected object, which the module tracks from the selection event. Auto-selecting would save a step when the user wants to adjust right away, but would make new objects behave differently from the rest. This is another step to prevent accidental manipulations, though it too comes at the cost of speed and potential convenience.

### 3.4 Removal needs a confirmation

Remove sits in the selected object's context menu and always asks for confirmation before anything is deleted. No single stray poke removes an object, and cancelling leaves it untouched.

### 3.5 Undo through a shared history

**Undo** reverses the last completed action of any kind: place, move, rotate, resize or remove. Undoing a placement removes the object; undoing a removal restores it with the same id and transform. Cancelled previews and cancelled removals never enter the history.

Because the last action can come from any feature module, the history can't live in one of them. This design proposes a small history subsystem in the core: each module pushes one command per completed action, and the command knows how to reverse itself. The core only orders commands; each module owns what its commands do. A separate stack per module was considered, but it can't tell which module made the last change.

## 4. Design

### 4.1 Components

- **Object lifecycle component** (on the user's pawn): runs the placement and removal flows, calls the host, pushes undo commands and publishes events. It is the only part with logic.
- **Host interface** (`IHTUIObjectHost`, implemented by the application): catalog, placement checks, spawn, remove, restore.
- **Presenters:** the catalog panel (title and image per entry), the preview ghost (a valid look, an invalid look, and the host's reason as text), and the Confirm/Cancel panel shared by placement and removal. They hold no logic, so an app can restyle them.
- **Remove action:** an item in the selected object's context menu that fires `Object.Remove`.

The catalog panel opens from the wrist toolkit's Menu and settles as a stable panel within comfortable reach, so browsing a long list doesn't need a raised arm. Entries are pressed by poke, or by ray and pinch when the panel is out of reach.

Placement moves through the states below. Leaving the preview any way other than a valid confirm returns to idle and creates nothing.

&#91;embedded content: placement states · 5 states, one way in\]

Removal moves through the states below. Only a completed Confirm removes the object; Undo brings it back.

&#91;embedded content: remove and undo states · 4 states\]

### 4.2 Intents consumed

| Intent | What it does |
| --- | --- |
| `Object.OpenCatalog` | Opens the catalog panel |
| `Object.ChooseEntry` | Chooses the entry under the finger or ray; starts a preview |
| `Object.DragPreview` | Started: the preview follows the aim ray. Completed: it stays. Cancelled: it returns to where the drag began |
| `Object.ConfirmPlacement` | Places the object, if the placement is valid |
| `Object.CancelPlacement` | Discards the preview; creates nothing |
| `Object.Remove` | Asks to remove the selected object; shows the Confirm/Cancel panel |
| `Object.ConfirmRemove` / `Object.CancelRemove` | Removes the object, or keeps it |

Two input contexts take priority while they're active:

- **Placement context** (from choosing an entry until confirm or cancel): pinch drags the preview, and the panel's buttons confirm or cancel.
- **Remove-confirm context** (while the remove panel shows): only the panel's buttons are live, so a stray pinch can't select another object mid-confirmation.

Suggested gestures for mapping A:

| Intent | Mapping A |
| --- | --- |
| `Object.OpenCatalog` | Wrist toolkit Menu, then Products (poke) |
| `Object.ChooseEntry` | Poke on a catalog entry |
| `Object.DragPreview` | Pinch |
| `Object.ConfirmPlacement` / `Object.ConfirmRemove` | Poke on the panel's check |
| `Object.CancelPlacement` / `Object.CancelRemove` | Poke on the panel's X |
| `Object.Remove` | Poke on Remove in the object's context menu |

The catalog panel tracks which entry is under the finger or ray, and the component reads it when `Object.ChooseEntry` completes.

### 4.3 Events published

| Event (after `HTUI.Event.Object.`) | When | Key payload fields |
| --- | --- | --- |
| `CatalogOpened` / `CatalogClosed` | The catalog panel opens or closes | Hand |
| `PreviewStarted` | An entry is chosen | Request ID, catalog ID, start transform |
| `PreviewDragged` | A drag ends (completed or cancelled) | Request ID, transform, valid, reason |
| `PlacementValidityChanged` | The preview turns valid or invalid | Request ID, valid, reason |
| `Placed` | The host spawned the object | Request ID, object, object ID, catalog ID, placement, valid |
| `PlacementCancelled` | The user cancels | Request ID, catalog ID |
| `PlacementRejected` | The host refuses at commit | Request ID, catalog ID, reason |
| `RemoveRequested` / `RemoveCancelled` | Remove is pressed, or its confirmation cancelled | Request ID, object ID |
| `Removed` | The host removed the object | Request ID, object ID, catalog ID, last transform |
| `Restored` | Undo brought a removed object back | Object ID, transform |

The request ID ties every event of one placement or removal together, so a whole attempt can be measured from `PreviewStarted` to `Placed`, including its drags and invalid spots. `Placed` carries the final transform, so placement error can be computed.

### 4.4 Configuration

One settings asset; every value has a default, so the module works without one. Values are starting proposals to tune through testing.

| Setting | Default |
| --- | --- |
| Preview start distance when the aim ray hits nothing | 1.5 m |
| Maximum placement distance along the aim ray | 10 m |
| Re-check placement when the preview moves at least | 1 cm or 1 degree, once per frame at most |
| Confirm panel position | 0.45 m in front of the user, 15 cm below eye height |
| Valid / invalid look | Green outline / red outline with the reason as text |
| Undo history depth | 20 actions |

## 5. Examples

These use the suggested mapping A from Section 4.2.

### 5.1 Placing a cart beside the table

1. The user opens the wrist toolkit's Menu and pokes Products. The catalog panel opens and `CatalogOpened` is broadcast.
2. They poke the "Instrument cart" entry. A preview appears where their aim ray meets the floor, and the Confirm/Cancel panel appears in front of them. `PreviewStarted` is broadcast.
3. They pinch the preview and point at the floor beside the operating table. The preview follows the ray; each time it moves far enough, the module calls `CheckPlacement`. The host reports valid, so the preview shows green.
4. They release. The preview stays, and `PreviewDragged` is broadcast.
5. They poke the green check. The host spawns the cart and returns its id; the module pushes a place command into the undo history and broadcasts `Placed`.
6. The cart is in the room, not selected. Selecting it works like any other object.

### 5.2 Dragging into a wall

1. While dragging, the user points at a spot where the cart would overlap the wall.
2. `CheckPlacement` returns invalid with the reason "Overlaps the wall." The preview turns red and shows the reason; `PlacementValidityChanged` is broadcast.
3. The panel's check is disabled, so poking it does nothing.
4. They drag the preview back into open floor; it turns green and the check is enabled again.

### 5.3 Losing the hand mid-drag

1. The user is dragging the preview when their hand moves out of the headset's view.
2. The drag arrives as cancelled, so the preview returns to where this drag began.
3. Nothing is placed and the placement stays open. When the hand returns, they can drag again, confirm or cancel.

### 5.4 The host refuses at commit

1. The preview showed valid, but the room changed before the user confirmed.
2. On confirm, `SpawnObject` fails with "Space is occupied."
3. The module broadcasts `PlacementRejected`, shows the reason on the preview, and keeps the placement open. Nothing enters the history.

### 5.5 Removing a cart, then undoing

1. The user selects the cart, opens its context menu and pokes Remove. The panel asks "Remove Instrument cart?" and `RemoveRequested` is broadcast.
2. They poke the check. The host removes the cart and returns its saved state (id, catalog id, transform). The module pushes a remove command and broadcasts `Removed`.
3. They realize it was a mistake and press Undo. The remove command asks the host to restore the cart, which returns with the same id and transform. `Restored` is broadcast.

### 5.6 Undoing a removal when the spot is now taken

1. The user removed a cart, then placed a table where it stood.
2. They press Undo. The latest command is the table's placement, so the table is removed.
3. They press Undo again. The cart's spot is free, so the cart is restored.

If the spot were still occupied, the remove command checks the placement first. When the check fails, it restores nothing, shows the host's reason, and stays at the top of the history, so the user can clear the spot and try again.

### 5.7 The same item placed twice

1. The user places two instrument carts from the same catalog entry.
2. The host gives each its own object id on spawn; both share the catalog id.
3. Removal, undo and every event use the object id, so the two carts are never confused.

## 6. API

Sketches only; names use the placeholder `HTUI` prefix.

### 6.1 Host interface

```cpp
USTRUCT() struct FHTUICatalogEntry
{
    GENERATED_BODY()
    UPROPERTY() FName CatalogId;
    UPROPERTY() FText DisplayName;
    UPROPERTY() TSoftObjectPtr<UTexture2D> Thumbnail;
    UPROPERTY() TSoftClassPtr<AActor> ActorClass;
    UPROPERTY() FRotator DefaultRotation;
};

USTRUCT() struct FHTUIPlacementCheck
{
    GENERATED_BODY()
    UPROPERTY() bool bValid = false;
    UPROPERTY() FText Reason;          // shown on the preview when invalid
};

// Enough to bring a removed object back exactly.
USTRUCT() struct FHTUIRemovedObjectState
{
    GENERATED_BODY()
    UPROPERTY() FGuid ObjectId;
    UPROPERTY() FName CatalogId;
    UPROPERTY() FTransform Transform;
    UPROPERTY() TMap<FName, FString> HostData;   // anything else the host needs
};

// Implemented by the application.
UINTERFACE() class UHTUIObjectHost : public UInterface { GENERATED_BODY() };
class HTUIOBJECTS_API IHTUIObjectHost
{
    GENERATED_BODY()
public:
    virtual TArray<FHTUICatalogEntry> GetCatalog() const = 0;

    // No side effects; called while dragging.
    virtual FHTUIPlacementCheck CheckPlacement(FName CatalogId, const FTransform& Proposed) const = 0;

    // Called once on confirm; may still fail. Assigns a unique ObjectId.
    virtual FHTUIPlacementCheck SpawnObject(FName CatalogId, const FTransform& Final,
                                            FGuid& OutObjectId, AActor*& OutActor) = 0;

    virtual bool RemoveObject(FGuid ObjectId, FHTUIRemovedObjectState& OutState) = 0;
    virtual FHTUIPlacementCheck RestoreObject(const FHTUIRemovedObjectState& State) = 0;
};
```

### 6.2 Lifecycle component

```cpp
UENUM() enum class EHTUILifecycleState : uint8
{
    Idle, Browsing, Previewing, Dragging, ConfirmingRemove
};

// Added to the user's pawn.
UCLASS() class HTUIOBJECTS_API UHTUIObjectLifecycleComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void SetHost(TScriptInterface<IHTUIObjectHost> Host);
    void SetSettings(UHTUIObjectSettings* Settings);

    // Requests from the application; intents arrive through the core.
    void OpenCatalog();
    FGuid BeginPlacement(FName CatalogId);      // returns the request ID
    void ConfirmPlacement();
    void CancelPlacement();
    FGuid RequestRemove(FGuid ObjectId);

    EHTUILifecycleState GetState() const;

private:
    void OnIntent(const FHTUIIntentEvent& Intent);   // ignores Cancelled outcomes for confirm/remove
    void OnSelectionChanged(FGameplayTag Channel, const FHTUISelectionChangedMessage& Msg);
    TWeakObjectPtr<AActor> Selected;                  // kept from the selection event
};
```

The application can drive placement directly through these functions, for example a desktop "add cart" button calling `BeginPlacement("Cart")`, without any gesture.

### 6.3 Event payloads

```cpp
USTRUCT() struct FObjectPlacedMessage
{
    GENERATED_BODY()
    UPROPERTY() FHTUIEventHeader Header;
    UPROPERTY() TObjectPtr<AActor> Object;
    UPROPERTY() FGuid ObjectId;
    UPROPERTY() FName CatalogId;
    UPROPERTY() FTransform Placement;
    UPROPERTY() bool bValidPlacement = true;
};

USTRUCT() struct FObjectRemovedMessage
{
    GENERATED_BODY()
    UPROPERTY() FHTUIEventHeader Header;
    UPROPERTY() FGuid ObjectId;
    UPROPERTY() FName CatalogId;
    UPROPERTY() FTransform LastTransform;
};

// PreviewStarted, PreviewDragged, PlacementValidityChanged, PlacementCancelled,
// PlacementRejected, RemoveRequested, RemoveCancelled, Restored follow the same pattern.
```

### 6.4 Undo history (proposed core addition)

```cpp
// One command per completed action. Pushed by the module that did it.
class HTUICORE_API IHTUICommand
{
public:
    virtual ~IHTUICommand() = default;
    virtual FText GetLabel() const = 0;                 // "Place Instrument cart"
    virtual bool Undo(FText& OutReason) = 0;             // false = refused; stays on top
};

UCLASS() class HTUICORE_API UHTUIHistorySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void Push(TUniquePtr<IHTUICommand> Command);
    bool CanUndo() const;
    bool Undo();                    // bound to History.Undo; broadcasts History.Undone or UndoRefused
    void Clear();
};
```

This module's two commands: undoing a place calls `RemoveObject`; undoing a remove calls `CheckPlacement`, then `RestoreObject`.

## 7. Epic and User Stories

**Epic: Object Lifecycle.** As a user, I want to add objects to my room and remove them again, so that I can build and correct my design without help from someone else.

| Story criterion | Covered by |
| --- | --- |
| S1: I can identify and select an available object | OBJ-2 |
| S1: I can position it and tell whether the placement is valid | OBJ-3, OBJ-4 |
| S1: I can adjust its placement after adding it | OBJ-6 |
| S4: I can remove a selected object | OBJ-5 |
| S4: I can recover from an accidental removal | OBJ-7 |

**OBJ-1: Host interface and sample catalog.** As a developer, I want to supply my own catalog and placement rules, so that the module works in my app without library changes.

- [ ] Given a host that implements `IHTUIObjectHost`, then the module reads its catalog and calls only that interface for checks, spawns and removals.
- [ ] The sample app's host provides three catalog entries and rejects placements outside the room or overlapping another object.
- [ ] No file in `HTUIObjects` includes any specific application header.

**OBJ-2: Browse and choose from the catalog.** As a user, I want to see what I can add, so that I can pick the right object.

- [ ] Given `Object.OpenCatalog`, then a panel shows every catalog entry with its title and image, and `CatalogOpened` is broadcast.
- [ ] Given `Object.ChooseEntry` on an entry, then a preview of that object appears and `PreviewStarted` is broadcast.
- [ ] Choosing an entry never places anything.

**OBJ-3: Drag the preview with live feedback.** As a user, I want to see whether a spot works while I move the object, so that I can find a valid place.

- [ ] Given a pinch on the preview, then it follows the aim ray on permitted surfaces until release.
- [ ] Given the host reports a spot invalid, then the preview shows red with the host's reason; valid shows green. Both use an outline and text, not colour alone.
- [ ] Given a cancelled drag, then the preview returns to where that drag began.

**OBJ-4: Confirm or cancel a placement.** As a user, I want to decide when the object is placed, so that I don't place it by accident.

- [ ] Given a valid preview, when `Object.ConfirmPlacement` completes, then the host spawns the object and `Placed` is broadcast with its final transform.
- [ ] Given an invalid preview, then Confirm is disabled and nothing is placed.
- [ ] Given `Object.CancelPlacement`, then the preview disappears, nothing is created, and nothing enters the history.
- [ ] Given the host refuses at commit, then `PlacementRejected` is broadcast and the preview stays.

**OBJ-5: Remove the selected object.** As a user, I want to remove an object I no longer need, so that I can correct my design.

- [ ] Given a selected object, when `Object.Remove` completes, then a confirmation panel names the object.
- [ ] Given `Object.ConfirmRemove`, then the host removes it and `Removed` is broadcast; given `Object.CancelRemove` or a cancelled press, then nothing changes.
- [ ] With nothing selected, Remove is unavailable.

**OBJ-6: Adjust a placement.** As a user, I want to adjust where an object goes after adding it, so that I can fine-tune the room.

- [ ] Given an open preview, then the user can drag it again any number of times before confirming.
- [ ] Given a just-placed object, then it is not selected, and the user can select it like any other object.

**OBJ-7: Undo a placement or a removal.** As a user, I want to take back my last change, so that a mistake is easy to fix.

- [ ] Given a completed placement, when `History.Undo` completes, then the object is removed.
- [ ] Given a completed removal, when `History.Undo` completes, then the object returns with the same id and transform, and `Restored` is broadcast.
- [ ] Given the original spot is now invalid, then nothing is restored, the reason is shown, and the command stays in the history.
- [ ] Cancelled previews and cancelled removals never enter the history.

## 8. Testing

Most of the module can be tested without a headset, with injected intents and a test double for the host.

**Unit tests** (logic in isolation, with a fake host):

- State machine: every exit except a valid confirm creates nothing (OBJ-4)
- Placement checks: called only when the preview moves past the threshold; invalid disables Confirm (OBJ-3)
- Host refusal at commit: no object, no history entry, `PlacementRejected` (OBJ-4)
- History commands: place then undo removes; remove then undo restores the same id and transform; a refused restore stays on top (OBJ-7)
- Unique ids: the same catalog entry placed twice yields two object ids (OBJ-1)

**Integration tests** (a test level, injected intents):

- Choose, drag, confirm: exactly one object exists, and `PreviewStarted`, `PreviewDragged` and `Placed` share one request ID
- Choose, then cancel: no object, no history
- A started then cancelled drag: the preview returns to the drag's start
- A cancelled press on Confirm or ConfirmRemove: nothing placed, nothing removed
- Remove, confirm, undo: the object returns with its id and transform
- The events of one attempt are enough to compute its time, drag count and placement error

**Build checks** (CI):

- `HTUIObjects` builds with only `HTUICore` as a library dependency

**In-headset checks:**

- Can a first-time user find, place and remove an object without help?
- Is an invalid spot obvious, and is its reason readable at the maximum distance?
- Is the Confirm/Cancel panel easy to reach when the object is far away?
- Is Remove hard to trigger by accident?
- Does not auto-selecting a new object feel like a wasted step?
- Losing the hand during a drag and during remove confirmation

## 9. Risks

| Risk | Impact | Mitigation |
| --- | --- | --- |
| The undo history isn't added to the core | Undo can't cover other modules' actions | The history only orders commands, so a copy inside this module can still cover place and remove |
| The host only reports errors on spawn | No live feedback while dragging | The interface requires a side-effect-free `CheckPlacement` |
| `CheckPlacement` is slow in a large room | The preview lags the hand | Re-check only past a movement threshold, at most once per frame |
| The Confirm panel adds a step | Slower placement | Measured in the headset; the panel can be moved or restyled without logic changes |
| Long catalogs tire the arm | Discomfort, slower browsing | The catalog settles as a stable panel within reach, not hand-anchored |
