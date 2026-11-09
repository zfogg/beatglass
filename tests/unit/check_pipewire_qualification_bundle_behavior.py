"""Exercise matrix/report bundle validation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

root_dir = Path(__file__).parents[1]
matrix_tool = root_dir / "pipewire" / "qualification_matrix.py"
report_tool = root_dir / "pipewire" / "render_qualification_matrix_report.py"
validator = root_dir / "pipewire" / "validate_qualification_bundle.py"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory); matrix = root / "matrix.json"; report = root / "matrix.md"
    subprocess.run([sys.executable, str(matrix_tool), str(matrix)], check=True)
    subprocess.run([sys.executable, str(report_tool), str(matrix), str(report)], check=True)
    result = subprocess.run([sys.executable, str(validator), str(matrix), str(report)], capture_output=True, text=True)
    if result.returncode != 0: raise SystemExit(result.stderr or result.stdout)
    report.write_text(report.read_text(encoding="utf-8").replace("| Linux |", "| Missing |"), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(matrix), str(report)], capture_output=True, text=True)
    if result.returncode == 0 or "missing platform" not in result.stderr + result.stdout:
        raise SystemExit("incomplete qualification report was accepted")
print("PipeWire qualification bundle behavior passed (2 scenarios)")
