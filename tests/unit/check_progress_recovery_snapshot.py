from pathlib import Path

source = Path("docs/progress.md").read_text()
for phrase in ("empty publication", "oversized publication", "native Windows, macOS, and Linux qualification"):
    if phrase not in source:
        raise SystemExit(f"progress snapshot missing: {phrase}")
print("progress recovery snapshot verified")
