"""Contract checks for deterministic overlay hide/restore focus recovery."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
STATES = (ROOT / "design" / "v1" / "STATES.md").read_text(encoding="utf-8")


def test_hidden_state_helper_activates_page_and_announces_evidence_boundary():
    start = QML.index("function focusOverlayHiddenState")
    body = QML[start : QML.index("    }", start) + 5]
    assert "navigation.currentIndex = 1" in body
    assert "window.requestActivate()" in body
    assert "Live evidence remains stale" in body
    assert "Retained settings remain available" in body


def test_hide_workflow_focuses_restore_or_retry_deterministically():
    start = QML.index("function hideOverlayAndRestoreControls")
    body = QML[start : QML.index("    function resetOverlayRecoveryAttempts", start)]
    assert "window.focusOverlayHiddenState(message, hidden ? restoreOverlay : hideOverlay)" in body
    assert "overlayDisplayRecoveryPending = true" in body


def test_restore_failures_share_an_activated_stale_focus_path():
    start = QML.index("function focusOverlayRestoreFailure")
    body = QML[start : QML.index("    Connections", start)]
    assert "navigation.currentIndex = 1" in body
    assert "Live evidence is stale until a fresh capture arrives." in body
    restore = QML[QML.index("function showOverlayAndHideControls") : QML.index("    function restoreOverlayVisualStateFromBridge")]
    assert restore.count("window.focusOverlayRestoreFailure(") == 3


def test_design_state_documents_hide_restore_focus_contract():
    assert "Overlay hide/restore focus workflow" in STATES


def test_unlock_workflow_centralizes_success_and_failure_focus():
    start = QML.index("function focusOverlayUnlockState")
    body = QML[start : QML.index("    Connections", start)]
    assert "navigation.currentIndex = 1" in body
    assert "Interaction state is current." in body
    unlock = QML[QML.index("function unlockOverlayAndRestoreFocus") : QML.index("    function reapplyOverlaySettingsAndRestore")]
    assert unlock.count("window.focusOverlayUnlockState(") == 2
    assert "setClickThrough(false)" in unlock


def test_persisted_settings_failures_share_stale_recovery_focus():
    start = QML.index("function focusOverlaySettingsRecoveryFailure")
    body = QML[start : QML.index("    Connections", start)]
    assert "overlayCaptureEvidenceStale = true" in body
    settings = QML[QML.index("function reapplyOverlaySettingsAndRestore") : QML.index("    function recoverOverlayAfterDisplayChange")]
    assert settings.count("window.focusOverlaySettingsRecoveryFailure(") == 3


def test_display_selection_persistence_failures_share_recovery_focus():
    selection = QML[QML.index("function persistOverlayDisplaySelection") : QML.index("    function forgetUnavailableOverlayDisplay")]
    assert selection.count("window.focusOverlayDisplayRecoveryFailure(") == 4
    assert "overlayPreferredScreenIndex = -1" in selection


def test_forgetting_display_uses_shared_recovery_focus():
    start = QML.index("function forgetUnavailableOverlayDisplay")
    body = QML[start : QML.index("    function persistOverlayDisplaySelection", start)]
    assert "window.focusOverlayDisplayRecoveryFailure(" in body
    assert "overlayPreferredDisplayName = \"\"" in body


def test_recovery_pause_and_reset_share_focus_transition():
    start = QML.index("function focusOverlayRecoveryPauseState")
    body = QML[start : QML.index("    function quitApplication", start)]
    assert "Live evidence is current." in body
    assert body.count("window.focusOverlayRecoveryPauseState(") == 2


def test_bridge_settings_failure_uses_availability_focus_transition():
    start = QML.index("function focusOverlayBridgeSettingsFailure")
    body = QML[start : QML.index("    Connections", start)]
    assert "overlayDisplayRecoveryPending = true" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "focusOverlayBridgeSettingsFailure(" in QML[QML.index("const synced = window.syncOverlaySettingsFromBridge()") :]


def test_startup_attach_and_sync_failures_share_fallback_state():
    start = QML.index("function focusOverlayAttachFailure")
    body = QML[start : QML.index("    function focusOverlayRecoveryAction", start)]
    assert "overlayDisplaySelectionRequired = true" in body
    completed = QML[QML.index("Component.onCompleted") : QML.index("    function focusOverlayAttachFailure")]
    assert completed.count("window.focusOverlayAttachFailure(") == 2


def test_capture_startup_failure_marks_stale_pending_recovery():
    start = QML.index("function focusOverlayCaptureStartupFailure")
    body = QML[start : QML.index("    function focusOverlayRecoveryAction", start)]
    assert "overlayCaptureEvidenceStale = true" in body
    assert "overlayDisplayRecoveryPending = true" in body
    assert "focusOverlayCaptureStartupFailure()" in QML[QML.index("Component.onCompleted") : start]


def test_capture_monitor_status_changes_share_one_transition_handler():
    start = QML.index("function handleCaptureMonitorStatusChanged")
    body = QML[start : QML.index("    Connections", start)]
    assert "transitionCaptureEvidence(true" in body
    assert "transitionCaptureEvidence(false" in body
    connections = QML[QML.index("target: window.captureMonitorBridge") : start]
    assert "window.handleCaptureMonitorStatusChanged()" in connections


def test_endpoint_changes_share_overlay_stale_recovery_handler():
    start = QML.index("function handleOverlayEndpointChanged")
    body = QML[start : QML.index("    Connections", start)]
    assert "overlayCaptureEvidenceStale = true" in body
    assert "overlayDisplayRecoveryPending = true" in body
    assert "endpointSelector.forceActiveFocus()" in body
    connections = QML[QML.index("target: window.endpointSelectionBridge") : start]
    assert "window.handleOverlayEndpointChanged()" in connections
