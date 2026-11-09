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
It also records requested summary/archive paths and an empty native artifact
list, making the host limitation explicit without claiming qualification.
The pending record includes the host OS and the exact runner command that must
be rerun on Linux, making the handoff actionable rather than merely advisory.
Generate the cross-platform gate scaffold with
`python tests/pipewire/qualification_matrix.py evidence/qualification-matrix.json`;
it records Linux, Windows, and macOS targets, status, and pending reasons while
native evidence is collected on each host.
Pass `--summary evidence/native-summary.json` to merge a runner summary's
qualified status and artifact paths into the host's matrix entry.
The matrix behavior fixture validates this merge, including pending reasons and
evidence paths, with `python tests/unit/check_pipewire_qualification_matrix_behavior.py`.
It covers both pending and qualified host summaries so the report can advance
as each native machine supplies evidence.
The behavior test also requires all three platform entries to remain present
and every pending platform to carry an actionable reason.
Validate a transferred matrix independently with
`python tests/pipewire/validate_qualification_matrix.py evidence/qualification-matrix.json`
before treating its cross-platform status as an actionable handoff.
Render a reviewable report with
`python tests/pipewire/render_qualification_matrix_report.py evidence/qualification-matrix.json evidence/qualification-matrix.md`.
Merge per-host evidence as it arrives with
`python tests/pipewire/merge_qualification_matrices.py evidence/qualification-matrix.json linux.json windows.json macos.json`;
the ingestion tool rejects conflicting platform entries and incomplete reports.
Validate the matrix and rendered report together with
`python tests/pipewire/validate_qualification_bundle.py evidence/qualification-matrix.json evidence/qualification-matrix.md`;
this keeps portable status and its reviewer-facing rendering synchronized.
Bundle validation also requires a supported host identity and, when present,
non-empty source commit provenance before the handoff is accepted.
The rendered report must repeat the matrix host identity, preventing a report
from being paired with evidence from another machine.
When commit provenance is available, the report must repeat that identity too;
native evidence is still explicitly pending until a host supplies a run.
Pending platform reasons must also appear in the rendered report, so reviewers
can act on the missing-host requirement without opening the raw JSON.
Qualified platform evidence paths must be visible in the report as well, so a
qualified status always points reviewers to concrete handoff artifacts.
Prepare an explicit, non-fabricating host capture plan with
`python tests/pipewire/prepare_native_host_capture.py evidence/native-host-capture-plan.json`;
it lists the Linux, Windows, and macOS commands/permissions still required.
Validate that plan before distributing it with
`python tests/pipewire/validate_native_host_capture_plan.py evidence/native-host-capture-plan.json`;
the validator refuses fabricated, incomplete, or non-pending host entries.
Pass that plan to the runner with `--capture-plan` so native execution refuses
to start from an invalid or fabricated host handoff.
Pending summaries preserve the same `--capture-plan` argument in their rerun
command, so the native host receives the exact validated handoff request.
They also record the capture-plan SHA-256 when the file is available, allowing
the native host to prove it used the intended preparation instructions.
Summaries explicitly record whether that plan passed validation before the
runner proceeded.
The pending-runner behavior fixture executes this Windows-safe path and asserts
that it exits unqualified while preserving the validated-plan provenance.
It also verifies invalid capture plans are rejected before any native command
can run.
The validator behavior fixture covers both an actionable mixed-status matrix
and rejection of incomplete platform coverage.
The report behavior fixture checks that qualified evidence and pending reasons
are visible in the generated Markdown handoff.
It also proves malformed matrix schemas are refused before report generation.
It also refuses to mark a platform qualified unless at least one evidence
artifact is listed.
Evidence entries must be non-empty strings, preventing malformed paths from
being presented as transferable qualification artifacts.
Control characters are rejected from evidence paths as well, keeping report
entries visible and safe to transfer between hosts.
Duplicate evidence paths are rejected so each listed artifact contributes a
distinct handoff record.
When a runner summary supplies a commit, the matrix preserves and validates
that non-empty provenance identity for later cross-host reconciliation.
The matrix host identity is likewise required to be a non-empty string before
the report is accepted.
Only the supported Linux, Windows, and macOS host identities are accepted.
Runner summaries carry the same matrix schema and a per-host matrix status,
so archived Linux results can be merged into the cross-platform report without
guessing whether a host was pending or qualified.
It also records the runner Python version and working directory for reproducible
handoff diagnostics.
It labels the target as `Linux/PipeWire` and the handoff as `pending`, so
downstream evidence tooling can distinguish an unrun gate from a failed one.
Linux runs use the same schema with `handoff_status: complete` only when the
qualification predicate passes, and `failed` otherwise.
The completed-summary verifier enforces that schema and requires a non-empty
commit identity before accepting discovery/JUnit digests.
When a transition artifact is supplied, it also verifies its digest and runs
the transition validator before accepting the completed summary.
The completed-summary verifier also cross-checks the recorded transition event
count against the captured envelope.
It binds transition platform and commit provenance to the completed summary as
well as to the archive manifest.
Completed summaries carrying transition evidence must identify a Linux host;
non-Linux summaries remain pending handoffs and cannot pass this verifier.
The native owner records whether its thread loop actually started, so early
PipeWire construction failures can tear down safely without locking an
unstarted loop; native Linux qualification must exercise these failure paths.
The portable transport contract also asserts that callback admission is
checked before FIFO insertion, preserving the stop/error boundary during
reacquisition.
Portable lifecycle coverage now exercises callback failure, discontinuity
publication, rejection of same-epoch restart, and stale-epoch suppression;
native Linux qualification must additionally reproduce these transitions with
the real PipeWire daemon.
Registry enumeration de-duplicates repeated globals by object serial before a
device switch is offered to the coordinator; native qualification should
exercise duplicate-global and remove/re-add scenarios.
When duplicate globals coexist, the newest registry id wins so updated node
metadata is used immediately rather than retaining stale device labels.
Discovery failures preserve the native PipeWire error code alongside the
diagnostic string; permission/connection recovery logic must use the code,
not parse human-readable text.
The shared discovery contract marks permission, daemon-connection, timeout,
and broken-pipe errors retryable while treating invalid requests as terminal;
qualification should verify both branches.
The monitor backend consumes that classification by retrying one discovery
attempt within the caller’s original timeout budget, allowing a PipeWire user
daemon or permission session to recover without unbounded blocking.
For native transition evidence, capture one JSON object per lifecycle event and
validate it before packaging:

```sh
python tests/pipewire/validate_pipewire_transition_evidence.py evidence/pipewire-transitions.json
# Include the validated artifact in the native summary/archive:
python tests/pipewire/run_linux_native_qualification.py --transitions evidence/pipewire-transitions.json --summary evidence/native-summary.json
```

The validator requires `start`, `process`, `error`, and `restart` events,
positive monotonic epochs, a sink serial, captured samples, a negotiated
restart format, a discontinuity marker, and a native error code.
Every event must be a JSON object; malformed event shapes are rejected before
the artifact can be packaged.
Each event must also carry a non-empty event name so lifecycle evidence remains
auditable when new event kinds are added.
The required lifecycle sequence is ordered as start, process, error, then
restart; out-of-order evidence is rejected.
The restart event must carry an epoch greater than the initial start epoch,
proving that reacquisition crossed a real stream boundary.
The error event itself must carry the discontinuity marker and a nonzero native
error code; unrelated event metadata cannot satisfy those requirements.
At least one process event with captured samples must carry the initial start
epoch, proving samples were delivered before the failure boundary.
The restart event must carry the selected sink serial and negotiated format so
device-switch evidence identifies the post-recovery target.
When present in a packaged summary, the archive verifier rechecks the same
required lifecycle events and epoch/discontinuity/error invariants.
It also requires the transition envelope’s schema, source commit, and host
platform, preventing raw or detached event lists from being accepted as native
qualification evidence.
The transition envelope commit must match the archive manifest commit; mixing
events from another checkout is rejected.
The summary’s transition SHA-256 is independently checked against the archived
payload, in addition to the manifest artifact digest.
The runner records the transition event count in the summary, and archive
verification cross-checks it against the packaged envelope.
Pass `--summary evidence/native-summary.json --archive
evidence/native-qualification.zip` to package the qualified summary and
captured artifacts immediately after native checks succeed.
The runner then invokes the independent archive verifier on that output before
reporting success, closing the producer-to-transfer validation loop.
The runner validates the supplied transition artifact before writing a
qualified summary, so invalid lifecycle evidence cannot pass the capture gate.
The packager also writes that count into the manifest; archive verification
cross-checks all three representations.
Manifest event counts are required to be positive JSON integers, not booleans
or coerced values.
The manifest carries the host platform too, and verification binds it to the
qualification summary before accepting the archive.
If the summary declares a platform, it must be a non-empty string so host
identity remains auditable across archive transfer.
When present, that platform identity must be a non-empty string; malformed
typed metadata is rejected before any evidence is trusted.
Transition-bearing manifests also declare the transition schema version, which
the verifier matches against the captured envelope.
They also carry the transition source commit, independently bound to the
captured envelope during verification.
The packager derives that manifest field from the captured payload rather than
duplicating a hard-coded schema value.
The captured host platform must also match the qualification summary platform,
preventing cross-host evidence mixing.
Transition-bearing qualified archives must identify a Linux host; Windows and
macOS runs remain pending handoff evidence rather than Linux qualification.
Capture provenance before validation and packaging:

```sh
python tests/pipewire/capture_pipewire_transition_evidence.py raw-events.json evidence/pipewire-transitions.json
```

The wrapper records the host platform, checkout commit, schema version, and
raw events so evidence cannot be detached from the build that produced it.
The archive packager reruns the transition validator before accepting the
artifact, preventing an unvalidated event file from entering the handoff.
It also rejects non-Linux transition provenance or a transition commit that
does not match the qualification summary before creating the archive.
Packaging also requires the summary’s positive transition event count before
it creates an archive.
It cross-checks that count against the actual transition payload before
writing any archive bytes.
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
The dependency-light end-to-end handoff fixture
`python tests/unit/check_pipewire_evidence_end_to_end.py` runs the packager and
independent verifier together, including transition evidence, so changes to
either side cannot silently drift apart.
The fixture also proves that an unqualified summary is refused before an
archive is created.
This checks the embedded manifest, artifact presence, byte sizes, and SHA-256
digests before the archive is accepted as a transferred handoff. It also
rejects duplicate or traversal-style ZIP entry names so an archive cannot
silently substitute ambiguous evidence files.
Unlisted ZIP entries are also rejected, so the manifest remains the complete
inventory of transferred evidence.
The verifier also cross-checks the manifest commit against the archived summary
commit, preventing evidence from being relabeled under a different build.
Malformed summary JSON is rejected with an explicit qualification-summary
error rather than being treated as an incomplete handoff.
The manifest itself must also decode to a JSON object before archive fields are
trusted.
Malformed transition evidence JSON is likewise rejected explicitly before
lifecycle validation.
The summary transition path must be a non-empty string before it is mapped to
an archived member.
Discovery and JUnit artifact paths follow the same string-only rule before
their archive basenames are compared.
The decoded summary must also be a JSON object, preventing arrays or scalar
values from bypassing field-level qualification checks.
The portable behavior fixture can be run with
`python tests/unit/check_pipewire_evidence_archive_verifier_behavior.py`; it
checks a valid archive plus rejection of unlisted, tampered, duplicate-entry,
and traversal-style artifacts. Empty evidence manifests are rejected as well,
so a qualified handoff always contains at least one archived artifact.
The manifest must also carry a non-empty repository commit identity.
The archived summary commit identity is independently required to be a
non-empty string before it can be matched to the manifest.
It records the `SOURCE_DATE_EPOCH` and UTC archive timestamp used for the ZIP,
so recipients can audit the reproducibility policy without the original host.
Run `python tests/unit/check_pipewire_evidence_packager_behavior.py` to verify
that fixed `SOURCE_DATE_EPOCH` inputs produce byte-for-byte identical archives.
The verifier parses that timestamp as ISO-8601 and rejects malformed metadata.
It also rejects negative or boolean `source_date_epoch` values, so the
reproducibility record cannot carry an invalid epoch disguised as an integer.
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
The archive manifest must carry the literal boolean `qualified: true`, not a
truthy substitute, so qualification status cannot be ambiguous on transfer.
Manifest artifact names are also unique case-insensitively, matching ZIP
transport behavior and preventing ambiguous duplicate declarations.
Dot and dot-dot artifact names are rejected as well, keeping the manifest
table limited to concrete flat archive members.
Control characters in declared names are rejected to match the ZIP entry
policy and prevent invisible archive members.
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
