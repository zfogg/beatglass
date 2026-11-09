from pathlib import Path


docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-design-system-overview.svg").read_text(encoding="utf-8")

assert "overlay-design-system-overview.svg" in docs
for marker in ("STATUS", "SETTINGS", "ACTIONS", "ANNOUNCEMENTS", "retained-settings summary"):
    assert marker in svg, marker
print("overlay design system overview contract: ok")
