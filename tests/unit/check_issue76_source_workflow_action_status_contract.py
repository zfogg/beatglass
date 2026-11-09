"""Verify selected source status exposes actionable analysis state."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    for marker in ("objectName: \"sourceWorkflowStatus\"", "analysis: ", "in progress", "status === \"Complete\"", "retry available"):
        assert marker in QML, marker
    print("Issue #76 source workflow action status contract verified (5 checks)")
if __name__ == "__main__": main()
