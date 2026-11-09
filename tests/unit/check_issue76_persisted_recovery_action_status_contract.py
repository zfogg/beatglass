"""Verify persisted recovery action availability is visible in the Library."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "persistedRecoveryActionStatus"')
    block = QML[start:start + 1400]
    for marker in ("retry available", "reopen storage", "active work", "Accessible.role: Accessible.Status", "Retry and discard never modify source audio"):
        assert marker in block, marker
    print("Issue #76 persisted recovery action status contract verified (5 checks)")

if __name__ == "__main__":
    main()
