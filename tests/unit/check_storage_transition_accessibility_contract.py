"""Source contract for storage transition and persisted outcome statuses."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def block(name: str) -> str:
    start = QML.index(f'objectName: "{name}"')
    end = QML.find("\n                            }", start)
    return QML[start:end]

def main() -> None:
    for name, dependency in (("storageTransitionStatus", "storageRecovery.status"), ("analysisRetryOutcome", "retryableRunIds"), ("analysisStaleState", "analysisStatus")):
        source = block(name)
        for marker in ("Accessible.value:", "Accessible.role: Accessible.Status", dependency):
            assert marker in source, f"{name} missing {marker!r}"
    print("storage transition accessibility contract: 3 statuses verified")

if __name__ == "__main__":
    main()
