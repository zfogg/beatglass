import json
from pathlib import Path

schema = json.loads(Path("tests/pipewire/native_host_provenance_schema.json").read_text())
assert schema["required"] == ["platform", "name"]
assert schema["properties"]["node_id"]["minimum"] == 1
assert schema["properties"]["secure_boot"]["type"] == "boolean"
assert schema["properties"]["clock_accuracy_ns"]["minimum"] == 0
print("Native host provenance schema verified")
