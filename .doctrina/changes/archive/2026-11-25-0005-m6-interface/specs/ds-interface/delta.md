# Spec Delta — capability: ds-interface

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/ds-interface/spec.md`

---

```ops
set-header Implementation: implemented — screens, keyboard, streaming, cancel and settings (`src/ui/`, `src/main.c`); verified on melonDS, real R4 run is M4
bump-version minor
set-criterion 1: verified
set-criterion 2: verified
append-requirement ubiquitous: The interface shall make every action reachable both by a DS button and by a touch target on the bottom screen.
append-requirement ubiquitous: The interface shall wrap text on both screens at word boundaries, breaking a word only when it is longer than a line.
append-requirement event: When the user presses L or R on the prompt screen, the interface shall replace the prompt with the previous or next example prompt.
append-requirement event: When the user saves the settings screen, the interface shall write the settings to /dslm/dslm.ini and apply them to the next generation.
append-criterion [verified] Every documented action works by button and by touch on melonDS — verified by `tests/hw/ui-navigation.md`
append-criterion [verified] Wrapped lines never exceed the width and keep words whole — verified by `tests/host/test_textview.c`
append-criterion [verified] Stopping generation keeps the tokens produced so far — verified by `tools/tests/test_engine_host.py`
```
