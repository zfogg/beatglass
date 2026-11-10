# Linux qualification plan

This plan records the evidence still required before Linux support can be claimed. WSL, CI, headless sessions, and virtual PipeWire graphs are useful development checks but do not satisfy the native desktop gates.

Retain `qualification-manifest.txt`, `qualification-artifacts.sha256`, `native-capture-evidence.json`, `native-recovery-evidence.json`, `native-output-evidence.json`, and `native-display-evidence.json` with each native run.

The Linux runner records an ISO-8601 UTC `captured_at` value and host environment
metadata in both pending and attempted summaries, so handoffs remain attributable
even when the native host is unavailable.
Attempted summaries also record the host operating-system label alongside the
platform identity.
Pending and attempted summaries use the explicit
`beatglass-native-runner-summary-v1` schema identifier for downstream validators.
The completed-summary verifier requires that schema, a timezone-aware capture
timestamp, Python environment identity, and host OS before accepting evidence.
The portable dry-run test also exercises the unavailable-host path and verifies
that it emits an attributable pending handoff rather than an empty report.
The pending handoff also enumerates the host, daemon, development-package, and
audio-sink blockers needed before a native run can start.
Cross-platform qualification scripts similarly record unrun native steps
explicitly rather than presenting an incomplete artifact set as success.
The Windows and macOS dry-run contract checks also require their retained
qualification manifest and scenario-case outputs to remain present in the
handoff scripts.

## Runnable preparation checks

- Run `python tests/pipewire/check_linux_native_prerequisites.py --report evidence/linux-pipewire-preflight.json` to record a machine-readable prerequisite result. On non-Linux hosts this exits successfully with `status: pending`; it never claims native capture evidence.
- The probe checks daemon reachability and the independent `pkg-config` headers/linker capability, so a host with a running daemon but no development package cannot be mistaken for a build-ready qualification host.
- The generated report includes the exact next probe, CMake configuration, and native-runner commands, making a pending Windows/macOS handoff actionable without implying that those commands ran.
- Build with `-DBEATGLASS_BUILD_PIPEWIRE=ON` and run the complete CTest suite.
- Run the isolated virtual-sink graph tests for capture, source replacement, wrong-link injection, interruption, and bounded queue overflow.
- Run the strict API documentation checks and record the indexed header count.
- Record PipeWire, WirePlumber, compiler, kernel, Qt, and session versions with each result.

## Native Linux gates

For each session, record the exact distribution, kernel, compositor/session version, display server, PipeWire and WirePlumber versions, output device, and build identity.

- Verify output-device discovery, permission behavior, selected-sink routing, device removal, replacement, and recovery on real hardware.
- Verify that graph loss, format change, queue overflow, and discontinuity clear published analysis and require a new acquisition epoch.
- Transition artifacts may include `timestamp_ns` on every event. When present, the validator requires positive integer timestamps for all events in nondecreasing order, preserving capture timing evidence without inventing timestamps for older artifacts.
- A restart event may include `reacquisition` evidence. When present, it must identify the restart epoch and sink serial and include positive captured samples; malformed reacquisition claims are rejected rather than treated as proof of recovery.
- Transition artifacts may include `device_id` on each event. When present, the validator requires a non-empty identity on every event and at both lifecycle boundaries, allowing a real device switch to be observed without treating serial numbers alone as device provenance.
- Process events may include negotiated `format`; when present, the validator requires positive sample rate/channels and agreement with the restart format, preventing a recovery artifact from mixing incompatible stream descriptions.
- Transition events may include `sample_position`; when present, positions must be complete and monotonic, and successive process blocks may not overlap their captured sample ranges. This preserves discontinuity/reacquisition timing evidence without inventing positions.
- Error events may include `error_domain`; when present, every error event must carry a non-empty domain and the restart must state a non-empty `recovery_reason`, keeping native failure provenance tied to the claimed recovery.
- Transition artifacts may include `host` provenance with `platform`, `daemon_version`, and `capture_command`. When present, these fields must identify a supported host and the exact daemon/capture invocation, without turning pending artifacts into qualification claims.
- Transition events may include a zero-based `sequence`; when present, it must be complete and contiguous, so dropped or reordered evidence records are rejected before review.
- Transition artifacts may include a non-empty `artifact_id`; this lets retained native captures be referenced unambiguously during review without treating an identifier alone as qualification evidence.
- Transition artifacts may include an `artifact_sha256`; when present, it must be a complete hexadecimal SHA-256 digest so retained evidence can be integrity-checked before review.
- Transition artifacts may include `capture_started_at`; when present, it must be an ISO-8601 timestamp with timezone, preserving when native capture began without accepting ambiguous local times.
- Transition artifacts may include `capture_ended_at`; when both capture bounds are present, the end must be timezone-aware and not precede the start, preserving an auditable capture interval.
- Transition artifacts may include `event_count`; when present, it must equal the serialized event list length, preventing truncated or appended records from silently passing review.
- Exercise an X11 session with a real window manager, including stacking, input-shape behavior, monitor/DPI changes, fullscreen applications, and recovery from an off-screen overlay.
- Exercise GNOME and KDE Wayland sessions separately. Probe layer-shell availability and compositor refusal; fall back to an ordinary compact window when the required capability is absent.
- Verify that no fallback silently captures a microphone or a different output sink, and that the user can always recover the main controls.

Until these observations are recorded on actual Linux desktops, the project should describe Linux capture and overlay as experimental and unsupported.

Review acceptance also verifies that every `PASS` scenario points to a retained,
non-empty evidence file beneath the submitted report directory; a status string
alone cannot promote native evidence.

The reviewer additionally requires matching platform/manifest identity, a
40-character build commit, complete provenance, exact artifact counts, and the
full six-scenario inventory before a platform can be accepted. Every retained
artifact must also carry a valid 64-character SHA-256, positive byte count, and
an explicit successful hash check. Platform manifests must carry their own
validated SHA-256 identity plus the exact qualification command and output
artifact used to produce the handoff. The declared output path and digest must
also resolve to the same retained artifact entry.

Reports must contain one entry per platform and one entry per scenario; duplicate
platform or scenario identifiers are rejected rather than silently overwritten.
Scenario statuses are limited to `PASS`, `UNRUN`, `FAIL`, and `BLOCKED`; only
`PASS` may carry evidence, preventing contradictory pending reports.
The report-level platform inventory must match its platform-support summary, and
`ready: true` is only valid with an `ACCEPTED` exit-gate status.
Platform identities are limited to `linux`, `windows`, and `macos`.
An acceptance report must include all three platform entries; a single-host
report remains pending even if its local scenarios pass.
Reports use readiness schema version `1`; a report cannot claim ready status
while disabling the native qualification requirement.
Ready reports must have no exit-gate reason codes; pending reports must retain
at least one reason code explaining the unresolved gate. Ready reports must
also have no handoff-check errors.
Reason codes must be non-empty strings so pending gates remain machine-readable.
Reason codes must also be unique to avoid obscuring the actual blocker set.
They must also have no documentation-check errors.
The report handoff status must be `complete` (or omitted for legacy reports)
when `ready: true`; pending or failed handoffs cannot be promoted.
Conversely, `exit_gate_status: ACCEPTED` is invalid unless `ready: true` is
also present.
Gate booleans such as `ready` and `native_qualification_required` must be JSON
booleans, not strings that merely resemble booleans.
Support summaries must use one known status per platform: `experimental`,
`qualified`, `unsupported`, or `pending`.
If both `scenarios` and `scenario_inventory` are present, they must be identical;
the reviewer will not silently choose one view over the other.
`qualified` requires every scenario in that platform's inventory to be `PASS`;
an all-`PASS` inventory cannot remain marked non-qualified.
Retained-byte totals must equal the sum of the recorded artifact sizes, so
manifests cannot under-report or over-report the evidence they contain.
Scenario evidence digest maps must reference only `PASS` scenarios and contain
valid SHA-256 values present in the retained artifact inventory. Artifact paths
must be unique within a platform bundle and every artifact must declare a
non-empty provenance role and hexadecimal SHA-256 digest. Artifact paths must be
relative, normalized paths that cannot escape the evidence bundle.
Dot segments, duplicate separators, and backslash-separated paths are rejected
to keep manifests portable across host platforms.
Artifact roles are limited to the six qualification scenario roles.
Every `PASS` scenario must have a corresponding retained evidence digest.
Each platform entry must also identify the capture environment and include an
ISO-8601 capture timestamp with timezone, so evidence cannot be detached from
its host run or interpreted in an unknown clock context. When a report declares
a build commit, every platform entry must use the same commit identity.
Capture timestamps later than the verifier clock are rejected as impossible
provenance.

Use the [native Linux evidence template](linux-native-evidence-template.md) for each session so environment identity, commands, observations, and limitations remain reviewable.
