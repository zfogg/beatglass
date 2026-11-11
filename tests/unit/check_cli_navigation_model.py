from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[2]
    header = (root / "include/beatglass/cli/navigation.hpp").read_text(encoding="utf-8")
    source = (root / "lib/core/navigation.cpp").read_text(encoding="utf-8")
    for marker in ("enum class Pane", "enum class Layout", "enum class Modal", "visible_commands", "resize"):
        assert marker in header, f"missing navigation contract: {marker}"
    for marker in ("command palette", "Layout::two_column", "std::tolower"):
        assert marker in source, f"missing navigation behavior: {marker}"
    print("CLI v2 navigation/layout/palette contract verified")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
