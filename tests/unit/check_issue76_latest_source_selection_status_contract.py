"""Verify selecting the latest source exposes a persistent status."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    assert 'property string latestPublishedSourceSelectionStatus: ""' in QML
    assert 'objectName: "latestPublishedSourceSelectionStatus"' in QML
    assert 'window.latestPublishedSourceSelectionStatus = "Selected latest published source: " + window.selectedName' in QML
    print("Issue #76 latest source selection status contract verified")


if __name__ == "__main__":
    main()
