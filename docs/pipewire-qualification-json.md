# PipeWire qualification JSON summary

The artifact validator's `--json` mode (invoke with `validate_qualification_artifacts.py ARTIFACT_DIR --json`) emits a stable object with
`schema: pipewire-qualification-v1`, `verified: true`, and a `files` map. Each required evidence filename maps to
its byte count. The summary also includes `artifact_dir`, identifying the
retained evidence directory. Consumers should treat any nonzero validator exit as an
incomplete run and must retain the original evidence directory alongside the
summary; the JSON is an index, not a replacement for native logs/traces.

Exit code `0` means every required file is present and non-empty; `1` means
evidence is incomplete or unsafe; `2` means the artifact directory or command
line is invalid.
# Process sample-count integrity

When a transition artifact includes `sample_count` on any `process` event, every
`process` event must include a non-negative integer `sample_count`. This prevents
partial callback accounting from being accepted as native evidence. The artifact
must still contain positive captured samples in the active epoch.

If any event supplies `epoch`, every event must supply a positive integer epoch.
Partial epoch annotations are rejected so lifecycle evidence cannot silently mix
attributed and unattributed records.

Process events may also include `serial`. If present, it must be a non-empty string
on every process event and must match the sink serial for that event's capture epoch:
the start serial before restart and the restart serial afterward. This preserves
device-switch ordering in retained native evidence.
