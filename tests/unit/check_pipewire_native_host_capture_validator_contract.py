"""Contract checks for native host capture plan validation."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_native_host_capture_plan.py").read_text(encoding="utf-8")
for marker in ("beatglass-native-host-capture-plan-v1", "fabricated", "native host capture plan is incomplete", '"Linux"', '"Windows"', '"Darwin"'):
    if marker not in source: raise SystemExit(f"missing capture-plan validator marker: {marker}")
print("PipeWire native host capture validator contract passed (6 checks)")
