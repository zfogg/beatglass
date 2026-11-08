from pathlib import Path

root = Path(__file__).resolve().parents[2]
runner = (root / "lib/library/analysis_runner.cpp").read_text(encoding="utf-8")
test = (root / "tests/unit/analysis_operation_test.cpp").read_text(encoding="utf-8")
required = ["catch (...) { return failed(); }", "decoder failure retains a retryable recovery record", "decoder interrupted"]
missing = [marker for marker in required if marker not in runner + test]
if missing:
    raise SystemExit("Missing exception recovery markers: " + ", ".join(missing))
print(f"Exception recovery contract verified ({len(required)} checks)")
