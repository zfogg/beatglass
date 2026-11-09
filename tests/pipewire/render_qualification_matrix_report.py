"""Render a validated qualification matrix as an actionable Markdown report."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("usage: render_qualification_matrix_report.py MATRIX.json REPORT.md")
matrix = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
if matrix.get("schema") != "beatglass-native-qualification-matrix-v1":
    raise SystemExit("invalid qualification matrix schema")
lines = ["# Native qualification matrix", "", f"Host: `{matrix.get('host_os', 'unknown')}`", "", "| Platform | Status | Evidence | Pending reason |", "| --- | --- | --- | --- |"]
for name, entry in matrix["platforms"].items():
    evidence = ", ".join(f"`{item}`" for item in entry.get("evidence", [])) or "—"
    reason = entry.get("pending_reason", "—")
    lines.append(f"| {name} | **{entry['status']}** | {evidence} | {reason} |")
Path(sys.argv[2]).write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"Rendered qualification matrix report to {sys.argv[2]}")
