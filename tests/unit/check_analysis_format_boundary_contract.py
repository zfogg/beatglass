"""Ensure the desktop bridge rejects metadata-only formats before queueing work."""
from pathlib import Path

root = Path(__file__).parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
start = source.index("void submitAnalysis(")
end = source.index("void cancelAnalysis()", start)
block = source[start:end]
for marker in (
    "inspect_file(path.toStdString())",
    "!details->analysis_supported",
    "decoder does not support this audio format",
):
    assert marker in block, marker
assert block.index("inspect_file(path.toStdString())") < block.index("make_file_decode_operation"), "unsupported formats must be rejected before queueing"
