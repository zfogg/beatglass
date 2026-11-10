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
The readiness checker enforces the same symlink-free parent-path rule for its
`--report` output.
