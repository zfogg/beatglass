from pathlib import Path
root = Path(__file__).resolve().parents[2]
header = (root / "include/beatglass/library/file_analysis_operation.hpp").read_text(encoding="utf-8")
source = (root / "lib/capture/file/file_analysis_operation.cpp").read_text(encoding="utf-8")
runner = (root / "lib/library/analysis_runner.cpp").read_text(encoding="utf-8")
required = {
    "factory": "make_file_decode_operation",
    "worker reader": "FileReader reader(path)",
    "cancellation boundary": "FileReadStatus::cancelled",
    "source verification": "FileVerificationOutcome::cancelled",
    "source availability": "is_regular_file(path, ec)",
    "decode handoff": "op.decode = [path]",
    "serialized frame result": "decoded_frames=",
    "wrapped progress handoff": "op.decode = [path](std::stop_token stop, const auto& progress)",
}
missing = [name for name, marker in required.items() if marker not in header + source + runner]
if missing: raise SystemExit("Missing file analysis operation markers: " + ", ".join(missing))
print(f"File analysis operation contract verified ({len(required)} checks)")
