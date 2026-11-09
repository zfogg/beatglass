"""Source contract for persisted analysis lifecycle diagnostics."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def block(name: str) -> str:
    start = QML.index(f'objectName: "{name}"')
    return QML[start:QML.find("\n                            }", start)]

def main() -> None:
    for name in ("analysisBatchLastResult", "persistedRecoveryStatus", "persistedRecoveryRejectionReason", "analysisAdmissionStatus", "persistedAnalysisProgress"):
        source = block(name)
        assert "Accessible.value:" in source, name
        assert "Accessible.role: Accessible.Status" in source, name
    print("persisted progress accessibility contract: 5 diagnostics verified")

if __name__ == "__main__":
    main()
