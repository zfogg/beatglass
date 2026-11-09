"""Contract checks for actionable qualification matrix report rendering."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "render_qualification_matrix_report.py").read_text(encoding="utf-8")
for marker in ("Native qualification matrix", "Platform", "Status", "Evidence", "Pending reason", "invalid qualification matrix schema"):
    if marker not in source:
        raise SystemExit(f"missing matrix report marker: {marker}")
print("PipeWire qualification matrix report contract passed (6 checks)")
