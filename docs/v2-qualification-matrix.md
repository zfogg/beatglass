# v2 GUI/CLI qualification matrix

The machine-readable matrix in [`v2-qualification-matrix.json`](v2-qualification-matrix.json) links every v2 visual fixture to the native scenario it would need to qualify. `design-only` means a deterministic render or contract exists, but no native desktop/audio/overlay evidence has been retained. It must not be reported as native support.

Evidence is promoted only when a platform-specific qualification bundle records the exact build identity, environment, commands, logs, and screenshot or recording for the mapped scenario. Empty `evidence` arrays are intentional for the current pending state.

Run the portable traceability validator:

```sh
python tests/unit/check_v2_qualification_matrix.py
```
