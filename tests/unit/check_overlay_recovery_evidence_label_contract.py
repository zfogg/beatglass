"""Contract coverage for the shared overlay recovery evidence label."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def test_shared_recovery_evidence_label_tracks_stale_state() -> None:
    assert 'readonly property string overlayRecoveryEvidenceLabel' in QML
    assert 'overlayCaptureEvidenceStale ? "Live evidence stale until fresh capture"' in QML


if __name__ == "__main__":
    test_shared_recovery_evidence_label_tracks_stale_state()
    print("1 check passed")
