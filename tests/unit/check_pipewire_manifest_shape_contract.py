import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[2] / "tests/pipewire"))
from manifest_contract import validate_inventory_shape

assert validate_inventory_shape([])
assert validate_inventory_shape([{"id": 1}])
assert validate_inventory_shape(["bad"])
print("Shared PipeWire inventory shape verified (3 checks)")
