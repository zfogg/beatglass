"""Contract coverage for paused overlay recovery stale status."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_paused_recovery_status_exposes_stale_evidence() -> None:
    marker = 'objectName: "overlayRecoveryPausedStatus"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 800]
    assert "live overlay evidence stale" in snippet
    assert "live overlay evidence is stale until recovery succeeds" in snippet


if __name__ == "__main__":
    test_paused_recovery_status_exposes_stale_evidence()
    print("1 check passed")
