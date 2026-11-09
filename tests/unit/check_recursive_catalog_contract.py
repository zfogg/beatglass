"""Executable contract for recursive library folder enumeration."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/library/catalog.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/library/catalog.cpp").read_text(encoding="utf-8")
BRIDGE = (ROOT / "src/desktop/library_catalog_bridge.hpp").read_text(encoding="utf-8")

assert "browse_recursive" in HEADER
assert "recursive_directory_iterator" in SOURCE
assert "disable_recursion_pending" in SOURCE
assert "browseRecursive" in BRIDGE
print("recursive catalog contract verified")
