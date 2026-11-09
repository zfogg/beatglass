from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "bool format_query_failed = false" in SOURCE
    assert "format_query_failed = true" in SOURCE
    assert '"Core Audio format query failed"' in SOURCE
    print("macOS format-query failure contract verified (3 checks)")

if __name__ == "__main__":
    main()
