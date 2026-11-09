"""Source contract for DJ telemetry statuses."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for marker in ("DJ monitoring status:", 'Accessible.name: "DJ estimates"'):
        start = QML.index(marker)
        source = QML[start:start + 350]
        assert "Accessible.value:" in source, marker
        assert "Accessible.role: Accessible.Status" in source, marker
    print("DJ telemetry accessibility contract: 2 statuses verified")

if __name__ == "__main__":
    main()
