# Validation record — 2026-10-29

Environment: current Linux workstation, SFML 3, OpenGL compatibility renderer, niri/Xwayland. These results describe this environment; other operating systems and SFML 2 compatibility branches were not exercised.

- Release GUI build completed without compiler warnings.
- CTest: all four suites passed (physics, coupled N-body, spatial behavior, product workflow).
- Product tests cover fixed-step equivalence at 30/60/144 FPS, paused/empty clocks, exact stepping, speed/dt separation, stable IDs, rejected property edits, collision selection, Unicode/legacy JSON, failed atomic replacement, and throttled exports.
- Separate graphics-free Release build completed and passed all four suites.
- Real desktop input workflow passed: graphical chooser, output choice, play/pause and stepping, visibility, XYZ fields and blue-axis dragging, invalid-input recovery, speed/dt separation, grid/origin button and shortcut agreement, forecast range, sidebar scrolling, named save, overwrite, explicit saved-file selection, reopen, help and unsaved-change protection.
- Exact 100-step headless Figure Eight run reached simulation time 0.1 and produced JSON/CSV output.
- Installation and CPack TGZ generation completed. Installed executable launched through PATH from `/tmp`, rendered a screenshot and exited normally with no stderr output; its bundled font loaded from the installed share directory.
- Formatting and `git diff --check` passed.
- Project-S2's 39 files in the pre-change SHA-256 manifest were unchanged.

[Desktop editor preview](editor-preview.png) was captured by the workflow after reopening and editing its test scene. Test data lived in temporary folders; imported legacy originals were retained.

Human participant sessions are still pending. Use [the usability protocol](usability-test.md); no human-study results are claimed here. CI configuration has been added; a remote CI run has not been triggered.

## UI layout check — 2026-10-30

The revised text layout was checked in real desktop windows at 760×600, 1280×840 and 1440×900. Startup button text was centered within two pixels of its control centers, and modal control rectangles did not overlap. Compact load/save/help dialogs, long Unicode scene/body names and caret scrolling were inspected in screenshots. The four CTest suites and the existing desktop editing/save/reopen workflow passed. No human participant study is implied by these checks.

## Developer readability refactor — 2026-10-30

- The UI implementation is split into composition, text, widgets, input, sidebar and dialogs; application event translation, scene coordination and diagnostics have their own files. Public classes retain their existing ownership and interfaces.
- A token comparison confirmed the 23 existing UI method implementations were preserved, with UI navigation literals expressed as equivalent named constants. Moved application handlers and the extracted event adapter retain their original logic. Numerical, document, serializer and rendering logic were checked for comment-only changes; Simulator setter argument names and forecast limit names were clarified without changing their values.
- GUI and graphics-free builds passed all four CTest suites. The revised graphical build also rendered a bounded startup screenshot successfully. Documentation links and CMake source registration were checked.
- The separate full desktop automation did not complete reliably on this Xwayland desktop: attempts stopped at pointer positioning or field-commit waits. The precise cause was not established in this refactor. Those failures are not reported as a passing desktop regression run. The existing driver logic is retained; its previous successful UI-layout run is recorded above.
- Nothing was committed or pushed. Remote platform CI has not run for this local refactor.

Start the code review with [the developer guide and file map](architecture.md).

## Pre-push review — 2026-10-30

The project owner confirmed the manual application check passed. Both local builds again passed all four CTest suites. Upload contents and the Project-S3 remote were reviewed; local builds and backups remain ignored. Shell quoting in the GitHub workflow was corrected before upload. Remote CI results are pending.
