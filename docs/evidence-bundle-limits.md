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
Artifact inventories also use case-insensitive collision keys so manifests
cannot contain two names that alias on Windows or other case-folding filesystems.
Referenced evidence fields use the same portable path checks as hashed
artifacts, so scenario-linked files cannot bypass the canonical path policy.
