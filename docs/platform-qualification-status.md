# Platform qualification status

This matrix records development evidence and the remaining gates. A build or headless test does not establish native desktop, audio, or overlay support.

| Platform | Current integrated/preparation evidence | Native gates still required |
| --- | --- | --- |
| Windows | `tests/windows/run_qualification.ps1` records build identity, configures/builds the x64 desktop targets, discovers tests, verifies artifacts, and can run CTest with `-RunTests`; endpoint-loss state and display selection/recovery have unit/QML coverage. | Real WASAPI loopback permission and routing, device removal/recovery on hardware, fullscreen and multi-monitor overlay behavior, tray recovery, DPI changes, and packaged runtime dependencies on supported Windows versions. |
| macOS | `tests/macos/run_qualification.sh` enables the production desktop-analysis bridge and records build/evidence metadata; `tests/macos/verify_qualification_artifacts.sh` verifies the retained bundle; shell syntax and local diff checks pass. | Re-run the production qualification on `zachbook-pro`; verify real Core Audio capture permission prompts, output-device switching/recovery, Spaces/fullscreen behavior, menu-bar recovery, Retina/DPI changes, Intel and Apple Silicon runtime behavior, and packaged signing/runtime dependencies. |
| Linux | Configure with `-DBEATGLASS_BUILD_PIPEWIRE=ON`; build serially when memory is constrained; run CTest, including optional virtual-graph tests; run strict documentation checks. | Real PipeWire device routing and permissions, device loss/replacement, X11 window-manager behavior, GNOME and KDE Wayland probing, layer-shell refusal/fallback, monitor/DPI changes, and recovery from an unavailable overlay capability. |

For every native run, record the OS version, kernel or build number, compiler, Qt version, audio backend/session version, display server or compositor, output device, build identity, exact commands, and observed limitations. Until those records exist for the relevant platform scenarios, support language must remain experimental.

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

The Linux capture gate also requires a documented sample-position clock mapping. A PipeWire graph-clock snapshot or callback arrival time is diagnostic evidence only; it must not be exposed as a frame-accurate anchor until buffer timing, converter/queue delay, clock domain, drift, and uncertainty have been measured on the target backend and device.

## Local evidence audit (2026-11-08)

The preparation scripts are executable on their respective hosts, but this Windows session is not a substitute for native qualification: it cannot produce WASAPI, Core Audio, or PipeWire device evidence. The current actionable handoff is therefore:

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
