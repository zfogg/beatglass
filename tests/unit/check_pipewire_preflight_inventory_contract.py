import importlib.util
from pathlib import Path

path = Path(__file__).parents[2] / "tests/pipewire/preflight_inventory.py"
spec = importlib.util.spec_from_file_location("pipewire_preflight", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

valid = [{"id": 42, "info": {"props": {
    "media.class": "Audio/Sink", "object.serial": "1001", "node.name": "alsa_output.test"
}}}]
assert module.validate_sink_inventory(valid) is None
assert "numeric" in module.validate_sink_inventory([{**valid[0], "id": -1}])
assert "unique" in module.validate_sink_inventory(valid + valid)
assert "non-empty" in module.validate_sink_inventory([{**valid[0], "info": {"props": {
    "media.class": "Audio/Sink", "object.serial": "", "node.name": "alsa_output.test"
}}}])
assert module.validate_sink_inventory({}) == "valid pw-dump JSON list"
print("PipeWire preflight inventory contract verified (5 checks)")
