"""Contract coverage for stale evidence during display recovery."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_display_recovery_exposes_stale_evidence_accessibly() -> None:
    marker = 'objectName: "overlayDisplayStatus"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1300]
    assert "recovery pending; live evidence stale" in snippet
    assert "live overlay evidence is stale until the overlay returns" in snippet


if __name__ == "__main__":
    test_display_recovery_exposes_stale_evidence_accessibly()
    print("1 check passed")
