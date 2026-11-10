"""Contract coverage for the combined-output track metadata boundary."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_overlay_exposes_track_metadata_unavailable_state() -> None:
    marker = 'objectName: "overlayTrackMetadataUnavailable"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 900]
    assert 'text: "Track info unavailable"' in snippet
    assert "combined system output does not identify individual tracks or decks" in snippet
    assert "separately supported source" in snippet


if __name__ == "__main__":
    test_overlay_exposes_track_metadata_unavailable_state()
    print("1 check passed")
