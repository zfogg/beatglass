from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/run_linux_native_qualification.py").read_text(encoding="utf-8")
for marker in ("check_linux_native_prerequisites.py", "BEATGLASS_BUILD_PIPEWIRE=ON", "BUILD_TESTING=ON", "^pipewire_tests$", "ctest", "capture_pipewire_manifest.py", "qualified", "Linux native qualification pending"):
    assert marker in source
print("PipeWire native runner contract verified (5 checks)")
