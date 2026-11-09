# PipeWire qualification JSON summary

The artifact validator's `--json` mode (invoke with `validate_qualification_artifacts.py ARTIFACT_DIR --json`) emits a stable object with
`verified: true` and a `files` map. Each required evidence filename maps to
its byte count. The summary also includes `artifact_dir`, identifying the
retained evidence directory. Consumers should treat any nonzero validator exit as an
incomplete run and must retain the original evidence directory alongside the
summary; the JSON is an index, not a replacement for native logs/traces.
