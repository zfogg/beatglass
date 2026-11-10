from pathlib import Path
import json

schema = json.loads(Path("tests/pipewire/native_host_provenance_report_schema.json").read_text(encoding="utf-8"))
assert schema["required"] == ["schema", "valid", "platform", "name", "checked_fields"]
assert schema["properties"]["schema"]["const"] == "beatglass-native-host-provenance-v1"
assert schema["properties"]["valid"]["const"] is True
assert schema["properties"]["checked_fields"]["uniqueItems"] is True
assert schema["additionalProperties"] is False
print("Native host provenance report schema verified")
