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
On non-Linux hosts the runner writes a structured pending summary when
`--summary` is provided and exits non-zero with `qualified: false`; this keeps
Windows/macOS handoffs auditable without claiming Linux qualification.
The pending summary still records the source checkout commit, allowing a
later Linux handoff to prove which code was actually qualified.
It labels the target as `Linux/PipeWire` and the handoff as `pending`, so
downstream evidence tooling can distinguish an unrun gate from a failed one.
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
The raw discovery JSON is preserved as `pipewire-tests-discovery.json` for
post-run evidence ingestion.
The summary includes its `native_discovery_sha256` digest for transfer checks.
After copying evidence, verify the complete handoff with
`python tests/pipewire/verify_native_run_summary.py evidence/native-summary.json`.
Then create a transfer-safe archive (the command refuses unqualified summaries,
digest mismatches, symlinked evidence, and accidental overwrites):
`python tests/pipewire/package_native_evidence.py evidence/native-summary.json evidence/native-qualification.zip`.
The archive contains the summary, discovery JSON, JUnit results, and any
captured command stdout/stderr files that exist. Review the ZIP contents before
attaching it to the qualification record; it is the auditable handoff artifact,
not a replacement for native desktop/audio evidence.
It also contains `evidence-manifest.json`, which records the qualified commit,
each archived file's byte size, and its SHA-256 digest for independent transfer
verification.
Archive entries are sorted and use a stable timestamp for reproducible
transfers; set `SOURCE_DATE_EPOCH` to the run's chosen epoch when byte-for-byte
reproducibility is required.
The exported archive can also be verified without the original checkout:
`python tests/pipewire/verify_native_evidence_archive.py evidence/native-qualification.zip`.
This checks the embedded manifest, artifact presence, byte sizes, and SHA-256
digests before the archive is accepted as a transferred handoff. It also
rejects duplicate or traversal-style ZIP entry names so an archive cannot
silently substitute ambiguous evidence files.
Unlisted ZIP entries are also rejected, so the manifest remains the complete
inventory of transferred evidence.
The verifier also cross-checks the manifest commit against the archived summary
commit, preventing evidence from being relabeled under a different build.
The portable behavior fixture can be run with
`python tests/unit/check_pipewire_evidence_archive_verifier_behavior.py`; it
checks a valid archive plus rejection of unlisted, tampered, duplicate-entry,
and traversal-style artifacts. Empty evidence manifests are rejected as well,
so a qualified handoff always contains at least one archived artifact.
The manifest must also carry a non-empty repository commit identity.
It records the `SOURCE_DATE_EPOCH` and UTC archive timestamp used for the ZIP,
so recipients can audit the reproducibility policy without the original host.
Run `python tests/unit/check_pipewire_evidence_packager_behavior.py` to verify
that fixed `SOURCE_DATE_EPOCH` inputs produce byte-for-byte identical archives.
The verifier parses that timestamp as ISO-8601 and rejects malformed metadata.
Qualified archives must include the summary JSON and machine-readable JUnit results; the verifier rejects evidence packages missing those native gate artifacts.
The producer also rejects evidence paths with colliding archive basenames before writing the ZIP.
Qualified archives must include the preserved CTest discovery JSON in addition to summary and JUnit results, proving both registration and execution.
The archive verifier parses those artifacts and requires a discovery `tests` list
and a JUnit `testsuite`/`testsuites` root, catching truncated or mislabeled files.
The discovery JSON must contain at least one registered test; an empty CTest listing is rejected as insufficient native evidence.
Each discovered CTest entry must also have a non-empty name; unnamed registrations are rejected as malformed evidence.
JUnit evidence must report at least one executed testcase; zero-test result documents are rejected.
JUnit evidence with non-zero failures/errors or failure/error testcase elements is rejected, keeping qualified archives failure-free.
Skipped JUnit testcases or non-zero skipped counts are rejected, so pending native scenarios cannot pass the qualification gate.
JUnit archives must include at least one named testcase; unnamed or testcase-free result documents are rejected.
Duplicate JUnit testcase names are rejected so each native result remains uniquely attributable.
CTest discovery entries must also have unique names, preserving an unambiguous native registration inventory.
The discovery and JUnit testcase-name sets must match exactly, tying registered native tests to executed results.
The verifier cross-checks summary `native_test_count` against the discovery inventory count.
ative_test_count against the discovery inventory count.
Qualified handoffs must explicitly record `native_test_name_match: true` in the summary after discovery/execution reconciliation.
ative_test_name_match: true in the summary after discovery/execution reconciliation.
The archived summary must itself report qualified: true; the manifest cannot promote an unqualified run.
The verifier also cross-checks summary `native_junit_test_count` against parsed JUnit testcases.
ative_junit_test_count against parsed JUnit testcases.
Summary JUnit status fields for failures, errors, and skips must all be zero and are cross-checked against parsed results.
The summary must explicitly record `native_junit_exists: true` for qualified handoffs.
ative_junit_exists: true for qualified handoffs.
Qualified summaries must include a non-empty `native_test_discovery` record in addition to the archived discovery JSON.
ative_test_discovery record in addition to the archived discovery JSON.
The summary discovery/JUnit digests are cross-checked against the transferred archive bytes.
If present, summary `native_test_discovery.count` must match the archived discovery inventory count.
ative_test_discovery.count must match the archived discovery inventory count.
Manifest artifact entries must use non-negative sizes and canonical lowercase SHA-256 digests; malformed metadata is rejected explicitly.
Summary artifact paths are checked against archive basenames to keep provenance tied to the transferred files.
Evidence archives use a flat basename layout; nested ZIP paths are rejected during verification.
### Aggregate JUnit integrity

The archive verifier accepts both single-suite and aggregate `testsuites` JUnit
documents. For aggregate output it sums every nested `testsuite` `tests`
attribute and requires that total to equal the serialized `<testcase>` count.
This prevents a qualified archive from masking an incomplete or internally
inconsistent native test report.

Failure, error, and skipped counts are likewise summed across nested suites;
any non-zero aggregate is rejected even when individual `<failure>`, `<error>`,
or `<skipped>` elements are absent.
All aggregate count attributes must also be well-formed, non-negative
integers; malformed or negative status metadata is rejected before the archive
can be marked qualified.
For `testsuites` roots, the declared failure, error, and skipped totals must
also equal the corresponding nested-suite totals.
Each suite’s status total cannot exceed its declared test count.
Archive entries are also checked case-insensitively so extraction remains safe
on Windows and other case-folding filesystems.
Both POSIX and Windows path separators are rejected in entry names; archives
must remain a flat basename-only handoff.
Control characters are rejected from entry names as well, avoiding ambiguous
or truncated extraction paths.
The packager applies the same case-folded collision check before writing the
archive, so invalid evidence is rejected at its source.
It also rejects control characters in source filenames before creating the
archive, keeping packager and verifier policies identical.
