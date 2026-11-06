# Study Strategy

## 1. Purpose

Evaluate how well users complete tasks with one hand tracking interaction method. We measure task performance, time, and how demanding each task feels.

## 2. Study Approach

- Tasks are organized into groups of similar tasks.
- Each participant completes every task group using one interaction method.

## 3. Interaction Being Tested

The study uses one interaction method. Potential interaction methods include the following.

- Pinch grab
- Ray + pinch
- Wholehand grab
- Distance grab

A/B testing is under consideration to determine which implementation works best for users. Participants would be split evenly between two implementations of the interaction. Each participant uses only one implementation and the validator/administrator record which one.

## 4. Participants

Participants ideally include people with little or no VR experience. The study begins with a pilot of 2 or 3 people followed by roughly 8 to 12 participants. For each participant, we collect their age range and prior VR experience.

## 5. Tasks

- Task wording is goal based and describes what the participant should accomplish.

  > "Pick up the cube and place it on the red marker."

- Before each task the test administrator reads a scripted explanation of the gesture used in that task (Script B). Each explanation has the same level of detail.

  > "To pick up an object, pinch your thumb and index finger together on it. Open your fingers to let go."

- Tasks within a group are completed in a fixed order.
- There is no time limit. We record how long each participant needs. A participant may choose to stop a task at any time. The test administrator then ends the task using the validator's controls and it is recorded as not completed.
- **Success tolerances:** starting values for position and rotation accuracy will be set before the pilot and tuned based on its results.

### Task Groups

Example task groups include the following. Some overlap between groups is expected.

- **Controls:** press a button on a panel.
- **Selection:** select nearby and distant objects.
- **Object manipulation:** pick up, move, rotate, and place objects.
- **Menus:** poke the Menu button on the wrist to open the wrist menu and choose an option.

## 6. Test Administrator Conduct

- Ensure that you read each script properly.
- Use neutral wording to avoid participant judgment.
- Remain neutral throughout the session and avoid reacting to successes or failures.
- Do not ask participants to think aloud during tasks. Collect their comments during the debrief instead.
- Offer breaks between tasks.
- Stop the session immediately if the participant feels unwell.
- Stay within reach of the participant at all times.

## 7. Intervention Policy

- The test administrator intervenes only when the participant asks for help.
- The administrator may repeat the task instruction and gesture explanation but gives no other help.
- Every intervention is logged by the validator.
- A task completed after an intervention counts as completed with help. Success rates are reported both with and without these tasks and their times are left out of the time measures since the help affects how long the task took.

## 8. Data Collected

- **Task success:** whether the participant completed the task within the success tolerances. For tasks that involve placing an object the participant must also have released it.
- **Attempts:** how many times the participant grabbed or selected an object before completing the task.
- **Time on task:** how long the participant took from when the test administrator started the task in the validator until completion.
- **Time to first interaction:** how long the participant took to make their first grab or selection after the task was started.
- **NASA TLX:** how demanding the participant felt the task was. Results are recorded with the NASA TLX tool at https://www.keithv.com/software/nasatlx/nasatlx.html. The weighted score is the main measure and the raw score is secondary.
- **Interventions:** how many times the participant asked for help.
- **Observations:** notes from the test administrator and comments from the debrief.

Task success, attempts, time measures, and interventions are recorded by the validator. Its implementation is covered in the [SOFTWARE_SETUP_STRATEGY.md](SOFTWARE_SETUP_STRATEGY.md).

## 9. NASA TLX Procedure

1. **Before the first task:** the administrator introduces the six scales by reading Script C.
2. **After each task:** the administrator reads each scale using Script D and records the participant's rating from 0 to 100 in steps of 5.
3. **After each task group:** the administrator reads the 15 pairwise comparisons in Script E in a shuffled order and records each answer. This happens once per group as the NASA TLX manual recommends.
4. **Entering results:** for each task, the administrator enters that task's six ratings and the comparison answers from its group into the NASA TLX tool. The participant answers the comparisons once per group and the administrator reuses those answers for every task in that group.
5. **Recording:** copy each result into the study spreadsheet (Section 11).

## 10. Session Procedure

- Introduce the TLX scales (Script C).
- For each task group:
  - For each task in the group:
    - Read the gesture explanation (Script B) and the task instruction (Script A).
    - Start the task in the validator which starts the timer. Reading the scripts is therefore not included in the task's time. The participant completes the task.
    - Collect TLX ratings (Script D).
  - Collect the 15 pairwise comparisons for the group (Script E).
- Debrief (Script F).

## 11. Linking the Data

- Every record carries the same three labels: **participant ID**, **task group**, and **task**.
- Participant ID format: `P01`, `P02`, and so on.
- **Validator results:** stored as one CSV file per session.
- **Study spreadsheet columns:**
  - Participant ID, task group, and task
  - Six TLX ratings, six TLX tallies, weighted TLX score, and raw TLX score
  - Administrator notes

## 12. Analysis

- **Descriptive statistics per task and per task group:** success rate, median time on task, median time to first interaction, median attempts, and mean TLX scores.
- **Problem list:** observed usability problems ranked by how often they occurred and how severe they were.

## 13. Reporting

- Structure
  1. Why we tested and how we prepared.
  2. What happened during testing.
  3. Implications and recommendations.
- Include what worked well and not just problems.

## 14. Pilot

Run 2 or 3 people through the full procedure before the study. Check that

- task instructions are clear.
- success tolerances are reasonable.
- the validator logs everything correctly.
- the TLX reading and recording process works smoothly.

Adjust the procedure based on what the pilot reveals and then begin the study.

---

## Scripts

### Script A: Task Instruction

> "In this task, [goal]. Begin when you're ready."

- Example: "In this task, pick up the cube and place it on the red marker. Begin when you're ready."

Each task's instruction is entered in the validator which shows it on the test administrator's screen so it is read word for word.

### Script B: Gesture Explanation

> "To [action], [gesture]. [How to release or finish, if needed.]"

- Example: "To pick up an object, pinch your thumb and index finger together on it. Open your fingers to let go."

Write one explanation per gesture with the same level of detail for each.

### Script C: Introducing the TLX Scales (before the first task)

> "After each task, I'll ask you to rate your experience on six scales. Each scale goes from 0 to 100 in steps of 5. Here's what each one means."
>
> - **Mental Demand:** "How much thinking, deciding, or remembering was required? Was the task easy or demanding, simple or complex?"
> - **Physical Demand:** "How much physical activity was required? Was the task easy or demanding, restful or laborious?"
> - **Temporal Demand:** "How much time pressure did you feel? Was the pace slow and leisurely or rapid and frantic?"
> - **Performance:** "How successful do you think you were in accomplishing the goal? How satisfied were you with your performance?"
> - **Effort:** "How hard did you have to work mentally and physically to accomplish your level of performance?"
> - **Frustration:** "How insecure, discouraged, irritated, or stressed did you feel compared to secure, content, and relaxed?"
>
> "For five of the scales, 0 means low and 100 means high. **Performance is the opposite.** 0 means you performed very well and 100 means you performed poorly. Do you have any questions about the scales?"

### Script D: TLX Ratings (after each task)

> "Now please rate the task you just completed. Use a number from 0 to 100 in steps of 5."
>
> 1. **Mental Demand:** "How mentally demanding was the task?"
> 2. **Physical Demand:** "How physically demanding was the task?"
> 3. **Temporal Demand:** "How much time pressure did you feel?"
> 4. **Performance:** "How successful were you? Remember that 0 means very well and 100 means poorly."
> 5. **Effort:** "How hard did you have to work?"
> 6. **Frustration:** "How insecure, discouraged, irritated, or stressed did you feel?"

If the participant gives a number between steps of 5, round up as the NASA TLX manual directs.

### Script E: Pairwise Comparisons (after each task group)

> "For each pair I read, tell me which one contributed more to the workload of the tasks you just did in this group."

Read the 15 pairs in a shuffled order. Shuffle a printed copy for each participant and group.

1. Effort or Performance
2. Temporal Demand or Frustration
3. Temporal Demand or Effort
4. Physical Demand or Frustration
5. Performance or Frustration
6. Physical Demand or Temporal Demand
7. Physical Demand or Performance
8. Temporal Demand or Mental Demand
9. Frustration or Effort
10. Performance or Mental Demand
11. Performance or Temporal Demand
12. Mental Demand or Effort
13. Mental Demand or Physical Demand
14. Effort or Physical Demand
15. Frustration or Mental Demand

### Script F: Debrief

> "Thank you that's everything. I have a few quick questions before you go."
>
> 1. "Was anything confusing or uncomfortable?"
> 2. "Was there anything that felt easy or natural?"
> 3. "Is there anything else you'd like to share about the experience?"
>
> "Thanks again for participating."