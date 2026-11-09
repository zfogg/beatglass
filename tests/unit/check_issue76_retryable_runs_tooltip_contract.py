"""Verify persisted recovery selection exposes a stable recovery tooltip."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "persistedRecoveryRuns"')
    block = QML[start:start + 1800]
    assert 'ToolTip.text: count > 0' in block
    assert '"Choose a saved analysis run to recover"' in block
    assert 'storageRecovery.selectedRunId' in block
    assert 'storageRecovery.selectRun(currentText)' in block
    print("Issue #76 retryable-runs tooltip contract verified (4 checks)")

if __name__ == "__main__":
    main()
