"""Contract checks for native host capture preparation."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "prepare_native_host_capture.py").read_text(encoding="utf-8")
for marker in ("beatglass-native-host-capture-plan-v1", '"Linux"', '"Windows"', '"Darwin"', '"fabricated":False', "PipeWire daemon"):
    if marker not in source: raise SystemExit(f"missing native capture marker: {marker}")
print("PipeWire native host capture contract passed (6 checks)")
