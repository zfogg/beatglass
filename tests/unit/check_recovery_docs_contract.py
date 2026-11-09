from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["metadata-gated", "coordinator resubmission boundary", "native qualification", "WASAPI", "Core Audio", "PipeWire"]
missing = [marker for marker in required if marker not in text]
if missing: raise SystemExit("Missing recovery documentation markers: " + ", ".join(missing))
print(f"Recovery documentation contract verified ({len(required)} checks)")
