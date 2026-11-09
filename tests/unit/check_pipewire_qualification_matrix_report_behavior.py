"""Exercise actionable Markdown qualification report rendering."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

renderer = Path(__file__).parents[1] / "pipewire" / "render_qualification_matrix_report.py"
matrix = {"schema":"beatglass-native-qualification-matrix-v1", "host_os":"Windows", "platforms": {
    "Linux":{"status":"pending","evidence":[],"pending_reason":"Linux host required","native_execution_attempted":False,"execution_blockers":["Linux host","PipeWire daemon"]},
    "Windows":{"status":"qualified","evidence":["windows.zip"],"native_execution_attempted":True},
    "Darwin":{"status":"pending","evidence":[],"pending_reason":"macOS host required"}}}
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    source, report = root / "matrix.json", root / "matrix.md"
    source.write_text(json.dumps(matrix), encoding="utf-8")
    result = subprocess.run([sys.executable, str(renderer), str(source), str(report)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    rendered = report.read_text(encoding="utf-8")
    for marker in ("Native qualification matrix", "Native attempt", "Blockers", "Linux", "Windows", "qualified", "windows.zip", "Linux host required", "PipeWire daemon"):
        if marker not in rendered:
            raise SystemExit(f"missing rendered report marker: {marker}")
    matrix["schema"] = "invalid"
    source.write_text(json.dumps(matrix), encoding="utf-8")
    result = subprocess.run([sys.executable, str(renderer), str(source), str(report)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid qualification matrix schema" not in result.stderr + result.stdout:
        raise SystemExit("invalid matrix schema was rendered")
print("PipeWire qualification matrix report behavior passed (2 scenarios)")
