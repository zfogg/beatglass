# Evidence bundle resource limits

`tests/platform/verify_evidence_bundle.py` rejects oversized manifests and
artifacts before parsing or hashing them. Defaults are 16 MiB per manifest,
1 GiB per retained artifact, and 1,000 artifact/scenario entries.

Qualification tooling can tighten these limits for constrained hosts with:

```text
python tests/platform/verify_evidence_bundle.py --root BUNDLE --manifest BUNDLE/manifest.json \
  --max-manifest-bytes 4194304 --max-artifact-bytes 268435456 --max-entries 100
```

All limits must be positive. This is a resource-safety policy only; it does
not replace native qualification or evidence integrity checks.

The ingest step uses the same policy before hashing native artifacts and before
writing its generated manifest. Pass the same three options to
`tests/platform/ingest_qualification_artifacts.py` to keep generation and
verification budgets aligned.

Ingestion also enforces the verifier's portable manifest contract: artifact
hashes are lowercase hexadecimal, and paths reject non-portable separators,
reserved characters, and invalid Unicode before files are hashed.

Identity fields are canonicalized at ingestion as well: platform names must
be supported lowercase values, commits must be lowercase SHA-1 values, and
environment strings must be trimmed, bounded, and free of control characters.
Capture timestamps must include a timezone and cannot be more than five minutes
in the future, preventing stale clock metadata from entering the bundle.
Valid timestamps are emitted canonically as UTC ISO-8601 values with a `Z`
suffix so manifests from different hosts compare consistently.
Generated manifests are written through a same-directory temporary file,
flushed, and atomically replaced so interrupted ingestion cannot leave a
partially written manifest.
The identity and hash-list inputs are bounded to 64 KiB and 16 MiB
respectively, preventing untrusted qualification directories from forcing
unbounded text reads before artifact validation begins.
Ingestion checks every output ancestor for symlink traversal before creating
missing nested report directories.
The verifier also rejects symlinked bundle roots and manifest inputs, keeping
validation anchored to the intended evidence workspace.
Ingestion also supports `--json`, returning validity, errors, output, and
effective limits for orchestration without parsing human-readable logs.
Both JSON interfaces include `schema_version: 1`; consumers should reject
unknown versions rather than guessing at field semantics.
Ingestion JSON also reports generated `artifact_count` and `artifact_bytes`
metrics for orchestration and audit logging.
Its `error_categories` map provides stable counts for ingestion dashboards,
matching verifier JSON diagnostics.
Artifact inventories also use case-insensitive collision keys so manifests
cannot contain two names that alias on Windows or other case-folding filesystems.
Referenced evidence fields use the same portable path checks as hashed
artifacts, so scenario-linked files cannot bypass the canonical path policy.
Referenced evidence also consumes the same per-artifact byte budget before it
is hashed into the generated manifest.
Ingestion additionally enforces an aggregate artifact budget (8 GiB by
default), configurable with `--max-total-artifact-bytes`, to bound total disk
and hashing work across a bundle.
The verifier applies the same aggregate budget, keeping generated manifests
and independently checked bundles under one resource policy.
Automation can pass `--json` to receive a stable object containing `valid` and
`errors` fields on stdout while retaining the normal exit status contract.
Readiness reports are also written atomically, preventing interrupted
qualification summaries from replacing a previously complete report.
The standalone readiness report generator uses the same atomic output
contract, so all report-producing qualification tools are interruption-safe.
Its report size is capped at 16 MiB by default and can be tightened with
`--max-report-bytes` before any output file is created.
The generator accepts `--json` to mirror the report to stdout for automation
while still retaining the durable output file.
Generated readiness reports expose generation error counts and categories for
dashboard consumers, matching the ingestion and verifier JSON contracts.
They also expose aggregate retained artifact count and byte totals for quick
qualification-volume checks without traversing each platform row.
Readiness output is capped at 16 MiB by default; constrained environments can
override this with `--max-report-bytes`.
Automation can pass `--json` to receive the complete readiness object on
stdout while retaining the normal readiness exit status.
The JSON object also includes `error_count` and the effective `limits`, making
resource policy observable to qualification automation.
Verifier JSON also reports retained `artifact_count` and `artifact_bytes`
metrics, enabling consumers to compare evidence volume with configured limits.
Errors include an `error_categories` count map for dashboards without requiring
clients to parse human-readable diagnostic strings.
Report generators reject symlinked output parents so a qualification command
cannot redirect a report outside its intended workspace.
The standalone generator checks every ancestor component, including missing
nested directories, before creating report output.
They also reject an existing symlink at the output path itself, preserving the
target and requiring explicit replacement of regular files only.
The readiness checker enforces the same symlink-free parent-path rule for its
`--report` output.
It also refuses to replace an existing symlink at that output path.
All ancestor components are checked, including missing nested directories that
would otherwise be created beneath a symlinked workspace component.
Its `--max-age-hours` value must be positive, preventing accidental
configuration that marks every retained bundle stale.
Manifest byte-limit violations are grouped under `manifest_size` in JSON
output for direct input-capacity triage.
Artifact-count limit violations are grouped under `entry_count` for direct
bundle-cardinality triage.
Unsupported artifact fields are grouped under `artifact_schema` for direct
manifest-contract triage.
Unsupported top-level manifest fields are grouped under `manifest_schema` for
direct bundle-contract triage.
Missing timestamp timezone information is grouped under `clock` for direct
capture-clock discipline triage.
Malformed timestamp values are grouped under `timestamp_format` for direct
capture-time serialization triage.
Future capture timestamps are grouped under `freshness` for direct evidence
age and clock-skew triage.
Timestamp format, timezone, and future-capture failures are grouped under
`temporal` in JSON output for clear freshness and clock-discipline reporting.
Missing retained evidence for a required scenario is grouped under
`scenario_evidence` for direct coverage triage.
Malformed manifest JSON is grouped under `parse`, keeping input decoding
failures distinct from valid-but-invalid evidence contracts.
Duplicate or conflicting artifact identities are grouped under `identity` in
JSON output for direct manifest deduplication triage.
Traversal, separator, and canonicalization failures are grouped under `path`
for direct workspace-boundary triage.
Artifact paths escaping the evidence root are grouped under `artifact_path`
for direct retained-file boundary triage.
Missing retained artifact files are grouped under `artifact_missing` for direct
evidence-retention triage.
Non-regular artifact entries are grouped under `artifact_type` for direct
retained-file type triage.
Symlinked artifact entries are grouped under `artifact_symlink` for direct
retained-file indirection triage.
Symlinked bundle roots are grouped under `bundle_symlink` for direct workspace
boundary triage.
Symlinked manifest inputs are grouped under `manifest_symlink` for direct
manifest-boundary triage.
Non-regular manifest inputs are grouped under `manifest_type` for direct
manifest-input type triage.
Unsupported platform identifiers are grouped under `platform` for direct
qualification-environment triage.
Environment descriptor length, whitespace, and control-character failures
are grouped under `environment` for direct runtime-identity triage.
Commit identifier format failures are grouped under `commit` for direct build
provenance triage.
Invalid scenario status values are grouped under `status` for direct result
state triage.
Malformed scenario evidence references are grouped under `scenario_reference`
for direct retained-evidence naming triage.
Malformed scenario evidence paths are grouped under `scenario_path` for direct
scenario-boundary triage.
Unsupported scenario fields are grouped under `scenario_schema` for direct
manifest-contract triage.
Shared scenario evidence ownership is grouped under `scenario_identity` for
direct coverage and deduplication triage.
Missing or mismatched evidence roles are grouped under `role` for direct
artifact-to-scenario contract triage.
Artifact role format and size failures are grouped under `artifact_role` for
direct retained-artifact metadata triage.
SHA-256 mismatches are grouped under `hash` for direct retained-file
integrity triage.
Malformed SHA-256 declarations are grouped under `hash_format` for direct
manifest-integrity triage.
Per-artifact byte-limit violations are grouped under `size` for direct
retained-file capacity triage.
The verifier also rejects symlinked bundle roots and manifest inputs, and
requires the root to be a directory and manifest to be a regular file before
parsing.
JSON output groups these pre-parse failures under `input_topology`, allowing
qualification dashboards to distinguish workspace setup errors from content
validation failures.
JSON output also groups manifest, artifact-count, and byte-budget violations
under `resource_limit`, making capacity failures distinct from malformed data.
Manifest contract failures such as missing provenance and required scenarios
are grouped under `schema`, keeping evidence-shape failures distinct from
workspace topology and capacity problems.
Missing provenance metadata is grouped under `provenance` in JSON output, so
qualification dashboards can identify incomplete build/environment identity
separately from other manifest-shape errors.
Required-scenario omissions are grouped under `scenario` in JSON output, so
qualification dashboards can distinguish incomplete test coverage from other
schema failures.
Artifact reference, hash, and size failures are grouped under `artifact` in
JSON output, keeping retained-file integrity problems easy to triage.
