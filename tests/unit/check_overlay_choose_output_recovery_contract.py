"""Contract coverage for choosing an output during overlay capture recovery."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_choose_output_action_routes_focus_to_endpoint_selector() -> None:
    marker = 'objectName: "chooseOverlayOutput"'
    snippet = QML[QML.index(marker) - 100 : QML.index(marker) + 2200]
    assert 'text: "Choose output"' in snippet
    assert "current estimates remain unavailable until fresh output is accepted" in snippet
    assert "endpointSelector.forceActiveFocus()" in snippet


if __name__ == "__main__":
    test_choose_output_action_routes_focus_to_endpoint_selector()
    print("1 check passed")
