import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[2] / "tests/pipewire"))
from manifest_contract import audio_sink_count, inventory_digest

inventory = [{"id": 1, "info": {"props": {"media.class": "Audio/Sink"}}}]
assert audio_sink_count(inventory) == 1
assert len(inventory_digest(inventory)) == 64
assert inventory_digest([{"b": 2, "a": 1}]) == inventory_digest([{"a": 1, "b": 2}])
print("Shared PipeWire manifest contract verified (3 checks)")
