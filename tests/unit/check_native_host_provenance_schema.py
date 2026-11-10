import json
from pathlib import Path

schema = json.loads(Path("tests/pipewire/native_host_provenance_schema.json").read_text())
assert schema["title"] == "Beatglass native host provenance"
assert schema["required"] == ["platform", "name"]
assert schema["properties"]["platform"]["enum"] == ["linux", "windows", "macos"]
assert schema["properties"]["name"]["minLength"] == 1
assert schema["properties"]["node_id"]["minimum"] == 1
assert schema["properties"]["node_id"]["type"] == "integer"
assert schema["properties"]["secure_boot"]["type"] == "boolean"
assert schema["properties"]["tpm_present"]["type"] == "boolean"
assert schema["additionalProperties"] is True
assert schema["required"] == ["platform", "name"]
assert schema["properties"]["node_id"]["minimum"] == 1
assert schema["properties"]["secure_boot"]["type"] == "boolean"
assert schema["properties"]["clock_accuracy_ns"]["minimum"] == 0
assert schema["properties"]["clock_accuracy_ns"]["type"] == "integer"
assert schema["properties"]["clock_source_count"]["type"] == "integer"
assert schema["properties"]["clock_source_count"]["minimum"] == 1
print("Native host provenance schema verified")
