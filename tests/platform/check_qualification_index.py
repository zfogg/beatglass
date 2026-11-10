"""Keep the README qualification-evidence index complete."""
from pathlib import Path

root = Path(__file__).parents[2]
readme = (root / "README.md").read_text(encoding="utf-8")
for link in ("docs/progress.md", "docs/platform-qualification-status.md", "docs/windows-qualification.md", "docs/macos-qualification.md", "docs/linux-qualification-plan.md"):
    if link not in readme:
        raise SystemExit(f"README is missing qualification index link: {link}")
print("README qualification index verified")
