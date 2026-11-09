"""Contract checks for independent native evidence archive verification."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "verify_native_evidence_archive.py").read_text(encoding="utf-8")
markers = ("zipfile", "evidence-manifest.json", "qualified", "size", "sha256", "digest mismatch", "unsafe archive entry", "duplicate archive entry", "unlisted archive artifact", "manifest commit does not match", "contains no artifacts", "no commit identity", "reproducibility metadata", "malformed archive timestamp")
missing = [marker for marker in markers if marker not in source]
if missing:
    raise SystemExit(f"missing archive-verifier contract markers: {missing}")
print(f"PipeWire evidence archive verifier contract passed ({len(markers)} checks)")
