from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["cache hits complete without decoder work", "missing or mismatched metadata rejects fail-closed", "check_bridge_resubmit_contract.py", "native decoder/runtime"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing retry decision markers: " + ", ".join(missing))
print(f"Retry decision documentation verified ({len(required)} checks)")
