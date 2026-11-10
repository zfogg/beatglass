"""Contract coverage for stopping overlay recovery."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_stop_recovery_exposes_stale_evidence_boundary() -> None:
    marker = 'objectName: "stopOverlayRecovery"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 700]
    assert "live overlay evidence remains stale until recovery succeeds" in snippet


if __name__ == "__main__":
    test_stop_recovery_exposes_stale_evidence_boundary()
    print("1 check passed")
