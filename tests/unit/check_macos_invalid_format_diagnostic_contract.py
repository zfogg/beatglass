from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert 'std::string diagnostic = "Invalid capture format"' in SOURCE
    assert '"; backend stop also failed"' in SOURCE
    assert "status_ = {SourceState::error, SourceError::backend_error, std::move(diagnostic)}" in SOURCE
    print("macOS invalid-format diagnostic contract verified (3 checks)")

if __name__ == "__main__":
    main()
