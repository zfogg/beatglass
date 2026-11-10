"""Verify latest published source action exposes actionable status."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    marker = 'objectName: "selectLatestPublishedSource"'
    start = QML.index(marker)
    block = QML[start : start + 900]
    assert "activeFocusOnTab: true" in block
    assert 'Accessible.value: enabled ? "available; " + window.analysisJobBridge.batchLastSource' in block
    print("Issue #76 latest source status contract verified")


if __name__ == "__main__":
    main()
