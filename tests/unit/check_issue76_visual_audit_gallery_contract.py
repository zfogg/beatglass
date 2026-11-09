from pathlib import Path


audit = Path("docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
assert "overlay-design-gallery.md" in audit
assert "review evidence" in audit.lower()
assert "native desktop" in audit.lower()
print("issue 76 visual audit gallery contract: ok")
