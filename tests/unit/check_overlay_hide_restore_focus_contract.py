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


def test_retry_shortcut_keeps_fallback_actionable():
    start = QML.index("function handleOverlayRetryShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "showOverlayAndHideControls()" in body
    assert "retryOverlay.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_display_focus_shortcut_announces_stale_boundary():
    start = QML.index("function focusOverlayDisplayShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "overlayDisplay.forceActiveFocus()" in body
    assert "overlayDisplaySelectionRequired" in body
    assert "overlayCaptureEvidenceStale" in body


def test_retry_focus_shortcut_announces_stale_boundary():
    start = QML.index("function focusOverlayRetryShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "retryOverlay.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_resume_shortcut_activates_recovery_page():
    start = QML.index("function handleOverlayResumeShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "navigation.currentIndex = 1" in body
    assert "resumeOverlayRecovery.clicked()" in body
    assert "Live evidence remains stale" in body


def test_safe_mode_clear_shortcut_focuses_retry():
    start = QML.index("function handleOverlayClearSafeModeShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "resetOverlayRecoveryAttempts()" in body
    assert "retryOverlay.forceActiveFocus()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_stop_shortcut_pauses_and_focuses_resume():
    start = QML.index("function handleOverlayStopShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "stopOverlayRecoveryAttempts()" in body
    assert "resumeOverlayRecovery.forceActiveFocus()" in body
    assert "Live evidence remains stale" in body


def test_paused_page_shortcut_preserves_stale_boundary():
    start = QML.index("function handleOverlayPausedPageShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "focusOverlayRecoveryAction" in body
    assert "Live evidence remains stale" in body


def test_paused_focus_shortcut_preserves_stale_boundary():
    start = QML.index("function handleOverlayPausedFocusShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "focusOverlayRecoveryAction" in body
    assert "Live evidence remains stale" in body


def test_recovery_action_focus_has_one_canonical_definition():
    assert QML.count("function focusOverlayRecoveryAction(message)") == 1
    start = QML.index("function focusOverlayRecoveryAction(message)")
    body = QML[start : QML.index("    function restoreOverlayDisplaySelectionFromBridge", start)]
    assert "overlayRecoveryEvidenceLabel" in body
    assert "resumeOverlayRecovery.forceActiveFocus()" in body


def test_safe_mode_clear_button_and_shortcut_share_action():
    start = QML.index("function handleOverlayClearSafeModeAction")
    body = QML[start : QML.index("    function handleOverlayStopShortcut", start)]
    assert "resetOverlayRecoveryAttempts()" in body
    assert "retryOverlay.forceActiveFocus()" in body
    assert "handleOverlayClearSafeModeAction()" in QML[QML.index("function handleOverlayClearSafeModeShortcut") : start]
    assert "onClicked: window.handleOverlayClearSafeModeAction()" in QML


def test_stop_recovery_button_and_shortcut_share_action():
    start = QML.index("function handleOverlayStopRecoveryAction")
    body = QML[start : QML.index("    function handleOverlayStopShortcut", start)]
    assert "stopOverlayRecoveryAttempts()" in body
    assert "resumeOverlayRecovery.forceActiveFocus()" in body
    assert "handleOverlayStopRecoveryAction()" in QML[QML.index("function handleOverlayStopShortcut") : start]
    assert "onClicked: window.handleOverlayStopRecoveryAction()" in QML


def test_retry_action_keeps_fallback_focusable_on_failure():
    start = QML.index("function handleOverlayRetryAction")
    body = QML[start : QML.index("    function handleOverlayClearSafeModeShortcut", start)]
    assert "showOverlayAndHideControls()" in body
    assert "retryOverlay.forceActiveFocus()" in body


def test_hide_toggle_uses_explicit_visibility_state():
    start = QML.index("function handleOverlayHideToggleShortcut")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "overlayHiddenByUser" in body
    assert "showOverlayAndHideControls()" in body
    assert "hideOverlayAndRestoreControls()" in body


def test_show_action_keeps_retry_focusable_on_failure():
    start = QML.index("function handleOverlayShowAction")
    body = QML[start : QML.index("    function handleOverlayHideToggleShortcut", start)]
    assert "showOverlayAndHideControls()" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "retryOverlay.forceActiveFocus()" in body


def test_hide_toggle_routes_restore_and_hide_through_action_helpers():
    start = QML.index("function handleOverlayHideToggleShortcut")
    body = QML[start : QML.index("    function handleOverlayRestoreAction", start)]
    assert "handleOverlayRestoreAction()" in body
    assert "handleOverlayHideAction()" in body


def test_reapply_button_and_shortcut_share_action():
    start = QML.index("function handleOverlayReapplyAction")
    body = QML[start : QML.index("    function focusOverlaySummaryReviewShortcut", start)]
    assert "reapplyOverlaySettingsAndRestore()" in body
    assert "reapplyOverlaySettings.forceActiveFocus()" in body
    assert "onClicked: window.handleOverlayReapplyAction()" in QML


def test_unlock_button_shares_unlock_focus_action():
    start = QML.index("function handleOverlayUnlockAction")
    body = QML[start : QML.index("    function focusOverlaySummaryReviewShortcut", start)]
    assert "unlockOverlayAndRestoreFocus()" in body
    assert "overlayInteractionMode.forceActiveFocus()" in body
    assert "onClicked: window.handleOverlayUnlockAction()" in QML


def test_hide_button_shares_hide_action():
    start = QML.index("function handleOverlayHideAction")
    body = QML[start : QML.index("    function handleOverlayRestoreAction", start)]
    assert "hideOverlayAndRestoreControls()" in body
    assert "onClicked: window.handleOverlayHideAction()" in QML


def test_show_button_action_announces_evidence_boundary():
    start = QML.index("function handleOverlayShowButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowAction", start)]
    assert "handleOverlayShowAction()" in body
    assert "overlayCaptureEvidenceStale" in body


def test_restore_action_has_one_shared_restore_transition():
    start = QML.index("function handleOverlayRestoreButtonAction")
    body = QML[start : QML.index("    function handleOverlayHideAction", start)]
    assert "handleOverlayShowAction()" in body
    assert "overlayCaptureEvidenceStale" in body
    assert "handleOverlayRestoreButtonAction()" in QML[QML.index("function handleOverlayRestoreAction") : start]


def test_restore_button_routes_to_shared_action():
    assert "onClicked: window.handleOverlayRestoreButtonAction()" in QML


def test_show_button_routes_to_shared_action():
    assert "onClicked: window.handleOverlayShowButtonAction()" in QML


def test_reapply_button_routes_to_named_action():
    start = QML.index("function handleOverlayReapplyButtonAction")
    body = QML[start : QML.index("    function focusOverlaySummaryReviewShortcut", start)]
    assert "reapplyOverlaySettingsAndRestore()" in body
    assert "reapplyOverlaySettings.forceActiveFocus()" in body
    assert "onClicked: window.handleOverlayReapplyButtonAction()" in QML


def test_retry_button_named_attempt_action():
    start = QML.index("function handleOverlayRetryButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowAction", start)]
    assert "overlayRecoveryAttemptCount += 1" in body
    assert "overlayDisplayStatus.forceActiveFocus()" in body


def test_retry_button_routes_to_named_action():
    assert "onClicked: window.handleOverlayRetryButtonAction()" in QML


def test_opacity_reset_uses_named_action():
    start = QML.index("function handleOverlayOpacityResetAction")
    body = QML[start : QML.index("    function handleOverlayClickThroughShortcut", start)]
    assert "setOpacity(resetOpacity)" in body
    assert "handleOverlayOpacityResetResult" in body


def test_click_through_button_routes_to_named_action():
    start = QML.index("function handleOverlayClickThroughButtonAction")
    body = QML[start : QML.index("    function handleOverlayClickThroughShortcut", start)]
    assert "setClickThrough(requestedClickThrough)" in body
    assert "overlayInteractionMode.forceActiveFocus()" in body
    assert "onClicked: window.handleOverlayClickThroughButtonAction(checked)" in QML


def test_opacity_edit_recovery_keeps_slider_focused():
    start = QML.index("function focusOverlayOpacityEditRecovery")
    body = QML[start : QML.index("    function handleOverlayClickThroughButtonAction", start)]
    assert "overlayOpacityControl.forceActiveFocus()" in body
    assert "announceCaptureRecoveryState(message" in body
    assert 'window.focusOverlayOpacityEditRecovery("DJ overlay opacity could not be changed' in QML
    assert 'window.focusOverlayOpacityEditRecovery("DJ overlay opacity edit complete' in QML

def test_display_selection_recovery_keeps_selector_focused():
    start = QML.index("function focusOverlayDisplaySelectionRecovery")
    body = QML[start : QML.index("    function handleOverlayClickThroughButtonAction", start)]
    assert "overlayDisplay.forceActiveFocus()" in body
    assert "announceCaptureRecoveryState(message" in body
    assert 'window.focusOverlayDisplaySelectionRecovery("Overlay display selected: "' in QML
    assert 'window.focusOverlayDisplaySelectionRecovery("Overlay display selection could not be applied' in QML


def test_resume_recovery_routes_button_and_shortcut_to_shared_action():
    start = QML.index("function handleOverlayResumeRecoveryAction")
    body = QML[start : QML.index("    function handleOverlayClearSafeModeShortcut", start)]
    assert "overlayRecoveryPausedByUser = false" in body
    assert "overlayRecoveryAttemptCount = 1" in body
    assert "showOverlayAndHideControls()" in body
    assert "overlayDisplayStatus.forceActiveFocus()" in body
    assert "window.handleOverlayResumeRecoveryAction()" in QML
    assert "onClicked: window.handleOverlayResumeRecoveryAction()" in QML


def test_stop_recovery_is_idempotent_when_already_paused():
    start = QML.index("function handleOverlayStopRecoveryAction")
    body = QML[start : QML.index("    function handleOverlayPausedPageShortcut", start)]
    assert "overlayRecoveryPausedByUser" in body
    assert "focusOverlayRecoveryPauseState" in body
    assert "return" in body
    assert "stopOverlayRecoveryAttempts()" in body


def test_size_recovery_keeps_selector_focused():
    start = QML.index("function focusOverlaySizeRecovery")
    body = QML[start : QML.index("    function handleOverlayClickThroughButtonAction", start)]
    assert "overlaySize.forceActiveFocus()" in body
    assert "announceCaptureRecoveryState(message" in body
    assert 'window.focusOverlaySizeRecovery("DJ overlay size set to "' in QML
    assert 'window.focusOverlaySizeRecovery("DJ overlay size could not be changed' in QML


def test_retry_button_respects_paused_recovery_boundary():
    start = QML.index("function handleOverlayRetryButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowButtonAction", start)]
    assert "overlayRecoveryPausedByUser" in body
    assert "focusOverlayRecoveryPauseState" in body
    assert "Resume recovery before retrying" in body
    assert "return" in body
    assert "overlayRecoveryAttemptCount += 1" in body

def test_clear_safe_mode_is_idempotent_before_limit():
    start = QML.index("function handleOverlayClearSafeModeAction")
    body = QML[start : QML.index("    function handleOverlayStopShortcut", start)]
    assert "overlayRecoveryAttemptCount < window.overlayRecoveryAttemptLimit" in body
    assert "focusOverlayRecoveryPauseState" in body
    assert "safe mode is not active" in body
    assert "return" in body
    assert "resetOverlayRecoveryAttempts()" in body


def test_show_button_requires_available_display():
    start = QML.index("function handleOverlayShowButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowAction", start)]
    assert "overlayDisplaySelectionRequired" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return" in body


def test_restore_button_requires_available_display():
    start = QML.index("function handleOverlayRestoreButtonAction")
    body = QML[start : QML.index("    function handleOverlayHideAction", start)]
    assert "overlayDisplaySelectionRequired" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return" in body


def test_show_button_requires_available_display():
    start = QML.index("function handleOverlayShowButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowAction", start)]
    assert "overlayDisplaySelectionRequired" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return" in body


def test_restore_button_requires_available_display():
    start = QML.index("function handleOverlayRestoreButtonAction")
    body = QML[start : QML.index("    function handleOverlayHideAction", start)]

def test_reapply_button_requires_available_display():
    start = QML.index("function handleOverlayReapplyButtonAction")
    body = QML[start : QML.index("    function handleOverlayUnlockAction", start)]
    assert "overlayDisplaySelectionRequired" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return" in body


def test_unlock_action_requires_available_click_through_overlay():
    start = QML.index("function handleOverlayUnlockAction")
    body = QML[start : QML.index("    function focusOverlaySummaryReviewShortcut", start)]
    assert "overlayBridge.available" in body
    assert "clickThrough" in body
    assert "focusOverlayAvailabilityRecovery" in body

def test_hide_action_requires_available_overlay():
    start = QML.index("function handleOverlayHideAction")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "overlayBridge.available" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "showOverlay" in body
    assert "return" in body


def test_show_action_requires_available_display():
    start = QML.index("function handleOverlayShowAction")
    body = QML[start : QML.index("    function focusOverlayDisplayShortcut", start)]
    assert "overlayDisplaySelectionRequired" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body

def test_retry_action_respects_paused_recovery_boundary():
    start = QML.index("function handleOverlayRetryAction")
    body = QML[start : QML.index("    function handleOverlayRetryButtonAction", start)]
    assert "overlayRecoveryPausedByUser" in body
    assert "focusOverlayRecoveryPauseState" in body
    assert "Resume recovery before retrying" in body
    assert "return false" in body


def test_reapply_button_respects_native_result_focus():
    start = QML.index("function handleOverlayReapplyButtonAction")
    body = QML[start : QML.index("    function handleOverlayUnlockAction", start)]
    assert "const reapplied" in body
    assert "if (reapplied)" in body
    assert "reapplyOverlaySettings.forceActiveFocus()" in body

def test_unlock_action_returns_native_restoration_result():
    start = QML.index("function handleOverlayUnlockAction")
    body = QML[start : QML.index("    function focusOverlaySummaryReviewShortcut", start)]
    assert "const restored" in body
    assert "if (restored)" in body
    assert "return restored" in body
    assert "return false" in body
def test_click_through_action_returns_application_result():
    start = QML.index("function handleOverlayClickThroughButtonAction")
    body = QML[start : QML.index("    function handleOverlayClickThroughShortcut", start)]
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body
    assert "return applied && clickThroughToggle.checked === requestedClickThrough" in body
    assert "overlayInteractionMode.forceActiveFocus()" in body


def test_click_through_shortcut_handles_missing_bridge():
    start = QML.index("function handleOverlayClickThroughShortcut")
    body = QML[start : QML.index("    function handleOverlayRetryShortcut", start)]
    assert "if (!window.overlayBridge)" in body

def test_unlock_focus_shortcut_requires_available_click_through_overlay():
    start = QML.index("function focusOverlayUnlockShortcut")
    body = QML[start : QML.index("    function focusOverlayShowShortcut", start)]
    assert "overlayBridge.available" in body
    assert "clickThrough" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body
    assert "return true" in body


def test_show_focus_shortcut_requires_available_display():
    start = QML.index("function focusOverlayShowShortcut")
    body = QML[start : QML.index("    function focusOverlayHideShortcut", start)]
    assert "overlayDisplaySelectionRequired" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return false" in body
    assert "return true" in body


def test_hide_focus_shortcut_requires_available_overlay():
    start = QML.index("function focusOverlayHideShortcut")
    body = QML[start : QML.index("    function handleOverlayHideToggleShortcut", start)]
    assert "overlayBridge.available" in body

def test_size_focus_shortcut_requires_available_overlay():
    start = QML.index("function focusOverlaySizeShortcut")
    body = QML[start : QML.index("    function focusOverlayClickThroughShortcut", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body
    assert "return true" in body


def test_click_through_focus_shortcut_requires_available_overlay():
    start = QML.index("function focusOverlayClickThroughShortcut")
    body = QML[start : QML.index("    function focusOverlayOpacityShortcut", start)]

def test_opacity_focus_shortcut_requires_available_overlay():
    start = QML.index("function focusOverlayOpacityShortcut")
    body = QML[start : QML.index("    function handleOverlayOpacityResetResult", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body
    assert "return true" in body


def test_opacity_reset_requires_available_overlay():
    start = QML.index("function handleOverlayOpacityResetAction")
    body = QML[start : QML.index("    function focusOverlayOpacityEditRecovery", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body
    assert "return applied" in body

def test_retry_focus_shortcut_handles_missing_bridge():
    start = QML.index("function focusOverlayRetryShortcut")
    body = QML[start : QML.index("    function handleOverlayResumeShortcut", start)]
    assert "if (!window.overlayBridge)" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body
    assert "return true" in body
def test_reapply_focus_shortcut_propagates_result():
    start = QML.index("function focusOverlayReapplyShortcut")
    body = QML[start : QML.index("    function handleOverlayReapplyButtonAction", start)]
    assert "return window.handleOverlayReapplyAction()" in body
    action_start = QML.index("function handleOverlayReapplyAction")
    action_body = QML[action_start : QML.index("    function handleOverlayReapplyButtonAction", action_start)]
    assert "return window.handleOverlayReapplyButtonAction()" in action_body
    button_start = QML.index("function handleOverlayReapplyButtonAction")
    button_body = QML[button_start : QML.index("    function handleOverlayUnlockAction", button_start)]
    assert "return false" in button_body
    assert "return reapplied" in button_body
def test_restore_action_propagates_result():
    start = QML.index("function handleOverlayRestoreAction")
    body = QML[start : QML.index("    function handleOverlayRestoreButtonAction", start)]
    assert "return window.handleOverlayRestoreButtonAction()" in body
    button_start = QML.index("function handleOverlayRestoreButtonAction")
    button_body = QML[button_start : QML.index("    function handleOverlayHideAction", button_start)]
    assert "return false" in button_body
    assert "return window.handleOverlayShowAction()" in button_body
def test_hide_action_propagates_result():
    start = QML.index("function handleOverlayHideAction")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "return false" in body
    assert "return window.hideOverlayAndRestoreControls()" in body


def test_resume_action_propagates_native_result():
    start = QML.index("function handleOverlayResumeShortcut")
    body = QML[start : QML.index("    function handleOverlayResumeRecoveryAction", start)]
    assert "return window.handleOverlayResumeRecoveryAction()" in body
    action_start = QML.index("function handleOverlayResumeRecoveryAction")
    action_body = QML[action_start : QML.index("    function handleOverlayClearSafeModeShortcut", action_start)]
    assert "const resumed" in action_body
    assert "overlayDisplayStatus.forceActiveFocus()" in action_body
    assert "return resumed" in action_body

def test_clear_safe_mode_action_returns_result():
    start = QML.index("function handleOverlayClearSafeModeAction")
    body = QML[start : QML.index("    function handleOverlayStopShortcut", start)]
    assert "return false" in body
    assert "return true" in body
    assert "resetOverlayRecoveryAttempts()" in body
    assert "retryOverlay.forceActiveFocus()" in body
def test_stop_recovery_action_returns_result():
    start = QML.index("function handleOverlayStopRecoveryAction")
    body = QML[start : QML.index("    function handleOverlayPausedPageShortcut", start)]
    assert "return false" in body
    assert "stopOverlayRecoveryAttempts()" in body
    assert "resumeOverlayRecovery.forceActiveFocus()" in body
    assert "return true" in body
def test_hide_restore_toggle_shortcut_propagates_selected_action_result():
    start = QML.index("function handleOverlayHideToggleShortcut")
    body = QML[start : QML.index("    function handleOverlayRestoreAction", start)]
    assert "return window.handleOverlayRestoreAction()" in body
    assert "return window.handleOverlayHideAction()" in body
def test_display_focus_shortcut_returns_bridge_availability():
    start = QML.index("function focusOverlayDisplayShortcut")
    body = QML[start : QML.index("    function focusOverlayRetryShortcut", start)]
    assert "overlayDisplay.forceActiveFocus()" in body
    assert "announceCaptureRecoveryState" in body
    assert "return window.overlayBridge !== null" in body


 
def test_retry_button_returns_native_result():
    start = QML.index("function handleOverlayRetryButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowButtonAction", start)]
    assert "return false" in body
    assert "const retried" in body
    assert "overlayDisplayStatus.forceActiveFocus()" in body
    assert "return retried" in body
def test_show_button_propagates_native_result():
    start = QML.index("function handleOverlayShowButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowAction", start)]
    assert "return false" in body
    assert "return window.handleOverlayShowAction()" in body
    action_start = QML.index("function handleOverlayShowAction")
    action_body = QML[action_start : QML.index("    function focusOverlayDisplayShortcut", action_start)]
    assert "const restored" in action_body
    assert "retryOverlay.forceActiveFocus()" in action_body
    assert "return restored" in action_body


def test_stop_recovery_shortcut_propagates_result():
    start = QML.index("function handleOverlayStopShortcut")
    body = QML[start : QML.index("    function handleOverlayStopRecoveryAction", start)]
    assert "return window.handleOverlayStopRecoveryAction()" in body
    action_start = QML.index("function handleOverlayStopRecoveryAction")
    action_body = QML[action_start : QML.index("    function handleOverlayPausedPageShortcut", action_start)]

def test_clear_safe_mode_shortcut_propagates_result():
    start = QML.index("function handleOverlayClearSafeModeShortcut")
    body = QML[start : QML.index("    function handleOverlayClearSafeModeAction", start)]
    assert "return window.handleOverlayClearSafeModeAction()" in body
    action_start = QML.index("function handleOverlayClearSafeModeAction")
    action_body = QML[action_start : QML.index("    function handleOverlayStopShortcut", action_start)]
    assert "return false" in action_body
    assert "return true" in action_body


def test_size_shortcut_routes_named_result_helper():
    start = QML.index("function handleOverlaySizeShortcut")
    body = QML[start : QML.index("    function handleOverlayOpacityResetAction", start)]
    assert "resizeOverlay" in body
    assert "handleOverlaySizeChangeResult" in body
    assert "return resized" in body
    assert "window.handleOverlaySizeShortcut()" in QML


def test_click_through_shortcut_routes_named_action_result():
    start = QML.index("function handleOverlayClickThroughShortcutAction")
    body = QML[start : QML.index("    function handleOverlayClickThroughShortcut()", start)]
    assert "unlockOverlayAndRestoreFocus()" in body
    assert "handleOverlayClickThroughButtonAction(true)" in body
    shortcut = QML[QML.index("function handleOverlayClickThroughShortcut()") : QML.index("    function handleOverlayRetryShortcut", QML.index("function handleOverlayClickThroughShortcut()"))]
    assert "return window.handleOverlayClickThroughShortcutAction()" in shortcut


def test_retry_shortcut_routes_named_action_result():
    start = QML.index("function handleOverlayRetryShortcutAction")
    body = QML[start : QML.index("    function handleOverlayRetryShortcut()", start)]
    assert "window.show()" in body
    assert "handleOverlayRetryButtonAction()" in body
    shortcut_start = QML.index("function handleOverlayRetryShortcut()")
    shortcut = QML[shortcut_start : QML.index("    function handleOverlayRetryAction", shortcut_start)]
    assert "return window.handleOverlayRetryShortcutAction()" in shortcut


def test_retry_action_returns_restore_result():
    start = QML.index("function handleOverlayRetryAction")
    body = QML[start : QML.index("    function handleOverlayRetryButtonAction", start)]
    assert "const restored = window.showOverlayAndHideControls()" in body
    assert "return restored" in body
    assert "retryOverlay.forceActiveFocus()" in body


def test_show_button_returns_failure_when_display_unavailable():
    start = QML.index("function handleOverlayShowButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowAction", start)]
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return false" in body
    assert "return window.handleOverlayShowAction()" in body


def test_hide_focus_shortcut_announces_before_success_return():
    start = QML.index("function focusOverlayHideShortcut")
    body = QML[start : QML.index("    function focusOverlaySizeShortcut", start)]
    assert body.index("announceCaptureRecoveryState") < body.index("return true")
    assert "Restore is ready." in body


def test_stop_recovery_action_has_single_success_return():
    start = QML.index("function handleOverlayStopRecoveryAction")
    body = QML[start : QML.index("    function handleOverlayPausedPageShortcut", start)]
    assert body.count("return true") == 1
    assert "resumeOverlayRecovery.forceActiveFocus()" in body


def test_summary_review_focus_shortcut_reports_success():
    start = QML.index("function focusOverlaySummaryReviewShortcut")
    body = QML[start : QML.index("    function focusOverlayUnlockShortcut", start)]
    assert "overlaySettingsSummaryReviewAction.forceActiveFocus()" in body
    assert "announceCaptureRecoveryState" in body
    assert "return true" in body


def test_paused_recovery_shortcuts_propagate_focus_result():
    page_start = QML.index("function handleOverlayPausedPageShortcut")
    page = QML[page_start : QML.index("    function handleOverlayPausedFocusShortcut", page_start)]
    focus_start = QML.index("function handleOverlayPausedFocusShortcut")
    focus = QML[focus_start : QML.index("    function handleOverlayHideToggleShortcut", focus_start)]
    assert "return window.focusOverlayRecoveryAction" in page
    assert "return window.focusOverlayRecoveryAction" in focus


def test_size_shortcut_handles_unavailable_overlay():
    start = QML.index("function handleOverlaySizeShortcut")
    body = QML[start : QML.index("    function focusOverlaySizeRecovery", start)]
    assert "overlayBridge.available" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body
    assert "resizeOverlay" in body


def test_overlay_pulse_is_theme_aware_and_invalidates_stale_tracking():
    start = QML.index("property real pulseOpacity")
    body = QML[start : QML.index("            MouseArea", start)]
    assert "pulseColor" in body
    assert "Qt.darker" in body
    assert "Qt.lighter" in body
    assert 'status === "Tracking"' in body
    assert "isFinite(bpm)" in body
    assert "overlaySurface.pulseOpacity = 0.0" in body
    assert "reduceMotion" in body


def test_theme_preview_selection_uses_shared_accessible_helper():
    start = QML.index("function selectThemeIndex")
    helper = QML[start : QML.index("    // Keep the visual selection synchronized", start)]
    assert "themePreferenceAnnouncement" in helper
    assert "themePreferencesBridge.themeIndex" in helper
    assert "requestPaint" in helper
    previews = QML[QML.index("id: themePreviews") : QML.index("id: themeSelector", QML.index("id: themePreviews"))]
    assert previews.count("window.selectThemeIndex") >= 5


def test_theme_selector_and_reset_use_shared_selection_helper():
    selector_start = QML.index('id: themeSelector')
    selector = QML[selector_start : QML.index('id: reduceMotion', selector_start)]
    assert "window.selectThemeIndex(currentIndex)" in selector
    reset_start = QML.index('objectName: "resetAppearance"')
    reset = QML[reset_start : QML.index('objectName: "', reset_start + 20)]
    assert "window.selectThemeIndex(0)" in reset


def test_reduce_motion_switch_uses_named_preference_action():
    start = QML.index("function setReduceMotionPreference")
    helper = QML[start : QML.index("    // Keep the visual selection synchronized", start)]
    assert "themePreferencesBridge.reduceMotion = enabled" in helper
    assert "themePreferenceAnnouncement" in helper
    assert "return false" in helper
    switch_start = QML.index('objectName: "reduceMotion"')
    switch = QML[switch_start : QML.index('objectName: "themePreferenceAnnouncement"', switch_start)]
    assert "window.setReduceMotionPreference(checked)" in switch


def test_reset_appearance_uses_shared_theme_and_motion_actions():
    start = QML.index("function resetAppearancePreferences")
    helper = QML[start : QML.index("    // Keep the visual selection synchronized", start)]
    assert "themePreferencesBridge.reset()" in helper
    assert "window.selectThemeIndex(0)" in helper
    assert "window.setReduceMotionPreference(false)" in helper
    button_start = QML.index('objectName: "resetAppearance"')
    button = QML[button_start : QML.index('objectName: "themePreviews"', button_start)]
    assert "window.resetAppearancePreferences()" in button


def test_click_through_button_requires_available_overlay():
    start = QML.index("function handleOverlayClickThroughButtonAction")
    body = QML[start : QML.index("    function handleOverlayClickThroughShortcutAction", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body


def test_hide_action_requires_destination_display():
    start = QML.index("function handleOverlayHideAction")
    body = QML[start : QML.index("    function recoverOverlayAfterDisplayChange", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body


def test_restore_button_requires_available_overlay():
    start = QML.index("function handleOverlayRestoreButtonAction")
    body = QML[start : QML.index("    function handleOverlayHideAction", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return false" in body


def test_resume_recovery_requires_available_overlay():
    start = QML.index("function handleOverlayResumeRecoveryAction")
    body = QML[start : QML.index("    function handleOverlayClearSafeModeShortcut", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return false" in body


def test_retry_button_requires_available_overlay():
    start = QML.index("function handleOverlayRetryButtonAction")
    body = QML[start : QML.index("    function handleOverlayShowButtonAction", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return false" in body


def test_show_paths_require_available_overlay():
    button_start = QML.index("function handleOverlayShowButtonAction")
    button = QML[button_start : QML.index("    function handleOverlayShowAction", button_start)]
    action_start = QML.index("function handleOverlayShowAction")
    action = QML[action_start : QML.index("    function focusOverlayDisplayShortcut", action_start)]
    assert "overlayBridge.available" in button
    assert "overlayBridge.available" in action
    assert "screenNames.length === 0" in button
    assert "screenNames.length === 0" in action


def test_click_through_shortcut_requires_available_overlay():
    start = QML.index("function handleOverlayClickThroughShortcutAction")
    body = QML[start : QML.index("    function handleOverlayClickThroughShortcut()", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body


def test_display_focus_shortcut_requires_available_overlay():
    start = QML.index("function focusOverlayDisplayShortcut")
    body = QML[start : QML.index("    function focusOverlayRetryShortcut", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return true" in body


def test_retry_focus_shortcut_requires_available_overlay():
    start = QML.index("function focusOverlayRetryShortcut")
    body = QML[start : QML.index("    function handleOverlayResumeShortcut", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body


def test_reapply_focus_shortcut_requires_available_overlay():
    start = QML.index("function focusOverlayReapplyShortcut")
    body = QML[start : QML.index("    function handleOverlayReapplyAction", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayDisplayRecoveryFailure" in body
    assert "return window.handleOverlayReapplyAction()" in body


def test_hide_focus_shortcut_requires_destination_display():
    start = QML.index("function focusOverlayHideShortcut")
    body = QML[start : QML.index("    function focusOverlaySizeShortcut", start)]
    assert "overlayBridge.available" in body
    assert "screenNames.length === 0" in body
    assert "focusOverlayAvailabilityRecovery" in body
    assert "return false" in body


def test_paused_recovery_focus_helper_reports_success():
    start = QML.index("function focusOverlayRecoveryAction")
    body = QML[start : QML.index("    function restoreOverlayDisplaySelectionFromBridge", start)]
    assert "resumeOverlayRecovery.forceActiveFocus()" in body
    assert "return true" in body


def test_overlay_pulse_exposes_non_color_accessible_status():
    start = QML.index("id: overlaySurface")
    body = QML[start : QML.index("            Timer", start)]
    assert 'Accessible.name: "Overlay beat pulse"' in body
    assert "Accessible.value: pulseOpacity > 0 ? \"active\" : \"idle\"" in body
    assert "valid live BPM" in body
    assert "reduced motion disables pulsing" in body


def test_overlay_pulse_requires_finite_positive_bpm_to_run():
    start = QML.index("id: beatPulse")
    body = QML[start : QML.index("            onTriggered", start)]
    assert "status === \"Tracking\"" in body
    assert "isFinite(Number(window.liveBridge.tempo))" in body
    assert "Number(window.liveBridge.tempo) > 0" in body


def test_overlay_pulse_status_label_exposes_running_state():
    start = QML.index('objectName: "overlayPulseStatus"')
    label = QML[start : QML.index('objectName: "overlayHarmony"', start)]
    assert 'text: "Beat pulse "' in label
    assert "beatPulse.running" in label
    assert "Accessible.role: Accessible.Status" in label


def test_overlay_pulse_fades_with_reduced_motion_aware_easing():
    start = QML.index("property real pulseOpacity")
    body = QML[start : QML.index("            Timer", start)]
    assert "Behavior on pulseOpacity" in body
    assert "Easing.OutQuad" in body
    assert "reduceMotion" in body


def test_live_evidence_group_reports_pulse_status():
    start = QML.index('objectName: "overlayLiveEvidenceGroup"')
    body = QML[start : QML.index('objectName: "overlayEscapeHint"', start)]
    assert '"; beat pulse "' in body
    assert "beatPulse.running" in body


def test_recovery_pause_focus_helper_reports_target_result():
    start = QML.index("function focusOverlayRecoveryPauseState")
    body = QML[start : QML.index("    function resetOverlayRecoveryAttempts", start)]
    assert "target.forceActiveFocus()" in body
    assert "return target !== null && target !== undefined" in body


def test_availability_recovery_helper_reports_target_result():
    start = QML.index("function focusOverlayAvailabilityRecovery")
    body = QML[start : QML.index("    function focusOverlayBridgeSettingsFailure", start)]
    assert "target.forceActiveFocus()" in body
    assert "return target !== null && target !== undefined" in body


def test_restored_state_focus_helper_reports_target_result():
    start = QML.index("function focusOverlayRestoredState")
    body = QML[start : QML.index("    function focusOverlayHiddenState", start)]
    assert "target.forceActiveFocus()" in body
    assert "Fresh capture can now be accepted." in body
    assert "return target !== null && target !== undefined" in body


def test_hidden_state_focus_helper_reports_target_result():
    start = QML.index("function focusOverlayHiddenState")
    body = QML[start : QML.index("    function focusOverlayRestoreFailure", start)]
    assert "target.forceActiveFocus()" in body
    assert "Retained settings remain available." in body
    assert "return target !== null && target !== undefined" in body


def test_restore_failure_focus_helper_reports_target_result():
    start = QML.index("function focusOverlayRestoreFailure")
    body = QML[start : QML.index("    function focusOverlayUnlockState", start)]
    assert "target.forceActiveFocus()" in body
    assert "Live evidence is stale until a fresh capture arrives." in body
    assert "return target !== null && target !== undefined" in body


def test_unlock_state_focus_helper_reports_target_result():
    start = QML.index("function focusOverlayUnlockState")
    body = QML[start : QML.index("    function focusOverlaySettingsRecoveryFailure", start)]
    assert "target.forceActiveFocus()" in body
    assert "Interaction state is current." in body
    assert "return target !== null && target !== undefined" in body


def test_capture_evidence_transition_reports_focus_target_result():
    start = QML.index("function transitionCaptureEvidence")
    body = QML[start : QML.index("    function focusOverlayAvailabilityRecovery", start)]
    assert "overlayCaptureEvidenceStale = stale" in body
    assert "focusTarget.forceActiveFocus()" in body
    assert "Fresh accepted capture is available." in body
    assert "return focusTarget !== null && focusTarget !== undefined" in body


def test_bridge_settings_failure_propagates_focus_result():
    start = QML.index("function focusOverlayBridgeSettingsFailure")
    body = QML[start : QML.index("    function focusOverlayRestoredState", start)]
    assert "overlayLaunchFallbackActive = true" in body
    assert "overlayDisplayRecoveryPending = true" in body
    assert "return window.focusOverlayAvailabilityRecovery" in body


def test_settings_recovery_failure_propagates_focus_result():
    start = QML.index("function focusOverlaySettingsRecoveryFailure")
    body = QML[start : QML.index("    Connections", start)]
    assert "overlayCaptureEvidenceStale = true" in body
    assert "overlayDisplayRecoveryPending = true" in body
    assert "return window.focusOverlayRestoreFailure" in body


def test_no_display_recovery_propagates_focus_result():
    start = QML.index("function focusOverlayNoDisplayRecovery")
    body = QML[start : QML.index("        function onScreensChanged", start)]
    assert "overlayDisplaySelectionRequired = true" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "return window.focusOverlayAvailabilityRecovery" in body


def test_display_loss_and_geometry_failure_propagate_focus_results():
    loss_start = QML.index("function focusOverlayDisplayLoss")
    loss = QML[loss_start : QML.index("    function announceOverlayGeometryState", loss_start)]
    geometry_start = QML.index("function focusOverlayGeometrySyncFailure")
    geometry = QML[geometry_start : QML.index("    function focusOverlayDisplaySelectionRequired", geometry_start)]
    assert "return window.focusOverlayAvailabilityRecovery" in loss
    assert "return window.focusOverlayAvailabilityRecovery" in geometry
    assert "overlayDisplayRecoveryPending = true" in loss
    assert "overlayCaptureEvidenceStale = true" in geometry


def test_display_selection_required_propagates_focus_result():
    start = QML.index("function focusOverlayDisplaySelectionRequired")
    body = QML[start : QML.index("    Connections", start)]
    assert "overlayDisplaySelectionRequired = true" in body
    assert "overlayDisplayRecoveryPending = true" in body
    assert "return window.focusOverlayAvailabilityRecovery" in body


def test_geometry_state_announcement_reports_validity():
    start = QML.index("function announceOverlayGeometryState")
    body = QML[start : QML.index("    function focusOverlayDisplayLoss", start)]
    assert "geometry.width <= 0" in body
    assert "return false" in body
    assert "Live evidence remains stale until fresh capture arrives." in body
    assert "return true" in body


def test_capture_startup_and_attach_failures_report_handled_result():
    attach_start = QML.index("function focusOverlayAttachFailure")
    attach = QML[attach_start : QML.index("    function focusOverlayCaptureStartupFailure", attach_start)]
    startup_start = QML.index("function focusOverlayCaptureStartupFailure")
    startup = QML[startup_start : QML.index("    function focusOverlayRecoveryAction", startup_start)]
    assert "overlayDisplaySelectionRequired = true" in attach
    assert "return window.focusOverlayAvailabilityRecovery" in attach
    assert "overlayDisplayRecoveryPending = true" in startup
    assert "return true" in startup


def test_display_recovery_failure_helper_is_defined_and_propagates_result():
    start = QML.index("function focusOverlayDisplayRecoveryFailure")
    body = QML[start : QML.index("    function focusOverlaySizeRecovery", start)]
    assert "overlayDisplayRecoveryPending = true" in body
    assert "overlayDisplaySelectionRequired = true" in body
    assert "overlayCaptureEvidenceStale = true" in body
    assert "return window.focusOverlayAvailabilityRecovery" in body


def test_overlay_field_recovery_helpers_report_focus_success():
    opacity_start = QML.index("function focusOverlayOpacityEditRecovery")
    opacity = QML[opacity_start : QML.index("    function focusOverlayDisplaySelectionRecovery", opacity_start)]
    display_start = QML.index("function focusOverlayDisplaySelectionRecovery")
    display = QML[display_start : QML.index("    function focusOverlayDisplayRecoveryFailure", display_start)]
    size_start = QML.index("function focusOverlaySizeRecovery")
    size = QML[size_start : QML.index("    function handleOverlayClickThroughButtonAction", size_start)]
    assert "overlayOpacityControl.forceActiveFocus()" in opacity and "return true" in opacity
    assert "overlayDisplay.forceActiveFocus()" in display and "return true" in display
    assert "overlaySize.forceActiveFocus()" in size and "return true" in size
