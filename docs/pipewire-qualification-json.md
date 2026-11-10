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
If present, host `thermal_state` must be a non-empty string describing thermal
conditions during qualification.
If present, host `fan_state` must be a non-empty string describing fan or
cooling state during qualification.
If present, host `battery_state` must be a non-empty string describing battery
or mains-power state during qualification.
If present, host `network_state` must be a non-empty string describing network
connectivity state during qualification.
If present, host `dns_state` must be a non-empty string describing DNS
resolution state during qualification.
If present, host `ntp_state` must be a non-empty string describing clock
synchronization state during qualification.
If present, host `clock_source` must be a non-empty string identifying the
clock used for qualification timestamps.
If present, host `clock_accuracy_ns` must be a non-negative integer describing
the timestamp clock's accuracy bound.
If present, host `clock_drift_ppm` must be a numeric clock drift measurement;
boolean values are invalid.
If present, host `clock_jitter_ns` must be a non-negative numeric jitter
measurement; boolean values are invalid.
If present, host `clock_leap_state` must be a non-empty string describing leap
second handling during qualification.
If present, host `clock_sync_interval_ms` must be a non-negative integer
describing the clock synchronization interval.
If present, host `clock_sync_method` must be a non-empty string naming the
clock synchronization method.
If present, host `clock_sync_offset_ns` must be numeric; boolean values are
invalid.
If present, host `clock_holdover_state` must be a non-empty string describing
clock holdover behavior during qualification.
If present, host `clock_quality` must be a non-empty string describing the
clock quality classification.
If present, host `clock_stratum` must be a positive integer identifying the
clock synchronization stratum.
If present, host `clock_poll_interval_ms` must be a non-negative integer
describing clock status polling cadence.
If present, host `clock_source_count` must be a positive integer describing
the number of available synchronization sources.
If present, host `clock_source_primary` must be a boolean indicating whether
the recorded source was the host's primary synchronization source.
If present, host `clock_source_id` must be a non-empty string identifying the
selected synchronization source.
If present, host `clock_source_type` must be a non-empty string classifying the
selected synchronization source.
If present, host `clock_source_status` must be a non-empty string describing
the selected source's qualification status.
If present, host `clock_source_failover` must be a boolean indicating whether
the source was selected through failover.
If present, host `clock_source_switch_count` must be a non-negative integer
counting synchronization-source switches during qualification.
If present, host `clock_source_last_switch_ns` must be a non-negative integer
timestamp for the latest synchronization-source switch.
If present, host `clock_source_health` must be a non-empty string describing
the selected source's health.

The machine-readable host contract is captured in
`tests/pipewire/native_host_provenance_schema.json`; validator behavior remains
the qualification authority and native execution evidence is still required.
Before packaging a native run, validate the extracted host object directly:
`python tests/pipewire/validate_native_host_provenance.py HOST.json`.
The host validator accepts one host path and an optional report path; missing
or extra arguments fail before any evidence file is opened.
The host evidence root must be a JSON object; scalar, null, and array roots are
rejected before provenance fields are inspected.
Host `name` must contain visible non-whitespace text; empty and whitespace-only
names are rejected as unusable provenance.
The platform value is a case-sensitive enum (`linux`, `windows`, or `macos`);
unknown, empty, and differently cased values are rejected.
`node_id` must be a positive integer; JSON booleans are explicitly rejected
even though some language runtimes represent them as integers.
Zero and negative node IDs are rejected as well; `1` is the minimum accepted
host node identifier.
String-encoded node IDs are rejected; host evidence must carry the numeric JSON
type rather than a display-formatted substitute.
Fractional numeric node IDs are rejected too; node identifiers are integral
host identity values, not measurements.
`secure_boot` must be a JSON boolean when present; numeric sentinel values are
not accepted as native host evidence.
`tpm_present` follows the same boolean-only contract; numeric sentinel values
are rejected rather than interpreted as availability evidence.
Validated optional fields are retained in sorted `checked_fields` report
metadata, allowing reviewers to see which host attributes were actually
qualified without claiming native execution.
Malformed or ambiguous host JSON is rejected before any host fields are
trusted; duplicate keys are not silently normalized.
Missing host evidence files are rejected through the same unreadable-input path;
the validator never treats an absent file as an empty host object.
When emitting `REPORT.json`, an unwritable destination is also rejected with an
explicit error rather than leaving a partially written qualification artifact.
Reports are assembled in the destination directory and atomically replaced so
readers never observe a partially written JSON report.
Missing parent directories for the requested report path are created before the
temporary file is opened, which supports clean per-run evidence directories.
If a required parent path is occupied by a file, report emission fails as
unwritable rather than replacing or modifying that file.
Symlink report destinations are rejected as unwritable; qualification output
never follows a link or modifies its target.
Symlinked `HOST.json` inputs are rejected as unreadable so qualification does
not silently follow an unexpected evidence target.
Host input paths must be regular files; directories and special files are
rejected before any read operation begins.
Existing report destinations must be regular files; directories and other
special filesystem objects are rejected before temporary output is created.
The optional report path must differ from `HOST.json`; self-overwrite is
rejected before any output write begins.
The comparison uses resolved paths, so relative aliases of `HOST.json` are
protected as well.
Hard-link aliases are rejected through a same-file check, preventing a path
with a different name from overwriting the host evidence inode.
The temporary file is flushed and synchronized before replacement so a
successful validation report is durable across an abrupt process exit.
If replacement fails, the temporary file is removed so failed qualification
does not leave orphaned evidence artifacts beside the requested report.
Cleanup also covers failures while writing or synchronizing the temporary file
itself, before the destination can be replaced.
When replacing an existing report, its permission mode is preserved across the
atomic replacement where the host filesystem supports it.
On POSIX hosts the containing directory is synchronized after replacement as
well; Windows uses the compatible atomic-replace path without directory fsync.
Pass a second path to emit a deterministic validation report for the bundle:
`python tests/pipewire/validate_native_host_provenance.py HOST.json REPORT.json`.
The host validator's optional report output uses the same atomic temporary-file,
flush, and synchronization contract as the standalone report validator.
Its replacement path also preserves an existing report's permission mode on
POSIX hosts.
After host validation emits `REPORT.json`, run the standalone report validator
as a round-trip check before packaging the evidence bundle.
Validate that generated report before packaging with:
`python tests/pipewire/validate_native_host_provenance_report.py REPORT.json`.
The report validator accepts exactly one report path; missing or extra
arguments fail with the usage contract before any file is opened.
Reports with `valid: false` are rejected even when their other fields are
well-formed; failed qualification cannot be packaged as evidence.
The `valid` field must be the JSON boolean `true`, not a truthy number or
string.
The report schema identifier must exactly equal
`beatglass-native-host-provenance-v1`; missing, numeric, and older identifiers
are rejected.
Report names follow the host contract and must contain visible non-whitespace
text.
Report names must also be JSON strings; null, numeric, and array values are
rejected before packaging.
Report platforms must use the same exact lowercase enum as host evidence;
numeric, null, or padded values are rejected.
`checked_fields` must be a JSON array of non-empty strings; objects, scalars,
and null values are rejected as malformed report metadata.
An empty array is also rejected because every valid report must identify at
least its `platform` and `name` provenance fields.
The machine-readable schema encodes this minimum with `minItems: 2` in
addition to the executable validator checks.
Its platform enum is intentionally limited to `linux`, `windows`, and `macos`,
matching the host provenance validator.
Reports that list only one required field, or neither, are rejected for the
same reason; checked metadata must prove both identity dimensions.
Required field names are case-sensitive; display-cased variants such as
`Name` or `Platform` do not satisfy the contract.
The array must be lexicographically sorted to keep generated reports
deterministic and comparable across runs.
Every array entry must be non-empty; blank field names are rejected as
unverifiable metadata.
Whitespace-only entries are rejected as blank names as well.
Field names must also be unpadded; leading or trailing whitespace is rejected
instead of silently normalized.
The report shape is documented in
`tests/pipewire/native_host_provenance_report_schema.json`; reports must retain
both required provenance fields (`platform` and `name`) in `checked_fields`.
The list must also be sorted and contain each field only once; duplicate or
out-of-order fields are rejected before packaging.
Every checked-field entry must be a non-empty JSON string; malformed values are
rejected as invalid evidence rather than raising an internal validator error.
The generated report has a closed top-level shape; unknown report properties are
rejected so producers cannot silently bypass the qualification contract.
Unreadable files and malformed JSON are also rejected with a concise validation
error; they do not produce a traceback or a partially trusted report.
Files that are not valid UTF-8 are treated as unreadable and rejected by the
same check.
Duplicate JSON object keys are rejected as ambiguous evidence instead of being
silently overwritten by the parser.
If present, host `clock_source_latency_ns` must be a non-negative numeric
measurement of source latency; boolean values are invalid.
If present, host `clock_source_noise_ns` must be a non-negative numeric
measurement of source timing noise; boolean values are invalid.
If present, host `clock_source_phase_noise` must be a non-negative numeric
phase-noise measurement; boolean values are invalid.
If present, host `clock_source_frequency_hz` must be a positive numeric source
frequency; boolean values are invalid.
If present, host `clock_source_phase_offset_ns` must be numeric; boolean values
are invalid.
If present, host `clock_source_stability` must be a non-empty string describing
the selected source's stability classification.
On POSIX hosts this mode-preservation behavior is covered by an executable
permission regression test; Windows qualification uses the platform's native
permission semantics.
