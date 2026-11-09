from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = Path("docs/overlay-design-system.md").read_text(encoding="utf-8")
svg = Path("docs/overlay-design-tokens.svg").read_text(encoding="utf-8")

for marker in ("Review recovery", "Restore overlay", "Retry overlay", "Accessible.name", "Accessible.value"):
    assert marker in docs or marker in qml, marker
assert "overlay-design-tokens.svg" in docs
assert "#4ADE80" in svg and "#FBBF24" in svg and "#FB7185" in svg
print("overlay design system contract: ok")
