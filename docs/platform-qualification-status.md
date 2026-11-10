# Platform qualification status

## Current portable CLI/library evidence (2026-11-10)

Issue #4320 CLI evidence: the Windows x64 FTXUI build passes the focused TUI,
generated-audio, codec and snapshot tests (4/4). A GCC/FTXUI 7.0.0 build in WSL Ubuntu
successfully compiles and links the CLI, and Library/Live/Settings snapshots
plus generated-audio and TUI-contract checks pass. These are development and
cross-compiler preparation results, not native Linux qualification: WSL did
not have the PipeWire prerequisites, macOS was not available for a native run,
and the optional Essentia tonal path was not validated end-to-end.

The shipped CLI and music CLI expose documented `--help`/`-h` and
`--version`/`-V` entry points. JSON and human-readable analysis output expose
decoded duration, and generated WAV/AIFF/FLAC/MP3 integration checks validate
duration consistency and source immutability. These are portable development
checks only; they do not advance any native Windows, macOS, or Linux gate.

Remaining CLI/library gates are native decoder/runtime availability, packaged
dependency verification, real playback/output-device qualification, and the
platform evidence bundles listed below.

Readiness evidence for shortcut recovery must show that stop/resume shortcuts update an accessible announcement (`queued`, `running`, `cancelled`, `retry`, or `complete`), preserve focus, and do not leave stale results presented. Run the portable contract with `python tests/unit/check_desktop_analysis_transition.py`; native screen-reader and assistive-technology timing still require platform runs.

Native readiness evidence must verify keyboard parity for playback recovery: focus reaches pause/resume, retry, endpoint refresh, and reacquisition controls; accessible names expose current state; and stale-result clearing is announced without pointer input. Reproduce the portable audit with `python tests/unit/check_desktop_analysis_transition.py`; this does not replace native UI/accessibility qualification.

Native readiness handoff must include bridge transition evidence for playback recovery controls: queued, running, cancellation, retry, completion, endpoint loss, reacquisition, and stale-result clearing. Reproduce portable checks with `python tests/unit/check_desktop_analysis_transition.py` and `python tests/unit/check_playback_epoch_docs_contract.py`; these do not replace native desktop/audio/overlay observations.

The local retry decision contract is reproducible with `python tests/unit/check_bridge_resubmit_contract.py`, `python tests/unit/check_retry_evidence_docs.py`, and `git diff --check`. It verifies compatible retry admission and fail-closed rejection, but native decoder/runtime, playback, endpoint-loss, and overlay evidence remain required.

## Decoder retry evidence handoff

Native handoff bundles must demonstrate both a compatible retry (same decoder snapshot, source fixture hash, and identity) and an incompatible retry that is rejected fail-closed. Retain the original and retry commands, diagnostics, build/runtime identity, and resulting readiness status. The negative case is required evidence that stale decoder output cannot be promoted.

## Native codec/runtime evidence expectations

Each platform qualification run must record the decoder/runtime build identity, codec feature configuration, exact source fixture hash, decoder snapshot, selected backend/device, and the full command transcript. Evidence must include a successful decode plus a controlled failure or missing-codec case, with retained logs and the resulting readiness artifact. A portable decoder test or headless build is preparation evidence only and cannot advance the native matrix.

Decoder snapshot mismatch/legacy rejection is covered as a portable fail-closed rule. Native codec and runtime compatibility still require retained Windows, macOS, and Linux evidence; a passing mismatch contract is not native qualification.

Retry recovery requires a compatible non-empty `decoder_snapshot`; migrated legacy rows are intentionally rejected until fresh metadata is captured. This fail-closed policy prevents decoder drift from being presented as a valid result, but does not qualify codec/runtime behavior on Windows, macOS, or Linux.

Retryable storage schema v2 retains source-path and decoder-snapshot metadata for bridge resubmission. Portable migration and rejection checks do not establish that a native decoder, playback device, or audio endpoint can be reopened successfully; those observations remain required on each platform.

Playback epoch/recovery status: portable tests cover stale-state clearing and reacquisition after an epoch or endpoint transition. Native qualification must still capture real playback/output loss, recovery, and post-reacquisition evidence on each supported platform; headless epoch tests do not substitute for those observations.

This matrix records development evidence and the remaining gates. A build or headless test does not establish native desktop, audio, or overlay support.

| Platform | Current integrated/preparation evidence | Native gates still required |
| --- | --- | --- |
| Windows | `tests/windows/run_qualification.ps1` records build identity, configures/builds the x64 desktop targets, discovers tests, verifies artifacts, and can run CTest with `-RunTests`; endpoint-loss state and display selection/recovery have unit/QML coverage. | Real WASAPI loopback permission and routing, device removal/recovery on hardware, fullscreen and multi-monitor overlay behavior, tray recovery, DPI changes, and packaged runtime dependencies on supported Windows versions. |
| macOS | `tests/macos/run_qualification.sh` enables the production desktop-analysis bridge and records build/evidence metadata; `tests/macos/verify_qualification_artifacts.sh` verifies the retained bundle; shell syntax and local diff checks pass. | Re-run the production qualification on `zachbook-pro`; verify real Core Audio capture permission prompts, output-device switching/recovery, Spaces/fullscreen behavior, menu-bar recovery, Retina/DPI changes, Intel and Apple Silicon runtime behavior, and packaged signing/runtime dependencies. |
| Linux | Configure with `-DBEATGLASS_BUILD_PIPEWIRE=ON`; build serially when memory is constrained; run CTest, including optional virtual-graph tests; run strict documentation checks. | Real PipeWire device routing and permissions, device loss/replacement, X11 window-manager behavior, GNOME and KDE Wayland probing, layer-shell refusal/fallback, monitor/DPI changes, and recovery from an unavailable overlay capability. |

For every native run, record the OS version, kernel or build number, compiler, Qt version, audio backend/session version, display server or compositor, output device, build identity, exact commands, and observed limitations. Until those records exist for the relevant platform scenarios, support language must remain experimental.

## Recovery/resubmission validation

The portable recovery path now retains and displays retry identity fields and rejects resubmission without an explicit source path plus matching content, engine, configuration, and mode metadata. Local contract checks cover the bridge/UI handoff; they do not prove decoder correctness, native audio routing, or desktop recovery on supported hardware.

Local checks used for this slice:

```text
python tests/unit/check_recovery_details_ui_contract.py
python tests/unit/check_bridge_resubmit_contract.py
git diff --check
```

Native Windows WASAPI, macOS Core Audio, Linux PipeWire, desktop UI, overlay, device-loss, and packaged-runtime qualification remain open.

The platform status row and retained manifest must agree on source commit, capture time, and environment/toolchain; otherwise the run is not attributable to the documented build. Readiness reports also verify that aggregate failure-category counts, retained-artifact counts, and retained-artifact bytes equal the sum of their per-platform counts, expose retained artifact counts/bytes by platform, and require each retained count/byte total to match its artifact inventory.

The handoff matrix must contain exactly one Windows, one macOS, and one Linux row. Duplicate platform rows are a documentation error and fail the executable handoff check before retained evidence is considered.

Each retained evidence bundle must include `qualification-manifest.txt`, `qualification-artifacts.sha256`, `native-playback-evidence.json`, and `native-overlay-evidence.json` where those scenarios apply.

Every manifest must also identify its platform, source commit, UTC capture time, and environment/toolchain identity so evidence cannot be detached from the build that produced it.

Output and display scenarios must additionally retain `native-output-evidence.json` and `native-display-evidence.json` for endpoint selection/loss, overlay visibility, fullscreen, DPI, and monitor observations.

Capture and lifecycle scenarios must additionally retain `native-capture-evidence.json` and `native-recovery-evidence.json` for permission, endpoint-loss, reacquisition, and source-epoch observations.

The shared scenario matrix spans permission, capture, output-device selection, recovery, overlay, fullscreen, DPI, monitor changes, reduced motion, and packaged runtime behavior; each applicable row remains `UNRUN` until its evidence bundle is retained.

Accessibility readiness must separately record reduced-motion behavior, click-through recovery, and keyboard-accessible recovery controls.

Runtime evidence must explicitly retain the audio backend/session version, display server or compositor, exact commands, and observed limitations.

The scenario matrix must cover permission handling, output-device selection/loss, and recovery or reacquisition behavior on each applicable platform.

Detailed handoff checklists: [Windows](windows-qualification.md), [macOS](macos-qualification.md), and [Linux](linux-qualification-plan.md).

Each native scenario record must retain its capture timestamp, supporting logs, and a screenshot or recording path; a status without that evidence remains unqualified.

The retained-bundle verifier also requires every completed scenario to point to its own retained artifact. A single evidence file may not be reused to make multiple completed scenario rows appear qualified; duplicate evidence references are reported as a bundle error and keep the platform pending.

Artifact identity is canonical and relative to the retained bundle root. Alias paths such as `./capture.json`, path traversal, duplicate entries, and other non-canonical spellings are rejected so a scenario cannot silently refer to a different retained file than the manifest inventory records.

Scenario traceability is closed over the shared matrix: manifests must declare exactly the known `capture`, `recovery`, `output`, `display`, `playback`, and `overlay` scenario IDs. Unknown or duplicate IDs are verifier errors rather than silently ignored rows, so a typo cannot make a platform-specific native observation disappear from the readiness report.

Every completed scenario must declare an artifact `role` matching its scenario ID (`capture`, `recovery`, `output`, `display`, `playback`, or `overlay`). A valid hash or screenshot without a role, or in the wrong role, is not evidence for that scenario; the verifier reports the mismatch and keeps the platform pending.

## Issue #1 exit gate

Issue #1 remains open until the handoff is **fail-closed** across all three native platforms. A release owner may mark a platform ready only when its retained report has `ready: true`, an attributable manifest and source/build identity, complete required scenarios, valid retained paths and hashes, and no freshness, invalidation, provenance, or aggregate-consistency errors. The top-level report must show `all_platforms_ready: true` and `aggregate_consistent: true`; one missing, stale, invalidated, or inconsistent row keeps the entire handoff pending.

The three platform rows must be backed by actual desktop, audio, and overlay observations on supported Windows, macOS, and Linux machines—not headless, CI, WSL, SSH-only, synthetic, or portable-build substitutes. Each row must retain the exact commands, environment/toolchain, capture time, build/source identity, logs, screenshots or recordings, output/capture evidence, recovery evidence, and known limitations. The status matrix and retained manifests must agree on platform and provenance before support language can move beyond experimental.

The exit-gate decision is therefore: `READY` only when the machine-readable readiness report passes every platform and aggregate check; otherwise `PENDING` with the report's per-platform failures preserved. Do not claim issue #1 or v1 completion while any native scenario, platform row, artifact, or required evidence record remains pending.

Readiness reports also expose `artifact_role_summary` per platform. This is an audit view of which completed scenarios lack a declared retained-artifact role; any non-empty entry is a platform failure and keeps the handoff pending.

Reports additionally expose `native_qualification_summary` and `native_qualification_ready`. Each platform entry carries its unpassed scenarios, missing artifact roles, verifier error count, and readiness flag. Consumers must use this explicit summary, and may treat `native_qualification_ready: true` as an evidence-gate result only; it is not a substitute for the release owner’s review of the retained desktop/audio/overlay recordings and limitations.

The report also emits `native_summary_consistent`; it must be true before any readiness result is consumable. This cross-check ensures platform names, evidence-complete flags, and per-platform ready flags agree with the underlying rows.

For handoff automation, `native_qualification_blockers` gives one structured entry per blocked platform, including unpassed scenario IDs, missing artifact roles, and verifier errors. An empty list is necessary but not sufficient for a release decision; the owner must still review the retained native recordings and limitations.

The companion `native_blockers_consistent` flag must also be true: it verifies that the blocker platform set exactly matches the source rows that have unpassed scenarios, missing roles, or errors. Consumers must reject reports that fail this invariant.

Reports provide `platform_support_status` for dashboards: each row is explicitly `READY` or `PENDING`, with stable reason codes and blocker count. This is a projection of evidence readiness, not a claim that the platform has passed the required native desktop/audio/overlay review.

The companion `platform_support_status_consistent` flag must be true before consumers use that projection; it verifies that row order and READY/PENDING values still agree with the underlying native qualification summary.

Reports explicitly set `support_language` to `experimental` and `native_qualification_required` to `true`. These fields are intentionally conservative: evidence tooling cannot promote product support language or waive the native desktop/audio/overlay exit gate.

The machine-readable report declares `readiness_report_schema: 1`. Consumers must reject unknown schema versions rather than assuming that newly added fields or changed semantics are compatible.

Before handing a report to automation, run `python tests/platform/validate_readiness_report.py <retained-bundle-root>/readiness-report.json`. This local consumer check verifies the versioned conservative fields, exact platform status rows, and the fail-closed relationship between exit status and reason codes.

The consumer check also requires each platform row to link a manifest path and SHA-256 plus `provenance_complete: true`; a report with detached or incomplete platform identity is rejected before native evidence is considered.

Each platform artifact inventory must likewise retain a path, SHA-256, byte count, and `hash_valid: true`. This validates report linkage to retained files; it does not replace review of the actual native screenshots, recordings, logs, and command output.

Pass `--bundles <retained-bundle-root>` to `validate_readiness_report.py` to recompute each manifest and retained artifact hash/byte count locally. A report is not accepted when its recorded provenance or artifact inventory disagrees with the files on disk.

To create a conservative report from retained platform bundles, run `python tests/platform/generate_readiness_report.py <retained-bundle-root> <readiness-report.json>`. Generation always emits `PENDING`, keeps support language experimental, and requires a later native-evidence review before any gate can change.

The generator creates missing parent directories for the output path and writes
stable, sorted JSON, so reports can be safely produced into a fresh evidence
workspace and compared across runs.

Each retained manifest must include `qualification_command`; the generated platform row echoes it so reviewers can trace report values back to the exact command that produced the bundle. The command string is provenance metadata, not proof that native qualification succeeded.

Manifests also identify `qualification_output`, which must name a retained artifact. The validator checks that the report echoes this path and that the output is present in the manifest inventory, preventing command claims that cannot be tied to retained output.

Generated rows also carry `qualification_output_sha256`; retained-file validation recomputes that digest so a command's claimed output cannot be replaced after capture without invalidating the report.

The generated row also records `qualification_output_bytes`; the verifier checks the retained output size as well as its digest.

The retained-file verifier also requires the report artifact inventory to match the
manifest inventory exactly, requires unique scenario IDs and artifact paths, and
requires every scenario evidence hash mapping to match the manifest's scenario set.
Malformed JSON, path traversal, duplicate rows, and non-hexadecimal digests fail
closed before any qualification claim is accepted.

Each retained manifest must also carry a 40-character hexadecimal commit identity
and an ISO-8601 capture timestamp, so provenance fields are machine-checkable rather
than merely present.

Every retained artifact must declare a non-empty role, preserving the distinction
between command output, scenario evidence, and other captured files.

When consuming a generated report with `--bundles`, the scenario inventory is
cross-checked against each retained manifest (status and evidence path), so a
report cannot silently describe a different scenario state than the captured
bundle.

The consumer also requires the complete capture, recovery, output, display,
playback, and overlay scenario set on every manifest; partial bundles remain
ineligible for acceptance.

For completed scenarios, the retained evidence artifact's declared role must
match the scenario ID, preventing a valid file from being reassigned to a
different scenario after capture.

The consumer recomputes every manifest artifact hash as well as the report's
inventory hashes; a manifest that names a file but carries a stale digest is
rejected even when the report row itself is internally consistent.

Generated rows also echo the manifest platform identity, and the consumer checks
that echo before accepting any platform-specific evidence.

For each scenario with an evidence path, generated rows include `scenario_evidence_sha256`. Verification recomputes those hashes so scenario claims remain tied to retained files rather than mutable paths.

The qualification handoff checker treats `readiness_report_schema: 1`, `exit_gate_status`, and `exit_gate_reason_codes` as required documentation contracts; removing any of these descriptions fails the local handoff check before a platform handoff is accepted.

The same documentation contract covers `platform_support_status` and `native_qualification_required`, ensuring dashboard projections remain explicitly conservative.

Readiness is stricter than manifest syntax: every required `capture`, `recovery`, `output`, `display`, `playback`, and `overlay` scenario must be `PASS` before a platform can be ready. The report exposes `unpassed` scenario IDs, so a complete list containing `UNRUN` rows cannot be mistaken for native qualification.

The same rule is enforced by the executable `check_evidence_readiness.py` command: any required scenario that is missing or not `PASS` makes the command exit nonzero. A report may still be generated for diagnosis, but its incomplete bundle must not be accepted by automation.

Every report includes `exit_gate_status` (`PENDING` or `READY`) and `exit_gate_reasons`. Automation should consume these fields together; `PENDING` is the safe default for missing, stale, inconsistent, or incomplete native evidence.

For stable automation, `exit_gate_reason_codes` contains normalized categories such as `manifest`, `retention`, `freshness`, `scenario`, `hash`, `path`, and consistency codes. Unpassed scenarios and missing artifact roles are classified as `scenario`. Human-readable reasons remain available for handoff diagnosis; consumers must not infer readiness by matching prose.

Before accepting a handoff, run `python tests/platform/check_evidence_readiness.py --status docs/platform-qualification-status.md --bundles <retained-bundle-root> --report <retained-bundle-root>/readiness-report.json`. The command requires exactly one unique Windows, macOS, and Linux status row with one attributable manifest beneath the bundle root, and reports platform-count/summary consistency plus per-platform failure summaries (including ready flags, ready count, not-ready platform names, category-count consistency, and category-to-platform details) and failure/aggregate-summary, error-count, failure-category-count, and category-to-platform consistency checks alongside manifest identity, provenance status and freshness error details, invalidation, retention status, missing or hash-mismatched artifact paths, manifest hashes, retained artifact counts/bytes, sorted artifact/scenario inventories, scenario status counts, plus top-level/per-platform error/count/failure-category summaries, required-scenario summaries, provenance summaries, and `summary_consistent`/`all_platforms_ready`/`aggregate_consistent` checks in a reproducible machine-readable report. A stale, invalidated, inconsistent-summary, inconsistent-error-count, inconsistent-category-count, inconsistent-category-platform-list, incomplete-retention, hash-mismatched, invalid-scenario, or `ready: false` report must remain pending; consumers must propagate its top-level errors rather than treating individual row output as informational.

## Issue #76 UI gate

The DJ workflow must remain usable when overlay capability is unavailable or denied. Native checks must verify that the main controls stay recoverable, the selected output device and capture state remain visible, stale analysis is cleared after loss, and an unavailable overlay never causes an invisible or stranded window. Click-through, tray/menu-bar recovery, fullscreen behavior, monitor changes, and reduced-motion behavior are separate platform observations; a mockup or window flag alone does not satisfy them.

The committed design evidence and state/component gallery are documented in
[`docs/overlay-design-gallery.md`](overlay-design-gallery.md), including the
native screenshot qualification matrix. These renderings organize the evidence
but do not change any platform row to READY.

The Linux capture gate also requires a documented sample-position clock mapping. A PipeWire graph-clock snapshot or callback arrival time is diagnostic evidence only; it must not be exposed as a frame-accurate anchor until buffer timing, converter/queue delay, clock domain, drift, and uncertainty have been measured on the target backend and device.

## Local evidence audit (2026-11-08)

The preparation scripts are executable on their respective hosts, but this Windows session is not a substitute for native qualification: it cannot produce WASAPI, Core Audio, or PipeWire device evidence. The current actionable handoff is therefore:

Qualification matrices normalize absolute runner artifact paths to paths relative
to the summary directory before handoff. This keeps retained evidence portable
and allows the matrix validator's traversal checks to remain fail-closed.
For local handoff review, `validate_qualification_matrix.py MATRIX.json ROOT`
additionally verifies every listed evidence artifact exists beneath `ROOT`;
the original one-argument shape remains a portable schema-only check.
The evidence-root check resolves each artifact before containment testing, so a
symlink inside the retained root cannot point to evidence outside that root.

| Evidence class | Windows session result | Required native owner/action |
| --- | --- | --- |
| Build identity and artifact inventory | Prepared by `tests/windows/run_qualification.ps1`; no native-device run claimed here | Run on supported Windows hardware with a logged-in desktop session and retain the generated evidence directory. |
| macOS production bridge and overlay | Not runnable on Windows; script/checklist remains the source of truth | Run `tests/macos/run_qualification.sh` on `zachbook-pro`, then complete the permission, device-loss, overlay, and packaged-runtime observations above. |
| Linux PipeWire timing and routing | Not runnable on Windows | Run the Linux qualification plan with a real graph and record clock-domain, queue-delay, drift, and uncertainty measurements. |

This audit intentionally records unavailable evidence as pending rather than treating portable builds, fake backends, or headless tests as native support.

## Documentation reconciliation

The merged handoff tooling and this status page distinguish implemented preparation checks, attributable retained evidence, and native qualification observed on hardware. The first two states are covered by local verifier tests and portable fixtures; none changes the Windows, macOS or Linux rows to `READY`. Support language remains experimental until each row contains real desktop, audio and overlay evidence and the machine-readable report passes the all-platform exit gate.

The evidence-bundle verifier also rejects incomplete (`UNRUN` or `PENDING`) scenarios that claim retained evidence. Evidence ownership is reserved for completed `PASS` or `FAIL` scenarios, preventing a pending row from being mistaken for a qualified observation.

Completed scenario evidence references are also required to be a single retained path string. Malformed values such as arrays fail closed as unretained evidence rather than being interpreted as a valid artifact reference.

Artifact manifest hashes follow the same fail-closed rule: `sha256` must be a string before normalization or comparison. Structured or otherwise malformed hash values are rejected as invalid artifact entries instead of causing verifier exceptions.

Scenario inventory IDs are validated as strings before set-based completeness checks. Structured IDs fail as invalid scenarios and preserve the missing-required-scenario signal, keeping malformed manifests pending rather than allowing an exception to bypass the consumer decision.

Duplicate-ID checks apply only to validated string IDs; repeated structured IDs are reported as invalid scenarios without allowing the duplicate analysis itself to throw.

Artifact-role indexing likewise considers only validated string paths. A structured artifact path is rejected during manifest validation and cannot trigger a secondary exception while scenario ownership is being checked.

Scenario statuses are validated as strings before allowed-state membership checks. Structured status values fail as invalid scenarios and leave the evidence bundle pending rather than raising in the verifier.

The readiness report consumer also stringifies malformed status values when building diagnostic counts, ensuring invalid manifests remain reportable and pending instead of failing while aggregating the inventory.

Readiness retained-file and artifact-role indexes include only validated string paths. Structured paths are surfaced through verifier errors and unlisted-file diagnostics without causing report aggregation to raise.

Readiness artifact-role diagnostics also require each scenario evidence reference to be a string before lookup. Structured evidence references are classified as missing ownership and keep the platform pending instead of raising during report generation.

Readiness report aggregation treats non-list `artifacts` or `scenarios` containers as empty diagnostic inventories while the verifier records the manifest contract error. This preserves a machine-readable pending report for malformed container types.

The lightweight readiness check applies the same validated-string scenario-ID rule as report generation, so malformed IDs cannot crash one consumer path while another reports a conservative pending result.

Valid JSON manifests must still have an object root. Non-object roots are reported explicitly by both readiness consumers and remain pending rather than causing `.get()` aggregation failures.

Artifact diagnostic inventories sort paths by their string representation, so mixed valid and malformed path values remain deterministically reportable while the bundle stays pending.

The lightweight readiness check also normalizes null or non-list artifact/scenario containers before iteration, matching report generation and preserving verifier contract errors rather than raising.

The end-to-end local handoff command is `python tests/platform/run_readiness_handoff.py <retained-bundle-root> <report.json> --status docs/platform-qualification-status.md`. It generates the conservative report, validates the same JSON contract consumed by readiness tooling, checks retained bundles against the documented platform matrix, and records any `handoff_check_errors` in the report artifact itself. When `--review-json` is combined with `--consume-review` or `--require-ready`, the handoff consumes the retained report and packet as one bundle and verifies their binding. A structurally valid result remains `PENDING` until native desktop/audio/overlay evidence is reviewed; incomplete scenarios intentionally produce a nonzero result.

`handoff_check_errors` is a required report field: it is always a list of nonempty strings (possibly empty when no status matrix was supplied), so consumers can distinguish a validated handoff artifact from one whose readiness checks were never recorded. With `--check-docs`, the command also runs `check_qualification_handoff.py` and records `documentation_check_errors`, proving the executable qualification procedures and status contract are present before a handoff is reviewed.

`documentation_check_errors` is also required and typed as a list of nonempty strings. Generated reports initialize both error lists explicitly; a consumer must reject reports that omit either audit trail.

The report consumer is `python tests/platform/consume_readiness_handoff.py <report.json>`. It validates the complete report contract and rejects `PENDING` (including all reports generated before native gates pass); only an explicitly validated `READY` report can be accepted by downstream automation. Add `--consume` to `run_readiness_handoff.py` to execute this consumer against the report produced by the same handoff run.

Portable regression coverage executes the cross-platform qualification script syntax/required-artifact dry run when PowerShell is available and checks the macOS verifier shell syntax when Bash is available. These dry runs exercise script wiring and artifact-tool presence only; they do not establish native device, desktop, audio, or overlay qualification.

Native script output can be ingested with `python tests/platform/ingest_qualification_artifacts.py <qualification-output> <bundle>/manifest.json`. The ingester verifies listed hashes and path containment, carries forward identity metadata, and emits all native scenarios as `PENDING`; it never upgrades script output to qualification by itself.

The same ingestion is available in the end-to-end handoff via `--native-output <qualification-output> --native-platform windows|macos|linux`; this writes the platform manifest before report generation, validation, and optional consumption. Ingestion failures remain visible and cannot be converted into readiness.

The handoff report records those failures in `ingestion_errors` as well as returning a nonzero result, making missing or tampered native artifacts auditable after the command exits.

When ingestion is invoked through the handoff command, the manifest platform must match `--native-platform`; mismatched identity is rejected and recorded as an ingestion error rather than attributed to the wrong qualification row.

Ingestion also requires a 40-hex source commit and nonempty capture time in the native identity manifest. Missing or malformed provenance is retained as an explicit ingestion error and cannot be promoted to readiness.

Capture times are parsed as ISO-8601 during ingestion; a malformed timestamp is rejected before the artifact can participate in a readiness handoff.

Ingestion also imports the native script’s referenced playback, overlay, display-topology, and audio-inventory files into the artifact inventory, so those handoff inputs cannot be silently omitted from provenance review.

The readiness report preserves each artifact’s role through generation, allowing downstream review to distinguish topology/audio/playback/overlay evidence from generic qualification outputs.

The final consumer rejects a `READY` report with missing or empty artifact roles, making provenance loss between ingestion and acceptance fail closed.

The portable regression suite exercises the complete review path—generation, optional native ingestion, documentation/readiness checks, report persistence, and downstream consumption—and expects a nonzero `PENDING` result for incomplete native evidence. This verifies orchestration without treating fixtures as hardware proof.

`ingestion_errors` is a required list-of-strings report field. The downstream consumer rejects malformed or omitted ingestion audit data, preventing native artifact provenance failures from being hidden by an otherwise valid report shape.

The consumer also rejects any report containing nonempty generation, handoff, documentation, or ingestion error lists—even if its gate status is manually changed to `READY`. Audit failures are therefore fail-closed at the final acceptance boundary.

The consumer additionally rejects `READY` while `native_qualification_required` remains true. A downstream acceptance decision therefore requires both an error-free report and an explicit native-qualification state change; portable preparation evidence cannot be promoted by editing the gate flag alone.

`READY` is also rejected while any platform support row remains `experimental`; native acceptance must update the support matrix consistently with the reviewed evidence.

`READY` additionally requires every platform scenario inventory row to be `PASS`; any `UNRUN`, `PENDING`, or `FAIL` scenario keeps the consumer non-accepting.

For human review, render `python tests/platform/review_readiness_handoff.py <report.json> <review.md>`. The checklist exposes each platform’s scenario statuses and artifact roles, but remains explicitly informational and preserves `PENDING` until native gates are reviewed.

The same checklist can be exported during handoff with `--review-output <review.md>`, keeping the report and human-review artifact generated from one identical input.

The exported checklist includes all four audit-error channels (`generation_errors`, `handoff_check_errors`, `documentation_check_errors`, and `ingestion_errors`) before the platform scenario/artifact sections, giving reviewers an actionable reason for every pending handoff.

It also displays the native-required flag and each platform’s support status, making an inconsistent transition visible before strict consumer acceptance.

Each platform section includes manifest hash, source commit, capture time, and environment anchors so reviewers can correlate checklist observations with retained provenance.

It also includes the qualification command and output linkage recorded for that platform, making the review artifact traceable to the native run that produced the evidence.

The checklist’s acceptance diagnostics are generated by the same consumer used by strict review, so the exported artifact shows the exact downstream rejection reasons rather than only raw status fields.

Use `--require-ready` when the checklist is an acceptance gate; it exits nonzero for `PENDING`. Without that flag, checklist export remains informational and never changes the report state.

Strict review also invokes the downstream consumer, so malformed audit trails, unpassed scenarios, missing roles, experimental support, or native-required state cannot pass merely by changing the top-level gate.

Use `--json-output <review.json>` alongside the Markdown export for machine-readable review tooling; the packet carries gate state, acceptance diagnostics, platform support, provenance anchors, scenario status, and artifact summaries without changing acceptance semantics.

Machine review packets use `review_packet_schema: 1`; consumers should reject unsupported versions rather than infer fields from an unversioned export.

Each packet also carries a SHA-256 hash of its source report; packet consumers reject missing or malformed hash anchors so review artifacts remain traceable to the exact report under review.

For retention verification, pass `--report <report.json>` to the packet consumer; it recomputes the source hash and rejects packets detached from the retained report.

For release/review handoff, retain both files in one bundle and consume them together with `python tests/platform/consume_review_bundle.py <bundle-root>`. The bundle consumer validates the report contract, requires the packet, and verifies the packet's source hash against that exact retained report. Missing or replaced files remain pending; this portable workflow does not claim native desktop/audio/overlay qualification.

To create that retained handoff bundle from generated artifacts, run `python tests/platform/retain_review_bundle.py <report.json> <review.json> <bundle-root>`. Export is verified immediately; pending native evidence or any detached/invalid input returns a nonzero result and remains diagnostic rather than release-ready.

To feed a canonical bundle into release/review automation, run `python tests/platform/review_release_decision.py <bundle-root> <decision.json>`. The decision artifact records `ACCEPTED` or conservative `PENDING`, the exact report SHA-256, and machine-readable reasons. Missing, detached, or native-pending evidence can never be promoted by this consumer. The end-to-end handoff can emit the same artifact with `--review-json <review.json> --decision-output <decision.json>`; a pending decision is returned as a nonzero handoff result for release automation.

The release boundary is executable: `python tests/platform/promote_review_bundle.py <bundle-root> <release-root>` copies the canonical report and packet only after the decision consumer returns `ACCEPTED`; it also writes and verifies `release-manifest.json` with both promoted artifact hashes. A `PENDING` bundle leaves the release destination untouched.

Downstream release jobs must verify the promoted artifact with `python tests/platform/verify_release_manifest.py <release-root> --signoff <signoff.json>`. The verifier recomputes both hashes, requires manifest schema 1 and `ACCEPTED`, binds the signoff to the manifest report hash, and rejects missing, modified, or unsigned files before publication.

The final release-directory check is `python tests/platform/verify_release_bundle.py <release-root>`. It requires the retained signoff in the release directory and verifies the complete report/packet/manifest/signoff set together before publication.

Native review owners can create an explicit assessment with `python tests/platform/review_native_evidence.py <report.json> <reviewer> <native-review.json>`. The assessment lists every platform/scenario blocker and remains `PENDING` until required scenarios have retained `PASS` evidence; it records review metadata but does not replace actual desktop/audio/overlay qualification.

The end-to-end handoff emits this assessment with `--native-review-output <native-review.json> --native-reviewer <reviewer>`, alongside the report and packet. A pending assessment returns nonzero and remains an explicit qualification blocker.

The complete signed release workflow is `python tests/platform/prepare_release_bundle.py <report.json> <review.json> <retained-root> <release-root> --signoff <signoff.json>`: it retains and verifies the pair, evaluates the conservative decision, checks both signoff hashes, promotes only `ACCEPTED`, and verifies the resulting manifest against the human signoff. Native-pending evidence or a mismatched signoff stops before release output is created.

After automation accepts a bundle, a release owner records manual acceptance with `python tests/platform/record_release_signoff.py <decision.json> <reviewer> <signoff.json>`. The signoff is bound to the decision's report and review-packet hashes and refuses `PENDING`; it is an audit record, not a substitute for native desktop/audio/overlay qualification.

The final review checklist is intentionally ordered: retain the pair, emit the decision, record manual signoff, run the signed preparation workflow (which retains `signoff.json` beside the release artifacts), then verify the dual-hash release manifest before publication. Each step remains fail-closed and native qualification remains a separate required gate.

The machine consumer is `python tests/platform/consume_review_packet.py <review.json>`. It rejects unsupported packet versions, recorded acceptance errors, and non-`READY` gate state; native qualification remains pending until the packet is backed by reviewed hardware evidence.

The packet consumer independently rechecks native-required state, experimental support rows, and scenario statuses, so clearing `acceptance_errors` in a tampered packet cannot bypass release acceptance.

The handoff command can emit the same packet directly with `--review-json <review.json>`, ensuring Markdown and machine-readable review outputs are derived from the identical generated report.

Add `--consume-review` to validate that emitted packet immediately; pending native evidence intentionally makes this integrated command nonzero.

When `--require-ready` is supplied with `--review-json`, packet consumption is automatic; strict release review cannot accidentally omit the versioned consumer step.

The release/review workflow should use both `--review-json` and `--consume-review`; this keeps the machine packet versioned and ensures its consumer—not only the producer—approves the handoff.

The equivalent end-to-end invocation is `run_readiness_handoff.py ... --review-output review.md --require-ready`; it writes the checklist and fails the same handoff run when native acceptance has not been reached.

The portable consumer tests cover the eventual transition explicitly: clearing `native_qualification_required` removes only that native-state rejection; all remaining report-contract, provenance, and readiness checks still apply. This is a transition contract, not native evidence.

Tampered or path-escaping retained artifacts fail ingestion with explicit errors, so a bundle cannot become a seemingly valid pending manifest while silently losing provenance. The resulting manifest remains diagnostic only until native scenario observations are reviewed.
