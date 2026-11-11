"""Validate v2 fixture-to-native-scenario traceability without claiming qualification."""
import json
from pathlib import Path


root = Path(__file__).parents[2]
fixture_path = root / "design" / "v2" / "visual-regression.json"
matrix_path = root / "docs" / "v2-qualification-matrix.json"
fixtures = json.loads(fixture_path.read_text(encoding="utf-8"))["fixtures"]
matrix = json.loads(matrix_path.read_text(encoding="utf-8"))
rows = matrix.get("fixtures", [])
fixture_ids = [item["id"] for item in fixtures]
row_ids = [item.get("id") for item in rows]
if matrix.get("schemaVersion") != 1:
    raise SystemExit("unsupported v2 qualification matrix schema")
if row_ids != fixture_ids:
    raise SystemExit("qualification rows must cover visual fixtures in manifest order")
for row in rows:
    if row.get("surface") not in {"gui", "cli"}:
        raise SystemExit(f"invalid surface for {row['id']}")
    if row.get("status") not in {"design-only", "pending", "qualified"}:
        raise SystemExit(f"invalid qualification status for {row['id']}")
    if row["status"] != "qualified" and row.get("evidence"):
        raise SystemExit(f"non-qualified row has evidence: {row['id']}")
    if row["status"] == "qualified" and not row.get("evidence"):
        raise SystemExit(f"qualified row has no evidence: {row['id']}")
print(f"v2 qualification matrix: {len(rows)} fixture mappings; design-only={sum(row['status'] == 'design-only' for row in rows)}; qualified={sum(row['status'] == 'qualified' for row in rows)}")
