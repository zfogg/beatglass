# PipeWire Linux qualification runbook

Run from a Linux checkout with the PipeWire development package and a live
user daemon:

```sh
pkg-config --modversion libpipewire-0.3
cmake -S . -B build-pipewire -DBEATGLASS_BUILD_PIPEWIRE=ON
cmake --build build-pipewire
ctest --test-dir build-pipewire --output-on-failure -R pipewire
# The runner also invokes the daemon-backed pipewire_tests target explicitly.
# Or run the complete configure/build/CTest pass with a JSON summary:
python tests/pipewire/run_linux_native_qualification.py --summary evidence/native-summary.json
# Optional machine-readable artifact summary:
python tests/pipewire/validate_qualification_artifacts.py <artifact-dir> --json
```

Record the commit, compiler, PipeWire version, sink serial, negotiated format,
and daemon status in the evidence template. Exercise discovery, stream start,
process delivery, callback error, stop/quiesce, restart, and source
reacquisition. Attach logs or recordings for each case; headless CI does not
replace native desktop/audio qualification.

The runner JSON records native_pipewire_tests separately; qualification requires that target to return zero.
It also records the manifest capture command's stdout/stderr alongside the
native command logs.
The summary records the repository commit and `libpipewire-0.3` version used
for the run.
After capture, the runner validates the manifest before marking the run
qualified; capture or ingestion failure therefore fails the native gate.
The explicit native CTest invocation uses `--no-tests=error`, so a build that
omits `pipewire_tests` cannot report a false green qualification.
It also applies a 30-second per-test timeout to bound stalled stream startup
or callback delivery while preserving captured failure logs.
Before execution, the runner captures CTest's JSON test listing to prove the
native target registered concrete cases.
The summary exposes that listing as `native_test_discovery`, separately from
the `native_pipewire_tests` execution result.
The runner also records the parsed `native_test_count` from CTest's JSON
`tests` array.
Qualification additionally requires `native_test_count` to be positive.
The summary exposes the prerequisite/daemon result separately as
`daemon_preflight`.
When a stage fails, `failed_command` identifies the first failing command.
The native test run also writes `pipewire-tests.xml` via CTest's JUnit output
for machine-readable ingestion of individual stream cases.
The runner records `native_junit_exists` and requires it for qualification.
Qualification also requires zero skipped testcases; skipped native stream cases
remain explicitly pending rather than being treated as success.
The summary includes `native_junit_sha256` so the archived JUnit evidence can
be verified after transfer.
It also records `native_junit_tests`, the concrete testcase names executed.
Qualification requires the discovered and executed testcase-name sets to match.
