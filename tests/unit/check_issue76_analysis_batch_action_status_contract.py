"""Verify batch cancel/retry/discard availability is visible in the Library."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "analysisBatchActionStatus"')
    block = QML[start:start + 1200]
    for marker in ("cancel available", "retry or discard available", "Accessible.role: Accessible.Status", "retain source audio"):
        assert marker in block, marker
    print("Issue #76 analysis batch action status contract verified (4 checks)")

if __name__ == "__main__":
    main()
