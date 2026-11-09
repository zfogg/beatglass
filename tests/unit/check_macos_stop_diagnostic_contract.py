from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert 'status_.message = stopped ? std::string{} : "Core Audio backend failed to quiesce"' in SOURCE
    assert "quiescence_failed_ = !stopped" in SOURCE
    print("macOS stop diagnostic contract verified (2 checks)")

if __name__ == "__main__":
    main()
