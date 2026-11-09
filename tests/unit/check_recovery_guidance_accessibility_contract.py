"""Source contract for persisted recovery guidance accessibility."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "analysisRecoveryGuidance"')
    end = QML.find("\n                            }", start)
    source = QML[start:end]
    for marker in (
        'Accessible.name: "Analysis recovery guidance"',
        "Accessible.value: text",
        "Accessible.role: Accessible.Status",
        "storageRecovery.status",
        "storageRecovery.retryableRunIds",
    ):
        assert marker in source, f"missing {marker!r}"
    print("recovery guidance accessibility contract: verified")

if __name__ == "__main__":
    main()
