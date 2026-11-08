from pathlib import Path
root = Path(__file__).resolve().parents[2]
header = (root / "include/beatglass/library/file_analysis_operation.hpp").read_text(encoding="utf-8")
source = (root / "lib/capture/file/file_analysis_operation.cpp").read_text(encoding="utf-8")
required = {
    "factory": "make_file_decode_operation",
    "worker reader": "FileReader reader(path)",
    "cancellation boundary": "FileReadStatus::cancelled",
    "source verification": "FileVerificationOutcome::cancelled",
    "serialized frame result": "decoded_frames=",
}
missing = [name for name, marker in required.items() if marker not in header + source]
if missing: raise SystemExit("Missing file analysis operation markers: " + ", ".join(missing))
print(f"File analysis operation contract verified ({len(required)} checks)")
