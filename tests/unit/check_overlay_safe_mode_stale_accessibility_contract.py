"""Contract coverage for truthful safe-mode overlay status."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_safe_mode_status_explains_stale_evidence_and_recovery() -> None:
    assert 'objectName: "overlaySafeModeStatus"' in QML
    assert "monitoring continues without the native overlay" in QML
    assert "live overlay evidence is stale until the overlay is restored" in QML
    assert 'objectName: "clearOverlaySafeMode"' in QML


if __name__ == "__main__":
    test_safe_mode_status_explains_stale_evidence_and_recovery()
    print("1 check passed")
