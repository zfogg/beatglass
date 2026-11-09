from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert '"Core Audio capture start failed"' in SOURCE
    assert '"; backend stop also failed"' in SOURCE
    assert "status_ = {SourceState::error, error, std::move(diagnostic)}" in SOURCE
    print("macOS start-failure diagnostic contract verified (3 checks)")

if __name__ == "__main__":
    main()
