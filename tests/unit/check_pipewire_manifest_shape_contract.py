import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[2] / "tests/pipewire"))
from manifest_contract import validate_inventory_shape

assert validate_inventory_shape([])
assert validate_inventory_shape([{"id": 1}])
assert validate_inventory_shape([{"id": 1, "info": {"props": {"media.class": "Audio/Sink"}}}, {"id": 1, "info": {"props": {}}}])
assert validate_inventory_shape(["bad"])
assert validate_inventory_shape([{"id": 1}])
assert validate_inventory_shape([{ "id": 1, "info": {"props": {"media.class": "Audio/Sink", "object.serial": "1", "node.name": "test"}}}]) is None
print("Shared PipeWire inventory shape verified (4 checks)")
