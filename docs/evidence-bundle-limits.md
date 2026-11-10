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
