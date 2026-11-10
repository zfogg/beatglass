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

Reacquisition evidence may include `device_id`; when supplied it must be a
non-empty identity matching the restart event's device. This prevents recovery
samples from being attributed to a different device after a switch.

Reacquisition evidence may include `format`; when supplied it must contain positive
sample rate and channel count and match the restart event's negotiated format.

If any event supplies `epoch`, every event must supply a positive integer epoch.
Partial epoch annotations are rejected so lifecycle evidence cannot silently mix
attributed and unattributed records.

When any `error` event includes `error_code`, every error event must include a
non-zero integer code. This keeps native failure provenance complete across repeated
discontinuities.

Process events may also include `serial`. If present, it must be a non-empty string
on every process event and must match the sink serial for that event's capture epoch:
the start serial before restart and the restart serial afterward. This preserves
device-switch ordering in retained native evidence.

Reacquisition evidence may include `timestamp_ns`; when supplied it must be a
positive integer no earlier than the restart timestamp.

Reacquisition evidence may include `drop_count`; when supplied it must be a
non-negative integer so overflow accounting remains trustworthy through recovery.

Reacquisition evidence may include `error_code`; when supplied it must be a
non-zero integer preserving native recovery error provenance.

Reacquisition evidence may include `error_domain`; when supplied it must be a
non-empty string identifying the native error domain.

Reacquisition evidence may include `recovery_reason`; when supplied it must be a
non-empty string describing why native capture was restarted.

Reacquisition evidence may include `process_index`; when supplied it must be a
non-negative integer identifying the recovery callback order.

Reacquisition evidence may include `event_count`; when supplied it must be a
positive integer describing the retained recovery event total.

Reacquisition evidence may include `artifact_id`; when supplied it must be a
non-empty identity matching the top-level artifact identity.

Reacquisition evidence may include `artifact_sha256`; when supplied it must be a
64-character hexadecimal digest matching the top-level artifact digest.

Reacquisition evidence may include `daemon_version`; when supplied it must be a
non-empty string identifying the native daemon that performed recovery.

Reacquisition evidence may include `capture_command`; when supplied it must be a
non-empty command string identifying how recovery capture was performed.

Reacquisition evidence may include `capture_started_at`; when supplied it must be
an ISO-8601 timestamp with an explicit timezone.

Reacquisition evidence may include `capture_ended_at`; when supplied it must be a
timezone-aware ISO-8601 timestamp no earlier than its recovery start timestamp.

Reacquisition evidence may include `platform`; when supplied it must be one of
`linux`, `windows`, or `macos`.

Reacquisition evidence may include `node_id`; when supplied it must be a positive
integer identifying the native capture node.

Reacquisition evidence may include `source_name`; when supplied it must be a
non-empty native source name.

Reacquisition evidence may include `commit`; when supplied it must be non-empty
and match the top-level artifact commit.

Reacquisition evidence may include `toolchain`; when supplied it must be a
non-empty string identifying the runtime/toolchain context.

Reacquisition evidence may include `environment`; when supplied it must be a
non-empty runtime environment description.

Reacquisition evidence may include `host`; when supplied it must be a non-empty
host identity for the machine producing recovery evidence.

Reacquisition evidence may include `capture_id`; when supplied it must be a
non-empty identity for the recovery capture stream.

Reacquisition evidence may include `source_id`; when supplied it must be a
non-empty native source identifier.

Reacquisition evidence may include `sink_name`; when supplied it must be a
non-empty human-auditable sink name.

Reacquisition evidence may include `source_type`; when supplied it must be a
non-empty native source classification.

Reacquisition evidence may include `profile`; when supplied it must be a
non-empty capture profile identifier.

Reacquisition evidence may include `policy`; when supplied it must be a
non-empty capture-policy identifier.

Reacquisition evidence may include `sample_position`; when supplied it must be a
non-negative integer, preserving a valid timeline coordinate for recovery samples.
Reacquisition evidence may include `reason_code`; when supplied it must be a
non-zero integer identifying the machine-readable recovery reason.

Reacquisition evidence must include the same non-empty `serial` as the restart
event, preventing recovery evidence from being attributed to a different sink.

When reacquisition evidence includes `format`, its `sample_rate` and `channels`
must be positive integers, preventing loosely typed values from passing native
format validation.

Any transition event that includes `format` follows the same positive-integer
requirements for `sample_rate` and `channels`.

The required restart-event `format` uses these same strict positive-integer
requirements for `sample_rate` and `channels`.

When both lifecycle boundaries provide `device_id`, the restart identity must
match the start identity so a recovery cannot silently switch devices.

The `epoch` values on the start and restart lifecycle events must be positive
integers, with restart strictly advancing beyond start.

When process events provide `sample_position`, it must be a non-negative
integer so timeline overlap checks cannot accept loosely typed coordinates.

When process events provide `timestamp_ns`, it must be a non-negative integer.
Provided process timestamps must also be monotonic in event order.

When process events provide `duration_ns`, it must be a non-negative integer.
When process events provide `sample_rate`, it must be a positive integer.
When process events provide `channels`, it must be a positive integer.
Every process event must carry a positive `epoch` matching either the start or
restart lifecycle epoch.
Every transition event must include a non-empty string `event` name.
When both timestamps and durations are provided, adjacent process intervals
must not overlap.
Lifecycle events must occur in `start`, `process`, `error`, `restart` order;
duplicate lifecycle boundaries are rejected.
When present, top-level `event_count` must be a positive integer equal to the
number of evidence events.

When reacquisition evidence provides `event_count`, it must equal the number
of process events recorded in the restarted epoch.
When `host` provenance is present, it must include a non-empty `name` and a
supported `platform` (`linux`, `windows`, or `macos`).
If present, host `node_id` must be a positive integer.
If present, host `build_id` must be a non-empty string identifying the native
build that produced the evidence.
If present, host `kernel` must be a non-empty string describing the host kernel.
If present, host `os_version` must be a non-empty string identifying the host OS
release.
If present, host `arch` must be a non-empty string identifying the CPU
architecture.
If present, host `container_id` must be a non-empty string identifying the
container or sandbox that produced the evidence.
If present, host `timezone` must be a non-empty string describing the host
clock timezone used for human-readable timestamps.
If present, host `distro` must be a non-empty string identifying the OS
distribution or platform family.
If present, host `virtualization` must be a non-empty string describing the
virtualization or bare-metal context.
If present, host `hypervisor` must be a non-empty string identifying the
hypervisor when evidence was captured in a virtual machine.
If present, host `secure_boot` must be a boolean indicating whether secure boot
was enabled during qualification.
If present, host `tpm_present` must be a boolean indicating whether a TPM was
available during qualification.
If present, host `attestation` must be a non-empty string describing the host
attestation or measurement record.
If present, host `firmware` must be a non-empty string identifying the host
firmware release.
If present, host `bios` must be a non-empty string identifying the BIOS or boot
firmware version.
If present, host `board` must be a non-empty string identifying the hardware
board or machine model.
If present, host `serial_number` must be a non-empty string identifying the
qualified host instance.
If present, host `cloud_provider` must be a non-empty string identifying the
cloud or hosting provider.
If present, host `region` must be a non-empty string identifying the provider
region or site.
If present, host `availability_zone` must be a non-empty string identifying
the provider availability zone or physical site.
If present, host `rack_id` must be a non-empty string identifying the physical
rack or placement.
If present, host `power_state` must be a non-empty string describing the host
power state during qualification.
