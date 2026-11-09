"""Source contract for visualization recovery and handoff statuses."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for marker in ("Visualization interaction hint", "Visualization selection reset status", "Visualization analysis handoff:", "Visualization playback epoch feedback:"):
        start = QML.index(marker)
        source = QML[start:start + 500]
        assert "Accessible.value:" in source, marker
        assert "Accessible.role: Accessible.Status" in source, marker
    print("visualization recovery status contract: 4 statuses verified")

if __name__ == "__main__":
    main()
