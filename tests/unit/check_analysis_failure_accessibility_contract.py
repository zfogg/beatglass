"""Source contract for live analysis failure and recovery diagnostics."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def block(object_name: str) -> str:
    start = QML.index(f'objectName: "{object_name}"')
    end = QML.find("\n                            }", start)
    assert end != -1, object_name
    return QML[start:end]

def main() -> None:
    expectations = {
        "analysisFailureReason": ["Accessible.value: visible ? text : \"\"", "Accessible.role: Accessible.Status", "failureReason"],
        "analysisRecoveryStatus": ["Accessible.value: visible ? text : \"\"", "Accessible.role: Accessible.Status", "recoveryStatus"],
        "analysisQueueError": ["Accessible.value: visible ? text : \"\"", "Accessible.role: Accessible.Status", "analysisError"],
    }
    for object_name, markers in expectations.items():
        source = block(object_name)
        for marker in markers:
            assert marker in source, f"{object_name} missing {marker!r}"
    print(f"analysis failure accessibility contract: {len(expectations)} diagnostics verified")

if __name__ == "__main__":
    main()
