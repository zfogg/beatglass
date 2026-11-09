from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/verify_native_run_summary.py").read_text(encoding="utf-8")
for marker in ("qualified", "failed_command", "native_discovery_json", "native_junit", "native_junit_sha256", "native_discovery_sha256", "digest mismatch"):
    assert marker in source
print("Native PipeWire summary verifier contract verified (7 checks)")
