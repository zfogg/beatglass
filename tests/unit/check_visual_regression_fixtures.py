"""Validate the deterministic v2 GUI/CLI visual-regression fixture contract."""
import json
from pathlib import Path


root = Path(__file__).parents[2]
manifest_path = root / "design" / "v2" / "visual-regression.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
fixtures = manifest.get("fixtures", [])
if manifest.get("schemaVersion") != 1 or manifest.get("engine") != "playwright":
    raise SystemExit("visual regression manifest has an unsupported schema or engine")
if len(fixtures) != 10:
    raise SystemExit(f"expected 10 v2 fixtures, found {len(fixtures)}")

ids = [fixture.get("id") for fixture in fixtures]
if len(ids) != len(set(ids)) or any(not value for value in ids):
    raise SystemExit("fixture IDs must be unique and non-empty")
surfaces = {fixture.get("surface") for fixture in fixtures}
if surfaces != {"gui", "cli"}:
    raise SystemExit(f"expected GUI and CLI fixtures, found {sorted(surfaces)}")
for fixture in fixtures:
    viewport = fixture.get("viewport")
    if not isinstance(viewport, list) or len(viewport) != 2 or any(not isinstance(value, int) or value <= 0 for value in viewport):
        raise SystemExit(f"invalid viewport for {fixture['id']}")
    if not fixture.get("baseline", "").startswith("baselines/"):
        raise SystemExit(f"baseline for {fixture['id']} must be relative to baselines/")
threshold = manifest.get("defaultThreshold", {})
if threshold.get("maxDiffPixels", 0) <= 0 or not 0 < threshold.get("maxDiffPixelRatio", 0) < 1:
    raise SystemExit("default image-diff thresholds must be positive and bounded")
missing = [fixture["baseline"] for fixture in fixtures if not (manifest_path.parent / fixture["baseline"]).exists()]
print(f"visual regression fixtures: {len(fixtures)} defined; {len(missing)} baselines pending")
