"""Contract coverage for grouped overlay control recovery semantics."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_overlay_controls_group_exposes_stale_recovery_boundary() -> None:
    marker = 'objectName: "overlayDisplayControls"'
    snippet = QML[QML.index(marker) : QML.index(marker) + 1000]
    assert "recovery pending; live evidence stale" in snippet
    assert "live overlay evidence is stale until a display is confirmed" in snippet


if __name__ == "__main__":
    test_overlay_controls_group_exposes_stale_recovery_boundary()
    print("1 check passed")
