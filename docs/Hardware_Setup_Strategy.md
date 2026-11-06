# Hardware Testing Strategy

**Purpose:** every participant completes the study tasks under the same hardware and physical conditions so differences in results reflect the interactions being tested.

## 1. Equipment

| Item              | Choice                                           | Notes                                                        |
| ----------------- | ------------------------------------------------ | ------------------------------------------------------------ |
| Headset           | Meta Quest 2                                     | Use the same unit for every session if possible              |
| PC                | 1080 Lab Machine                                 | A designated machine for all sessions                        |
| Connection        | USB-C cable                                      | To avoid Wifi/Latency issues                                 |
| Backups           | Spare headset                                    | Prevents a hardware failure ending a session                 |
| Accessories       | Glasses spacer, cleaning wipes, spare face cover | For glasses wearers                                          |

## 2. Headset and Software Configuration

- **Firmware:** the same headset software version is used for every participant.
- **Headset settings:** developer mode on & hand tracking on.
- **PC settings:** Meta Horizon Link app is installed and is set as the active OpenXR runtime with Developer Runtime features enabled to allow hand tracking over link.
- **Study version:** the same version of the project is used for all sessions in a study round.

## 3. Testing Area

- **Location:** Viper Lab (subject to change).
- **Testing area:** a clear/marked space next to the study PC so the test administrator can reach the participant.
- **Boundary:** set the headset's boundary to testing area.

## 4. Lighting

- Same room with overhead lights fully on.
- Blinds closed to avoid direct sunlight
- No strong backlighting or mirrors in the testing area.
- If tracking struggles the test administrator may adjust the lighting and notes the change in the session record.

## 5. Participant Paperwork

- **Acknowledgment form:** Covers health and safety, screening questions, equipment care, voluntary participation, and data use.

## 6. Verbal Briefing

A short script read to every participant after they sign the acknowledgment form:

> "For this study, you'll put on a VR headset and complete a few short tasks using only your hands. Before each task, I'll explain what to do and describe the hand motions you'll use. Please stay inside the test area and let me know right away if you feel dizzy, uncomfortable, or unwell. You can stop at any time for any reason. After each task, I'll ask you to rate how demanding it felt."

## 7. Session Flow

1. Acknowledgment form is read and signed (see [Participant Acknowledgment Form](Participant_Acknowledgment_Form.md)).
2. Verbal briefing.
3. Headset fitting.
4. Study tasks and NASA TLX (see [Study Strategy](Study_Strategy.md)).
5. Study debrief.

## 8. Test Administrator Checklist (Each Session)

**Before the participant arrives:**

- [ ] Headset charged, lenses cleaned, and face cover cleaned or swapped
- [ ] USB-C cable connected, Link app running, and Quest Link started in the headset
- [ ] Study level running in VR Preview with the participant's ID set in the validator
- [ ] Testing area clear with lighting set to the baseline
- [ ] Acknowledgment form and briefing script ready

**During:**

- [ ] Headset view mirrored on the PC monitor so the test administrator can follow along with the validator's overlay visible
- [ ] Hand tracking verified before the first task (i.e. both hands visible and tracking in the headset)

**After:**

- [ ] Headset wiped down
- [ ] Validator results and session record is saved. This includes date, participant ID, headset unit, firmware version, PC, lighting notes, and any additional issues