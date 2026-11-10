"""Contract coverage for in-progress overlay retry stale status."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_attempt_status_exposes_stale_evidence() -> None:
    marker = 'objectName: "overlayRecoveryAttemptStatus"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 800]
    assert "live overlay evidence stale" in snippet
    assert "live overlay evidence is stale until the overlay is restored" in snippet


if __name__ == "__main__":
    test_attempt_status_exposes_stale_evidence()
    print("1 check passed")
