"""Contract checks for independent native evidence archive verification."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "verify_native_evidence_archive.py").read_text(encoding="utf-8")
markers = ("zipfile", "evidence-manifest.json", "qualified", "size", "sha256", "digest mismatch", "unsafe archive entry", "duplicate archive entry", "unlisted archive artifact", "manifest commit does not match", "contains no artifacts", "no commit identity", "reproducibility metadata", "malformed archive timestamp", "no JUnit results", "no CTest discovery JSON", "no tests list", "no registered tests", "unnamed test", "invalid root", "no executed tests", "failures or errors", "skipped tests", "unnamed testcases", "duplicate testcase names", "duplicate test names", "names do not match", "native_test_count does not match", "native test-name match", "summary is not qualified", "native_junit_test_count does not match", "summary reports JUnit failures", "native JUnit existence", "no native test discovery record", "summary native discovery digest", "summary native JUnit digest", "native discovery count does not match")
missing = [marker for marker in markers if marker not in source]
if missing:
    raise SystemExit(f"missing archive-verifier contract markers: {missing}")
print(f"PipeWire evidence archive verifier contract passed ({len(markers)} checks)")
