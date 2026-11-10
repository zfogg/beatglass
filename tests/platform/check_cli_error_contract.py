"""Ensure README documents the shared shipped-CLI error contract."""
from pathlib import Path

readme = (Path(__file__).parents[2] / "README.md").read_text(encoding="utf-8")
for binary in ("beatglass-cli", "beatglass-music-cli"):
    if binary not in readme:
        raise SystemExit(f"README missing binary in error contract: {binary}")
for token in ("error: ", "status 1", "status 2"):
    if token not in readme:
        raise SystemExit(f"README missing error contract token: {token}")
print("CLI error contract verified")
