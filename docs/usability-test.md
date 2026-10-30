# Usability evaluation protocol

Status: prepared for execution; no human study has been conducted or implied by the automated workflow tests.

Use 3–5 participants who have not used the editor. Include at least one person without physics training and one person comfortable with scientific notation. Ask them to think aloud; avoid explaining controls until they are stuck. Obtain their consent before recording. Do not collect personal information in the project repository.

Run the same build, scene and window size for each participant. Record build revision, OS, screen size, input device and any accessibility settings. Start at the graphical chooser.

| Task | Success criterion | Record |
|---|---|---|
| Start Figure Eight and pause it | Chooser and Pause found without coaching | Time, wrong clicks, assistance |
| Select a body and move it above the original plane | Z changes using a handle or labeled field | Route taken, axis confusion |
| Set its Z velocity to 0.125 | Correct body and value; recognizes position vs velocity | Errors, recovery |
| Hide the origin arrows and restore the grid | Finds independent controls | Confusion with handles/orientation widget |
| Change speed without changing numerical dt | Speed changes; participant can explain distinction | Misinterpretations, labels used |
| Display a longer predicted path | Prediction visible; horizon control found | Whether approximation/cap understood |
| Save, change, then reopen a scene | Named file reopened with stored properties | Dialog errors, path confusion |
| Try resetting with unsaved changes | Understands save/discard/cancel decision | Accidental data loss or hesitation |

After each task ask for ease on a 1–7 scale and one concrete improvement. After the session ask what the scene means, which controls were hardest to find, and what they expected to happen differently.

Copy this result template per participant using anonymous identifiers:

```text
Participant ID:
Build / OS / window size / input device:
Task | success unaided? | seconds | wrong turns | assistance | ease (1–7)
Observations (participant's words, not assumptions):
Severity / reproducible issue / suggested change:
```

Prioritize accidental destructive actions and wrong-body edits first, then repeated navigation failures and unclear scientific labels. Keep individual observations separate from conclusions; do not describe 3–5 participants as representative of every user. Re-run affected tasks after changes.
