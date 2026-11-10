"""Contract coverage for historical persisted diagnostic stale boundary."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
def test_persisted_diagnostic_exposes_stale_boundary() -> None:
    marker = 'objectName: "overlayPersistedRecoveryDiagnostic"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1500]
    assert "live evidence stale until fresh capture" in snippet
    assert "Live evidence is stale until fresh capture arrives." in snippet
if __name__ == "__main__":
    test_persisted_diagnostic_exposes_stale_boundary()
    print("1 check passed")
