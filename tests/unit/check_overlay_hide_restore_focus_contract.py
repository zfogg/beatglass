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


def test_overlay_recovery_without_display_uses_availability_focus():
    start = QML.index("function focusOverlayNoDisplayRecovery")
    body = QML[start : QML.index("    function focusOverlayRecoveryAction", start)]
    assert "overlayDisplaySelectionRequired = true" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "focusOverlayNoDisplayRecovery()" in QML[QML.index("onAvailableChanged") : start]


def test_restored_display_without_saved_match_uses_selection_focus():
    start = QML.index("function focusOverlayDisplaySelectionRequired")
    body = QML[start : QML.index("    function focusOverlayRecoveryAction", start)]
    assert "overlayDisplaySelectionRequired = true" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "focusOverlayDisplaySelectionRequired()" in QML[QML.index("function onScreensChanged") : start]


def test_interaction_state_changes_share_stale_evidence_announcement():
    start = QML.index("function handleOverlayInteractionStateChanged")
    body = QML[start : QML.index("    function focusOverlayDisplaySelectionRequired", start)]
    assert "overlaySettingsRevision += 1" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "handleOverlayInteractionStateChanged()" in QML[QML.index("function onClickThroughChanged") : start]


def test_opacity_changes_share_stale_recovery_handler():
    start = QML.index("function handleOverlayOpacityChanged")
    body = QML[start : QML.index("    function focusOverlayDisplaySelectionRequired", start)]
    assert "overlayOpacityEditInFlight" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "handleOverlayOpacityChanged()" in QML[QML.index("function onOpacityChanged") : start]


def test_geometry_recovery_preserves_stale_state_and_focuses_next_action():
    start = QML.index("function handleOverlayGeometryRecovery")
    body = QML[start : QML.index("    function focusOverlayDisplaySelectionRequired", start)]
    assert "overlayCaptureEvidenceStale = true" in body
    assert "recoverOverlayAfterDisplayChange()" in body
    assert "retryOverlay.forceActiveFocus()" in body
    assert "handleOverlayGeometryRecovery()" in QML[QML.index("function onGeometryChanged") : start]


def test_geometry_status_announcement_preserves_stale_boundary():
    start = QML.index("function announceOverlayGeometryState")
    body = QML[start : QML.index("    function focusOverlayDisplaySelectionRequired", start)]
    assert "overlayCaptureEvidenceStale = true" in body
    assert "Live evidence remains stale until fresh capture arrives." in body
    assert "announceOverlayGeometryState()" in QML[QML.index("function onGeometryChanged") : start]


def test_geometry_sync_failure_routes_to_display_recovery_focus():
    start = QML.index("function focusOverlayGeometrySyncFailure")
    body = QML[start : QML.index("    function focusOverlayDisplaySelectionRequired", start)]
    assert "overlayDisplaySelectionRequired = true" in body
    assert "focusOverlayAvailabilityRecovery" in body
    geometry = QML[QML.index("function onGeometryChanged") : start]
    assert "focusOverlayGeometrySyncFailure()" in geometry


def test_display_loss_uses_centralized_recovery_focus():
    start = QML.index("function focusOverlayDisplayLoss")
    body = QML[start : QML.index("    function focusOverlayDisplaySelectionRequired", start)]
    assert "overlayDisplaySelectionRequired = true" in body
    assert "focusOverlayAvailabilityRecovery" in body
    screens = QML[QML.index("function onScreensChanged") : start]
    assert "focusOverlayDisplayLoss()" in screens


def test_click_through_changes_focus_the_next_interaction_action():
    start = QML.index("function handleOverlayInteractionStateChanged")
    body = QML[start : QML.index("    function handleOverlayOpacityChanged", start)]
    assert "unlockOverlay.forceActiveFocus()" in body
    assert "overlayInteractionMode.forceActiveFocus()" in body


def test_external_opacity_changes_focus_review_control():
    start = QML.index("function handleOverlayOpacityChanged")
    body = QML[start : QML.index("    function handleOverlayGeometryRecovery", start)]
    assert "overlayOpacityControl.forceActiveFocus()" in body


def test_size_change_result_preserves_persisted_state_and_focus():
    start = QML.index("function handleOverlaySizeChangeResult")
    body = QML[start : QML.index("    function handleOverlayGeometryRecovery", start)]
    assert "overlayPreferredSizeIndex = nextIndex" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "overlaySize.forceActiveFocus()" in body
    assert "handleOverlaySizeChangeResult(resized, nextIndex)" in QML[QML.index("sequence: \"Ctrl+Alt+O\"") : start]


def test_reapply_shortcut_activates_and_restores_focus():
    start = QML.index("function focusOverlayReapplyShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "navigation.currentIndex = 1" in body
    assert "reapplyOverlaySettingsAndRestore()" in body
    assert "reapplyOverlaySettings.forceActiveFocus()" in body


def test_summary_review_shortcut_announces_full_recovery_state():
    start = QML.index("function focusOverlaySummaryReviewShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "overlaySettingsSummaryReviewAction.forceActiveFocus()" in body
    assert "overlayDisplayRecoveryPending" in body
    assert "overlayCaptureEvidenceStale" in body


def test_unlock_shortcut_announces_stale_boundary():
    start = QML.index("function focusOverlayUnlockShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "unlockOverlay.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_show_shortcut_announces_stale_boundary():
    start = QML.index("function focusOverlayShowShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "showOverlay.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_hide_shortcut_uses_explicit_hidden_state_for_focus():
    start = QML.index("function focusOverlayHideShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "window.overlayHiddenByUser" in body
    assert "restoreOverlay.forceActiveFocus()" in body
    assert "hideOverlay.forceActiveFocus()" in body
    shortcut = QML[QML.index("objectName: \"overlayHideFocusShortcut\"") : start]
    assert "window.focusOverlayHideShortcut()" in shortcut
    assert "if (hidden)" not in shortcut


def test_size_shortcut_announces_stale_boundary():
    start = QML.index("function focusOverlaySizeShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "overlaySize.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_click_through_shortcut_announces_stale_boundary():
    start = QML.index("function focusOverlayClickThroughShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "clickThroughToggle.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_opacity_shortcut_announces_stale_boundary():
    start = QML.index("function focusOverlayOpacityShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "overlayOpacityControl.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_opacity_reset_result_preserves_persisted_state_and_focus():
    start = QML.index("function handleOverlayOpacityResetResult")
    body = QML[start : QML.index("    function focusOverlayDisplaySelectionRequired", start)]
    assert "overlayOpacityEditInFlight = true" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "overlayOpacityControl.forceActiveFocus()" in body


def test_click_through_toggle_result_preserves_stale_boundary():
    start = QML.index("function handleOverlayClickThroughShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "setClickThrough(requested)" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "clickThroughToggle.forceActiveFocus()" in body
