import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    visible: true
    width: 1000; height: 700
    minimumWidth: 600; minimumHeight: 440
    title: "Beatglass"
    onClosing: {
        window.overlayWasVisibleBeforeAvailabilityLoss = false
        window.overlayDisplayRecoveryPending = false
        if (overlayBridge) overlayBridge.hideOverlay()
    }
    property var metadataBridge: metadata
    property var playbackBridge: typeof playbackSession !== "undefined" ? playbackSession : null
    property var liveBridge: livePresentation
    property var analysisJobBridge: typeof analysisJob !== "undefined" ? analysisJob : null
    readonly property bool analysisResultMatchesSelection: analysisJobBridge !== null
        && selectedAudioUrl.toString() !== ""
        && analysisJobBridge.sourcePath !== ""
        && analysisJobBridge.sourcePath === selectedAudioUrl.toLocalFile()
    property var captureMonitorBridge: typeof captureMonitor !== "undefined" ? captureMonitor : null
    property var endpointSelectionBridge: typeof endpointSelection !== "undefined" ? endpointSelection : null
    property var themePreferencesBridge: typeof themePreferences !== "undefined" ? themePreferences : null
    property var overlayBridge: typeof macOverlayController !== "undefined" ? macOverlayController : null
    property bool overlayDisplaySelectionRequired: false
    property bool overlayDisplayRecoveryPending: false
    property bool overlayWasVisibleBeforeAvailabilityLoss: false
    property bool overlayHiddenByUser: false
    property bool overlayLaunchFallbackActive: false
    // UI-visible count of native overlay recovery requests; the bridge performs retry work.
    property int overlayRecoveryAttemptCount: 0
    property bool overlayRecoveryPausedByUser: false
    readonly property int overlayRecoveryAttemptLimit: 3
    property int overlayGeometryRevision: 0
    property int overlaySettingsRevision: 0
    property bool overlayOpacityEditInFlight: false
    property real overlayPendingOpacity: -1
    property int overlayPreferredScreenIndex: -1
    property string overlayPreferredDisplayName: ""
    property int overlayPreferredSizeIndex: 1
    property bool overlayPersistedDiagnosticDismissed: false
    property url browseFolder: ""
    property var catalogRows: []
    property var recursiveCatalogRows: []
    property bool recursiveCatalogActive: false
    property bool sortDescending: false
    property string selectedName: ""
    property url selectedAudioUrl: ""
    property string latestPublishedSourceSelectionStatus: ""
    property string metadataStatus: "Choose an audio file to inspect its metadata."
    property bool metadataRetryPending: false
    property string visualizationSelectionResetReason: ""
    property string visualizationRecoveryAnnouncement: ""
    property string persistedRecoveryOutcomeAnnouncementKey: ""
    property bool persistedRecoverySelectionActivated: false
    property string activeAnalysisOutcomeAnnouncementKey: ""
    property string activeWorkerOutcomeAnnouncementKey: ""
    property bool storageReopenPending: false
    property string themePreferenceAnnouncement: ""
    property string catalogDiagnostic: ""
    readonly property bool libraryAnalysisResultReady: selectedName !== ""
        && analysisJobBridge !== null
        && (analysisJobBridge.status === "Complete" || analysisJobBridge.result !== "")
    readonly property string acceptedAnalysisIdentity: typeof visualizationPresentation === "undefined"
        || !libraryAnalysisResultReady
        || visualizationPresentation.generation < 0 || visualizationPresentation.revision < 0
        ? ""
        : (selectedName + "|generation:" + visualizationPresentation.generation + "|revision:" + visualizationPresentation.revision)
    property bool overlayVisualStateRecoveryRequired: false
    property bool overlayCaptureEvidenceStale: false
    readonly property string overlayRecoveryEvidenceLabel: overlayCaptureEvidenceStale ? "Live evidence stale until fresh capture" : "Live evidence current"
    property string overlayRecoveryAnnouncement: ""
    property string overlayPulseAnnouncement: "Beat pulse is idle until fresh Monitoring capture and valid BPM are available."
    readonly property bool playbackMatchesSelection: playbackBridge && selectedAudioUrl.toString() !== ""
        && playbackBridge.source.toString() === selectedAudioUrl.toString()
    readonly property bool analysisCacheIdentityReady: cacheContent.text.trim() !== "" && cacheEngine.text.trim() !== "" && cacheConfig.text.trim() !== "" && cacheMode.acceptableInput
    property bool analysisCacheIdentityDirty: true
        && cacheEngine.text !== "" && cacheConfig.text !== ""
        && (cacheMode.text === "offline" || cacheMode.text === "live")
    readonly property int recursiveAudioResultCount: {
        let count = 0
        for (let i = 0; i < recursiveCatalogRows.length; ++i)
            if (recursiveCatalogRows[i].audio) ++count
        return count
    }
    Shortcut {
        objectName: "libraryFilterFocusShortcut"
        sequence: "Ctrl+L"
        context: Qt.ApplicationShortcut
        onActivated: window.focusLibraryFilter()
    }
    Shortcut {
        objectName: "libraryRecursiveCatalogShortcut"
        sequence: "Ctrl+Alt+R"
        context: Qt.ApplicationShortcut
        enabled: window.browseFolder.toString() !== "" && !window.recursiveCatalogActive
        onActivated: window.requestLibraryRecursiveCatalog()
    }
    Shortcut {
        objectName: "libraryFilterFocusShortcut"
        sequence: "Ctrl+L"
        context: Qt.ApplicationShortcut
        onActivated: { search.forceActiveFocus(); search.selectAll(); window.announceVisualizationRecovery("Library filter focused.") }
    }
    Shortcut {
        objectName: "libraryRecursiveCatalogShortcut"
        sequence: "Ctrl+Alt+R"
        context: Qt.ApplicationShortcut
        enabled: window.browseFolder.toString() !== "" && !window.recursiveCatalogActive
        onActivated: addFolderRecursively.click()
    }
    Shortcut {
        objectName: "libraryRefreshShortcut"
        sequence: "Ctrl+R"
        context: Qt.ApplicationShortcut
        enabled: window.browseFolder.toString() !== ""
        onActivated: { files.refresh(); window.refreshCatalog(); window.resetLibraryBrowserSelection("Folder contents refreshed from the keyboard; entry selection cleared.") }
    }
    Shortcut {
        objectName: "libraryParentFolderShortcut"
        sequence: "Ctrl+Alt+Up"
        context: Qt.ApplicationShortcut
        enabled: window.browseFolder.toString() !== "" && files.parentFolder.toString() !== ""
        onActivated: window.navigateLibraryParentFolder()
    }
    Shortcut { objectName: "libraryChooseFolderShortcut"; sequence: "Ctrl+Alt+F"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 0; folderPicker.open() } }
    Shortcut {
        objectName: "librarySortShortcut"
        sequence: "Ctrl+Alt+S"
        context: Qt.ApplicationShortcut
        onActivated: {
            window.toggleLibrarySort(sortOrder)
        }
    }
    Shortcut {
        objectName: "librarySortFocusShortcut"
        sequence: "Ctrl+Alt+Shift+O"
        context: Qt.ApplicationShortcut
        onActivated: window.focusLibrarySortControl()
    }
    Shortcut {
        objectName: "libraryMetadataFocusShortcut"
        sequence: "Ctrl+Alt+Shift+M"
        context: Qt.ApplicationShortcut
        enabled: window.selectedAudioUrl.toString() !== ""
        onActivated: window.focusLibraryMetadataStatus()
    }
    Shortcut {
        objectName: "libraryActivateEntryShortcut"
        sequence: "Ctrl+Alt+Enter"
        context: Qt.ApplicationShortcut
        enabled: !window.recursiveCatalogActive && browser.currentIndex >= 0
        onActivated: window.activateLibraryFocusedEntry()
    }
    Shortcut {
        objectName: "playbackSeekBackShortcut"
        sequence: "Alt+Left"
        context: Qt.ApplicationShortcut
        enabled: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped" && window.playbackMatchesSelection
        onActivated: { window.playbackBridge.seek(Math.max(0, window.playbackBridge.currentFrame - 5 * playbackSampleRate.value), window.playbackBridge.epoch + 1); window.announceCaptureRecoveryState("Playback sought backward five seconds from the keyboard.") }
    }
    Shortcut {
        objectName: "playbackSeekForwardShortcut"
        sequence: "Alt+Right"
        context: Qt.ApplicationShortcut
        enabled: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped" && window.playbackMatchesSelection
        onActivated: { window.playbackBridge.seek(window.playbackBridge.currentFrame + 5 * playbackSampleRate.value, window.playbackBridge.epoch + 1); window.announceCaptureRecoveryState("Playback sought forward five seconds from the keyboard.") }
    }
    Binding { target: seekBackPlayback; property: "enabled"; value: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped" && window.playbackMatchesSelection }
    Binding { target: seekForwardPlayback; property: "enabled"; value: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped" && window.playbackMatchesSelection }
    Binding { target: seekToFrame; property: "enabled"; value: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped" && window.playbackMatchesSelection }
    Shortcut {
        objectName: "overlayEscapeShortcut"
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: if (window.overlayBridge && window.overlayBridge.clickThrough)
                         window.unlockOverlayAndRestoreFocus()
    }
    Shortcut {
        objectName: "overlayRestoreFocusShortcut"
        sequence: "Ctrl+Alt+Shift+W"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            navigation.currentIndex = 1
            window.show()
            window.raise()
            window.requestActivate()
            restoreOverlay.forceActiveFocus()
            window.announceCaptureRecoveryState("Restore overlay control focused.")
        }
    }
    Shortcut {
        objectName: "overlayPulseStatusFocusShortcut"
        sequence: "Ctrl+Alt+Shift+B"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge !== null
        onActivated: window.focusOverlayPulseStatus()
    }
    Shortcut {
        objectName: "overlayRetryFocusShortcut"
        sequence: "Ctrl+Alt+Shift+Y"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && !window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
        onActivated: window.focusOverlayRetryShortcut()
    }
    Shortcut {
        objectName: "overlayReapplyFocusShortcut"
        sequence: "Ctrl+Alt+Shift+O"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            navigation.currentIndex = 1
            window.show()
            window.raise()
            window.requestActivate()
            reapplyOverlaySettings.forceActiveFocus()
            window.announceCaptureRecoveryState("Reapply saved settings control focused.")
        }
    }
    Shortcut {
        objectName: "overlayRetentionFocusShortcut"
        sequence: "Ctrl+Alt+Shift+S"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge !== null
        onActivated: {
            navigation.currentIndex = 1
            window.show()
            window.raise()
            window.requestActivate()
            overlaySettingsRetentionReviewAction.forceActiveFocus()
            window.announceCaptureRecoveryState("Retained overlay settings review control focused.")
        }
    }
    Shortcut {
        objectName: "overlayUnlockFocusShortcut"
        sequence: "Ctrl+Alt+U"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.clickThrough
        onActivated: {
            window.focusOverlayUnlockShortcut()
        }
    }
    Shortcut {
        objectName: "overlaySummaryReviewShortcut"
        sequence: "Ctrl+Alt+Shift+M"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge !== null
        onActivated: {
            window.focusOverlaySummaryReviewShortcut()
        }
    }
    Shortcut {
        objectName: "overlayHideFocusShortcut"
        sequence: "Ctrl+Alt+J"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
        onActivated: {
            window.focusOverlayHideShortcut()
        }
    }
    Shortcut {
        objectName: "overlayShowFocusShortcut"
        sequence: "Ctrl+Alt+W"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            window.focusOverlayShowShortcut()
        }
    }
    Shortcut {
        objectName: "overlaySizeFocusShortcut"
        sequence: "Ctrl+Alt+Z"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            window.focusOverlaySizeShortcut()
        }
    }
    Shortcut {
        objectName: "overlayClickThroughFocusShortcut"
        sequence: "Ctrl+Alt+K"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            window.focusOverlayClickThroughShortcut()
        }
    }
    Shortcut {
        objectName: "overlayOpacityFocusShortcut"
        sequence: "Ctrl+Alt+L"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            window.focusOverlayOpacityShortcut()
        }
    }
    Shortcut {
        objectName: "overlayClickThroughShortcut"
        sequence: "Ctrl+Alt+I"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: window.handleOverlayClickThroughShortcut()
    }
    Shortcut {
        objectName: "overlayRetryShortcut"
        sequence: "Ctrl+Alt+Y"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && !window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: window.handleOverlayRetryShortcut()
    }
    Shortcut {
        objectName: "overlayClearSafeModeShortcut"
        sequence: "Ctrl+Alt+Shift+C"
        context: Qt.ApplicationShortcut
        enabled: window.overlayRecoveryAttemptCount >= window.overlayRecoveryAttemptLimit
        onActivated: window.handleOverlayClearSafeModeShortcut()
    }
    Shortcut {
        objectName: "overlayStopRecoveryShortcut"
        sequence: "Ctrl+Alt+Shift+X"
        context: Qt.ApplicationShortcut
        enabled: window.overlayRecoveryAttemptCount > 0 && window.overlayRecoveryAttemptCount < window.overlayRecoveryAttemptLimit
        onActivated: window.handleOverlayStopShortcut()
    }
    Shortcut {
        objectName: "overlayResumeRecoveryShortcut"
        sequence: "Ctrl+Alt+Shift+R"
        context: Qt.ApplicationShortcut
        enabled: window.overlayRecoveryPausedByUser && window.overlayBridge
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: window.handleOverlayResumeShortcut()
    }
    Shortcut {
        objectName: "overlayPausedRecoveryFocusShortcut"
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        enabled: window.overlayRecoveryPausedByUser && (!window.overlayBridge || !window.overlayBridge.clickThrough)
        onActivated: window.handleOverlayPausedFocusShortcut()
    }
    Shortcut {
        objectName: "overlayPausedRecoveryPageShortcut"
        sequence: "Ctrl+Alt+Shift+Pause"
        context: Qt.ApplicationShortcut
        enabled: window.overlayRecoveryPausedByUser
        onActivated: window.handleOverlayPausedPageShortcut()
    }
    Shortcut {
        objectName: "overlayDisplayFocusShortcut"
        sequence: "Ctrl+Alt+D"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.screenNames.length > 0
        onActivated: window.focusOverlayDisplayShortcut()
    }
    Shortcut {
        objectName: "overlayReapplyShortcut"
        sequence: "Ctrl+Alt+Shift+A"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            window.focusOverlayReapplyShortcut()
        }
    }
    Shortcut {
        objectName: "overlayHideShortcut"
        sequence: "Ctrl+Alt+H"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: window.handleOverlayHideToggleShortcut()
    }
    Shortcut {
        objectName: "overlayOpacityResetShortcut"
        sequence: "Ctrl+Alt+Shift+O"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            const resetOpacity = 0.9
            window.handleOverlayOpacityResetAction(resetOpacity)
        }
    }
    Shortcut {
        objectName: "overlaySizeShortcut"
        sequence: "Ctrl+Alt+O"
        context: Qt.ApplicationShortcut
        enabled: window.overlayBridge && window.overlayBridge.available
            && window.overlayBridge.screenNames.length > 0
            && !window.overlayDisplaySelectionRequired
        onActivated: {
            window.handleOverlaySizeShortcut()
        }
    }
    Shortcut {
        objectName: "playbackPauseShortcut"
        sequence: "Ctrl+Alt+Space"
        context: Qt.ApplicationShortcut
        enabled: window.playbackBridge !== null
            && (window.playbackBridge.state === "Playing" || window.playbackBridge.state === "Paused")
        onActivated: window.togglePlaybackPause()
    }
    Shortcut {
        objectName: "visualizationResumeShortcut"
        sequence: "Ctrl+Alt+R"
        context: Qt.ApplicationShortcut
        enabled: window.playbackBridge !== null && window.playbackBridge.state === "Paused"
        onActivated: {
            const resumed = window.playbackBridge.resume()
            window.announceVisualizationRecovery(resumed
                ? "Playback resumed from keyboard shortcut; accepted snapshot retained."
                : "Playback resume from keyboard shortcut failed; accepted snapshot retained.")
        }
    }
    Shortcut {
        objectName: "analysisCancelShortcut"
        sequence: "Ctrl+Alt+C"
        context: Qt.ApplicationShortcut
        enabled: window.analysisJobBridge !== null && window.analysisJobBridge.canCancel
        onActivated: {
            window.analysisJobBridge.cancel()
            window.announceVisualizationRecovery("Analysis cancelled from the keyboard; waiting for a replacement accepted snapshot.")
        }
    }
    Shortcut {
        objectName: "analysisRetryShortcut"
        sequence: "Ctrl+Alt+Y"
        context: Qt.ApplicationShortcut
        enabled: window.analysisJobBridge !== null && window.analysisJobBridge.canRetry
        onActivated: window.retryVisualizationAnalysis()
    }
    Shortcut {
        objectName: "analysisDiscardShortcut"
        sequence: "Ctrl+Alt+D"
        context: Qt.ApplicationShortcut
        enabled: window.analysisJobBridge !== null && window.analysisJobBridge.canRetry
        onActivated: {
            window.analysisJobBridge.discard()
            window.announceVisualizationRecovery("Analysis recovery discarded from the keyboard; focus remains on recovery controls.")
        }
    }
    Shortcut {
        objectName: "analysisBatchRetryShortcut"
        sequence: "Ctrl+Alt+B"
        context: Qt.ApplicationShortcut
        enabled: window.analysisJobBridge !== null
            && window.analysisJobBridge.retryBatch !== undefined
            && window.analysisJobBridge.batchAcceptedCount > 0
            && (window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled")
        onActivated: {
            const retried = window.analysisJobBridge.retryBatch()
            window.announceVisualizationRecovery(retried
                ? "Analysis batch retry queued from the keyboard."
                : "Analysis batch retry was rejected; no replacement work was queued.")
        }
    }
    Shortcut {
        objectName: "analysisBatchCancelShortcut"
        sequence: "Ctrl+Alt+K"
        context: Qt.ApplicationShortcut
        enabled: window.analysisJobBridge !== null
            && window.analysisJobBridge.cancelBatch !== undefined
            && window.analysisJobBridge.batchAcceptedCount > 0
            && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running")
        onActivated: {
            window.analysisJobBridge.cancelBatch()
            window.announceVisualizationRecovery("Analysis batch cancelled from the keyboard; waiting for a replacement accepted snapshot.")
        }
    }
    Shortcut {
        objectName: "analysisBatchDiscardShortcut"
        sequence: "Ctrl+Alt+Shift+D"
        context: Qt.ApplicationShortcut
        enabled: window.analysisJobBridge !== null
            && window.analysisJobBridge.discardBatch !== undefined
            && window.analysisJobBridge.batchAcceptedCount > 0
            && (window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled")
        onActivated: {
            window.analysisJobBridge.discardBatch()
            window.announceVisualizationRecovery("Analysis batch recovery discarded from the keyboard; focus remains on recovery controls.")
        }
    }
    Shortcut {
        objectName: "storageReopenShortcut"
        sequence: "Ctrl+Alt+O"
        context: Qt.ApplicationShortcut
        enabled: typeof storageRecovery !== "undefined" && storageRecovery.open !== undefined
            && (storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0)
        onActivated: {
            storageRecovery.open()
            window.announceVisualizationRecovery("Storage reopen requested from the keyboard; persisted recovery will refresh when available.")
        }
    }
    Shortcut {
        objectName: "visualizationStopShortcut"
        sequence: "Ctrl+Alt+Shift+S"
        context: Qt.ApplicationShortcut
        enabled: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped"
        onActivated: window.stopPlaybackWithRecovery("Playback stopped from keyboard shortcut; accepted snapshot cleared.")
    }
    Shortcut { objectName: "librarySearchShortcut"; sequence: "Ctrl+F"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 0; window.focusLibraryFilter("Library filter focused; type to filter the current folder.") } }
    Shortcut { objectName: "libraryClearFilterShortcut"; sequence: "Ctrl+Alt+Shift+Backspace"; context: Qt.ApplicationShortcut; enabled: search.text !== ""; onActivated: window.clearLibraryFilter(browser) }
    Shortcut { objectName: "libraryFolderPathFocusShortcut"; sequence: "Ctrl+Alt+Shift+I"; context: Qt.ApplicationShortcut; onActivated: window.focusLibraryFolderPath() }
    Shortcut { objectName: "libraryParentFocusShortcut"; sequence: "Ctrl+Alt+Shift+W"; context: Qt.ApplicationShortcut; enabled: window.browseFolder.toString() !== "" && files.parentFolder.toString() !== ""; onActivated: window.focusLibraryParentAction() }
    Shortcut { objectName: "libraryRecursiveFocusShortcut"; sequence: "Ctrl+Alt+Shift+Z"; context: Qt.ApplicationShortcut; enabled: window.browseFolder.toString() !== "" && !window.recursiveCatalogActive; onActivated: window.focusLibraryRecursiveAction() }
    Shortcut { objectName: "libraryRecursiveReturnFocusShortcut"; sequence: "Ctrl+Alt+Shift+J"; context: Qt.ApplicationShortcut; enabled: window.recursiveCatalogActive; onActivated: window.focusLibraryRecursiveReturnAction() }
    Shortcut { objectName: "libraryRecursiveAnalyzeFocusShortcut"; sequence: "Ctrl+Alt+Shift+K"; context: Qt.ApplicationShortcut; enabled: window.recursiveCatalogActive; onActivated: window.focusLibraryRecursiveAnalysisAction() }
    Shortcut { objectName: "libraryBatchStatusFocusShortcut"; sequence: "Ctrl+Alt+Shift+U"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: window.focusLibraryBatchStatus() }
    Shortcut { objectName: "libraryRecoveryStatusFocusShortcut"; sequence: "Ctrl+Alt+Shift+V"; context: Qt.ApplicationShortcut; enabled: typeof storageRecovery !== "undefined"; onActivated: { navigation.currentIndex = 0; analysisRecoveryStatus.forceActiveFocus(); window.announceVisualizationRecovery("Library analysis recovery status focused.") } }
    Shortcut { objectName: "libraryAdmissionStatusFocusShortcut"; sequence: "Ctrl+Alt+Shift+X"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: { navigation.currentIndex = 0; analysisAdmissionStatus.forceActiveFocus(); window.announceVisualizationRecovery("Library analysis admission status focused.") } }
    Shortcut { objectName: "libraryAnalysisProgressFocusShortcut"; sequence: "Ctrl+Alt+Shift+G"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: { navigation.currentIndex = 0; analysisProgress.forceActiveFocus(); window.announceVisualizationRecovery("Library analysis progress focused.") } }
    Shortcut { objectName: "libraryAnalysisSourceFocusShortcut"; sequence: "Ctrl+Alt+Shift+T"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: { navigation.currentIndex = 0; analysisSourcePath.forceActiveFocus(); window.announceVisualizationRecovery("Library analysis source path focused.") } }
    Shortcut { objectName: "libraryAnalysisFreshnessFocusShortcut"; sequence: "Ctrl+Alt+Shift+0"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: { navigation.currentIndex = 0; analysisFreshness.forceActiveFocus(); window.announceVisualizationRecovery("Library analysis freshness focused.") } }
    Shortcut { objectName: "libraryAnalysisProvenanceFocusShortcut"; sequence: "Ctrl+Alt+Shift+9"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: { navigation.currentIndex = 0; analysisProvenance.forceActiveFocus(); window.announceVisualizationRecovery("Library analysis provenance focused.") } }
    Shortcut { objectName: "libraryAnalysisUncertaintyFocusShortcut"; sequence: "Ctrl+Alt+Shift+8"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: { navigation.currentIndex = 0; analysisUncertainty.forceActiveFocus(); window.announceVisualizationRecovery("Library analysis uncertainty focused.") } }
    Shortcut { objectName: "libraryAnalysisFailureFocusShortcut"; sequence: "Ctrl+Alt+Shift+7"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: window.focusLibraryAnalysisFailure() }
    Shortcut { objectName: "libraryAnalysisQueueFocusShortcut"; sequence: "Ctrl+Alt+Shift+6"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: window.focusLibraryAnalysisQueue() }
    Shortcut { objectName: "libraryBatchProgressFocusShortcut"; sequence: "Ctrl+Alt+Shift+5"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: window.focusLibraryBatchProgress() }
    Shortcut { objectName: "libraryBatchResultFocusShortcut"; sequence: "Ctrl+Alt+Shift+4"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: window.focusLibraryBatchResult() }
    Shortcut { objectName: "libraryBatchPublicationFocusShortcut"; sequence: "Ctrl+Alt+Shift+3"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: window.focusLibraryBatchPublication() }
    Shortcut { objectName: "libraryBatchSourceFocusShortcut"; sequence: "Ctrl+Alt+Shift+2"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null; onActivated: window.focusLibraryBatchSource() }
    Shortcut { objectName: "libraryLatestPublishedSourceFocusShortcut"; sequence: "Ctrl+Alt+Shift+B"; context: Qt.ApplicationShortcut; enabled: window.analysisJobBridge !== null && window.analysisJobBridge.batchLastSource !== ""; onActivated: window.focusLibraryLatestPublishedSource() }
    Shortcut { objectName: "libraryRecoveryNextActionFocusShortcut"; sequence: "Ctrl+Alt+Shift+1"; context: Qt.ApplicationShortcut; enabled: typeof storageRecovery !== "undefined"; onActivated: window.focusLibraryRecoveryNextAction() }
    Shortcut { objectName: "libraryMetadataErrorFocusShortcut"; sequence: "Ctrl+Alt+Shift+F"; context: Qt.ApplicationShortcut; enabled: window.selectedAudioUrl.toString() !== "" && window.metadataStatus.indexOf("Error:") === 0; onActivated: window.focusLibraryMetadataErrorGuidance() }
    Shortcut { objectName: "libraryClearShortcut"; sequence: "Ctrl+Alt+Backspace"; context: Qt.ApplicationShortcut; enabled: window.selectedAudioUrl.toString() !== ""; onActivated: window.resetLibraryBrowserSelection("Library selection cleared; choose another source for fresh analysis.") }
    Shortcut { objectName: "libraryReopenShortcut"; sequence: "Ctrl+Alt+Shift+P"; context: Qt.ApplicationShortcut; enabled: window.selectedAudioUrl.toString() !== "" && window.playbackBridge !== null && !window.playbackMatchesSelection; onActivated: window.reopenLibrarySelectionFromShortcut() }
    Shortcut { objectName: "libraryPlayShortcut"; sequence: "Ctrl+Alt+P"; context: Qt.ApplicationShortcut; enabled: window.selectedAudioUrl.toString() !== "" && window.playbackBridge !== null; onActivated: window.playLibrarySelectionFromShortcut() }
    Shortcut { objectName: "libraryPlaybackFocusShortcut"; sequence: "Ctrl+Alt+Shift+Y"; context: Qt.ApplicationShortcut; enabled: window.selectedAudioUrl.toString() !== ""; onActivated: window.focusLibraryPlaybackAction() }
    Shortcut { objectName: "libraryRefreshFocusShortcut"; sequence: "Ctrl+Alt+Shift+Q"; context: Qt.ApplicationShortcut; enabled: window.browseFolder.toString() !== ""; onActivated: window.focusLibraryRefreshAction() }
    Shortcut { objectName: "libraryChooseFolderFocusShortcut"; sequence: "Ctrl+Alt+Shift+D"; context: Qt.ApplicationShortcut; onActivated: window.focusLibraryFolderChooser() }
    Shortcut { objectName: "libraryCatalogStatusFocusShortcut"; sequence: "Ctrl+Alt+Shift+C"; context: Qt.ApplicationShortcut; enabled: window.browseFolder.toString() !== ""; onActivated: window.focusLibraryCatalogStatus() }
    Shortcut { objectName: "librarySelectionStatusFocusShortcut"; sequence: "Ctrl+Alt+Shift+E"; context: Qt.ApplicationShortcut; onActivated: window.focusLibrarySelectionStatus() }
    Shortcut { objectName: "libraryShortcutHelpFocusShortcut"; sequence: "Ctrl+Alt+Shift+H"; context: Qt.ApplicationShortcut; onActivated: window.focusLibraryShortcutHelp() }
    Shortcut { objectName: "libraryAnalyzeShortcut"; sequence: "Ctrl+Alt+A"; context: Qt.ApplicationShortcut; enabled: window.selectedAudioUrl.toString() !== "" && window.analysisJobBridge !== null && window.analysisJobBridge.submitSelectedFile !== undefined && window.analysisJobBridge.status !== "Queued" && window.analysisJobBridge.status !== "Running"; onActivated: window.analyzeLibrarySelectionFromShortcut() }
    Shortcut { objectName: "libraryAnalysisFocusShortcut"; sequence: "Ctrl+Alt+Shift+N"; context: Qt.ApplicationShortcut; enabled: window.selectedAudioUrl.toString() !== ""; onActivated: window.focusLibraryAnalysisAction() }
    Shortcut { objectName: "libraryBrowserShortcut"; sequence: "Ctrl+Alt+L"; context: Qt.ApplicationShortcut; onActivated: window.focusLibraryResults() }
    Shortcut { objectName: "libraryResultsShortcut"; sequence: "Ctrl+Alt+Shift+L"; context: Qt.ApplicationShortcut; onActivated: window.focusLibraryResults() }
    Shortcut { objectName: "libraryRecursiveResultsFocusShortcut"; sequence: "Ctrl+Alt+Shift+R"; context: Qt.ApplicationShortcut; enabled: window.recursiveCatalogActive; onActivated: window.focusLibraryResults() }
    Shortcut { objectName: "libraryRecursiveReturnShortcut"; sequence: "Ctrl+Alt+Shift+B"; context: Qt.ApplicationShortcut; enabled: window.recursiveCatalogActive; onActivated: window.returnLibraryFromRecursiveShortcut() }
    Shortcut { objectName: "libraryTabShortcut"; sequence: "Ctrl+Shift+1"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 0; libraryTab.forceActiveFocus(); window.announceVisualizationRecovery("Library tab focused.") } }
    Shortcut { objectName: "djTabShortcut"; sequence: "Ctrl+Shift+2"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 1; djTab.forceActiveFocus() } }
    Shortcut { objectName: "settingsTabShortcut"; sequence: "Ctrl+Shift+3"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 2; settingsTab.forceActiveFocus() } }
    Shortcut {
        objectName: "bulkRecoveryDiscardShortcut"
        sequence: "Ctrl+Alt+Shift+A"
        context: Qt.ApplicationShortcut
        enabled: typeof storageRecovery !== "undefined" && storageRecovery.discardRuns !== undefined
            && storageRecovery.recoveryAvailable
            && !window.storageReopenPending
        onActivated: {
            storageRecovery.discardRuns(storageRecovery.retryableRunIds)
            window.announceVisualizationRecovery("All saved recovery runs discarded from the keyboard.")
        }
    }
    Shortcut {
        objectName: "persistedRecoveryShortcut"
        sequence: "Ctrl+Shift+R"
        context: Qt.ApplicationShortcut
        enabled: typeof persistedRecoveryRuns !== "undefined" && persistedRecoveryRuns.enabled
        onActivated: {
            navigation.currentIndex = 0
            persistedRecoveryRuns.forceActiveFocus()
            window.announceVisualizationRecovery("Persisted recovery selector focused; choose one of " + persistedRecoveryRuns.count + " saved runs to retry or discard.")
        }
    }
    Shortcut {
        objectName: "persistedRecoveryRetryShortcut"
        sequence: "Ctrl+Shift+Y"
        context: Qt.ApplicationShortcut
        enabled: typeof persistedRecoveryRuns !== "undefined"
            && persistedRecoveryRuns.enabled
            && typeof storageRecovery !== "undefined"
            && storageRecovery.selectedRunId !== ""
            && storageRecovery.analysisRetryable
        onActivated: {
            const runId = storageRecovery.selectedRunId
            storageRecovery.retrySelectedRun()
            window.overlayPersistedDiagnosticDismissed = true
            window.announceVisualizationRecovery("Persisted recovery retry admitted for " + runId + " from keyboard; reset status cleared.")
            retryPersistedRecovery.forceActiveFocus()
        }
    }
    Shortcut {
        objectName: "persistedRecoveryDiscardShortcut"
        sequence: "Ctrl+Shift+D"
        context: Qt.ApplicationShortcut
        enabled: typeof persistedRecoveryRuns !== "undefined"
            && persistedRecoveryRuns.enabled
            && typeof storageRecovery !== "undefined"
            && storageRecovery.selectedRunId !== ""
        onActivated: {
            const runId = storageRecovery.selectedRunId
            discardPersistedRecoveryButton.forceActiveFocus()
            window.announceVisualizationRecovery("Persisted recovery discard focused for " + runId + "; press Enter to review the confirmation dialog.")
        }
    }
    function transitionCaptureEvidence(stale, message, focusTarget) {
        window.overlayCaptureEvidenceStale = stale
        if (message !== "")
            window.announceCaptureRecoveryState(message + " " + (stale ? "Live evidence is stale until fresh capture arrives." : "Fresh accepted capture is available."))
        if (focusTarget)
            focusTarget.forceActiveFocus()
        return focusTarget !== null && focusTarget !== undefined
    }
    function focusOverlayAvailabilityRecovery(message, target) {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        window.announceCaptureRecoveryState(message + " " + (window.overlayCaptureEvidenceStale ? "Live evidence is stale until fresh capture arrives." : "Retained settings remain available."))
        if (target)
            target.forceActiveFocus()
        return target !== null && target !== undefined
    }
    function focusOverlayBridgeSettingsFailure(message, target) {
        window.overlayLaunchFallbackActive = true
        window.overlayDisplayRecoveryPending = true
        return window.focusOverlayAvailabilityRecovery(message + " Persisted settings remain retained for retry.", target)
    }
    function focusOverlayRestoredState(message, target) {
        window.show()
        window.raise()
        window.requestActivate()
        window.announceCaptureRecoveryState(message + " " + (window.overlayCaptureEvidenceStale ? "Live evidence remains stale until fresh capture." : "Fresh capture can now be accepted."))
        if (target)
            target.forceActiveFocus()
        return target !== null && target !== undefined
    }
    function focusOverlayHiddenState(message, target) {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        window.announceCaptureRecoveryState(message + " " + (window.overlayCaptureEvidenceStale ? "Live evidence remains stale until fresh capture." : "Retained settings remain available."))
        if (target)
            target.forceActiveFocus()
        return target !== null && target !== undefined
    }
    function focusOverlayRestoreFailure(message, target) {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        window.announceCaptureRecoveryState(message + " Live evidence is stale until a fresh capture arrives.")
        if (target)
            target.forceActiveFocus()
        return target !== null && target !== undefined
    }
    function focusOverlayUnlockState(message, target) {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        window.announceCaptureRecoveryState(message + " " + (window.overlayCaptureEvidenceStale ? "Live evidence remains stale until fresh capture." : "Interaction state is current."))
        if (target)
            target.forceActiveFocus()
        return target !== null && target !== undefined
    }
    function focusOverlaySettingsRecoveryFailure(message, target) {
        window.overlayCaptureEvidenceStale = true
        window.overlayLaunchFallbackActive = true
        window.overlayDisplayRecoveryPending = true
        return window.focusOverlayRestoreFailure(message, target)
    }
    Connections {
        target: typeof storageRecovery !== "undefined" ? storageRecovery : null
        function onStatusChanged() {
            if (storageRecovery.status === "Opening storage") {
                window.storageReopenPending = true
                window.activeWorkerOutcomeAnnouncementKey = ""
                window.persistedRecoveryOutcomeAnnouncementKey = ""
                window.persistedRecoverySelectionActivated = false
                return
            }
            if (storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0) {
                window.storageReopenPending = false
                window.activeWorkerOutcomeAnnouncementKey = ""
                window.persistedRecoveryOutcomeAnnouncementKey = ""
                window.persistedRecoverySelectionActivated = false
                if (typeof reopenStorage !== "undefined")
                    reopenStorage.forceActiveFocus()
                window.announceVisualizationRecovery("Storage is unavailable; reopen storage before retrying analysis recovery.")
            }
        }
        function onRetryableRunsChanged() {
            if (!window.storageReopenPending || storageRecovery.status !== "Storage open")
                return
            window.storageReopenPending = false
            window.persistedRecoverySelectionActivated = false
            if (storageRecovery.retryableRunIds.length > 0) {
                window.announceVisualizationRecovery("Storage reopened; " + storageRecovery.retryableRunIds.length + " persisted recovery runs are available.")
                if (typeof persistedRecoveryRuns !== "undefined")
                    persistedRecoveryRuns.forceActiveFocus()
            } else {
                window.announceVisualizationRecovery("Storage reopened; no persisted recovery runs are available.")
                libraryTab.forceActiveFocus()
            }
        }
        function onSelectedRunIdChanged() {
            window.persistedRecoverySelectionActivated = false
            window.persistedRecoveryOutcomeAnnouncementKey = ""
        }
        function onAnalysisStatusChanged() {
            const status = storageRecovery.analysisStatus
            if (storageRecovery.selectedRunId === "") {
                if (status !== "Analysis failed" && status !== "Analysis cancelled") {
                    window.activeAnalysisOutcomeAnnouncementKey = ""
                    return
                }
                const activeOutcomeKey = status
                if (window.activeAnalysisOutcomeAnnouncementKey === activeOutcomeKey)
                    return
                window.activeAnalysisOutcomeAnnouncementKey = activeOutcomeKey
                if (status === "Analysis failed")
                    window.announceVisualizationRecovery("Active analysis failed; Retry analysis is available.")
                else
                    window.announceVisualizationRecovery("Active analysis was canceled; Retry analysis is available.")
                retryAdmittedAnalysis.forceActiveFocus()
                return
            }
            window.activeAnalysisOutcomeAnnouncementKey = ""
            if (!window.persistedRecoverySelectionActivated || !storageRecovery.analysisRetryable) {
                window.persistedRecoveryOutcomeAnnouncementKey = ""
                return
            }
            if (status !== "Analysis failed" && status !== "Analysis cancelled") {
                window.persistedRecoveryOutcomeAnnouncementKey = ""
                return
            }
            const outcomeKey = storageRecovery.selectedRunId + "|" + status
            if (window.persistedRecoveryOutcomeAnnouncementKey === outcomeKey)
                return
            window.persistedRecoveryOutcomeAnnouncementKey = outcomeKey
            if (status === "Analysis failed") {
                window.announceVisualizationRecovery("Persisted recovery run " + storageRecovery.selectedRunId + " failed; retry is available.")
                retryPersistedRecovery.forceActiveFocus()
            } else {
                window.announceVisualizationRecovery("Persisted recovery run " + storageRecovery.selectedRunId + " was canceled; retry is available.")
                retryPersistedRecovery.forceActiveFocus()
            }
        }
    }
    Shortcut {
        objectName: "visualizationWaveformShortcut"
        sequence: "Ctrl+1"
        context: Qt.ApplicationShortcut
        onActivated: { visualizationCanvas.viewMode = "waveform"; waveformView.forceActiveFocus() }
    }
    Shortcut {
        objectName: "visualizationSpectrumShortcut"
        sequence: "Ctrl+2"
        context: Qt.ApplicationShortcut
        onActivated: { visualizationCanvas.viewMode = "spectrum"; spectrumView.forceActiveFocus() }
    }
    property string selectedTheme: "Light"
    readonly property bool compactHeader: width < 760
    readonly property var themeNames: ["Light", "Dark", "High contrast", "Purple", "Blue", "Yellow"]
    readonly property var themeBackgrounds: ["#f2f4ef", "#10151c", "#000000", "#251f3e", "#102c43", "#332a1b"]
    readonly property var themeForegrounds: ["#142c2c", "#f1f6f7", "#ffffff", "#f9f5ff", "#effaff", "#fff9e8"]
    readonly property var themeAccentColors: ["#126f60", "#8ee5cc", "#8cffe2", "#d4a8ff", "#75d9ff", "#ffd45e"]
    property int selectedThemeIndex: themePreferencesBridge ? themePreferencesBridge.themeIndex : 0
    function selectThemeIndex(index) {
        const nextIndex = Math.max(0, Math.min(window.themeNames.length - 1, index))
        window.selectedThemeIndex = nextIndex
        window.selectedTheme = window.themeNames[nextIndex]
        window.themePreferenceAnnouncement = "Theme selected: " + window.selectedTheme + ". " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "Reduced motion is enabled." : "Full motion is enabled.")
        if (window.themePreferencesBridge)
            window.themePreferencesBridge.themeIndex = nextIndex
        if (typeof visualizationCanvas !== "undefined")
            visualizationCanvas.requestPaint()
        return true
    }
    function setReduceMotionPreference(enabled) {
        if (!window.themePreferencesBridge)
            return false
        window.themePreferencesBridge.reduceMotion = enabled
        window.themePreferenceAnnouncement = enabled
            ? "Reduced motion enabled for " + window.selectedTheme + "; beat pulse and presentation transitions are minimized."
            : "Reduced motion disabled for " + window.selectedTheme + "; standard presentation transitions restored."
        return true
    }
    function resetAppearancePreferences() {
        if (!window.themePreferencesBridge)
            return false
        window.themePreferencesBridge.reset()
        window.selectThemeIndex(0)
        window.setReduceMotionPreference(false)
        window.themePreferenceAnnouncement = "Appearance reset to Light theme with full motion."
        return true
    }
    // Keep the visual selection synchronized when preferences are restored or
    // changed by another settings surface. Assignments from the controls below
    // intentionally remain immediate; this connection handles external changes.
    Connections {
        target: window.themePreferencesBridge
        function onThemeIndexChanged() {
            const index = Math.max(0, Math.min(window.themeNames.length - 1,
                                               window.themePreferencesBridge.themeIndex))
            window.selectedThemeIndex = index
            window.selectedTheme = window.themeNames[index]
            window.themePreferenceAnnouncement = "Theme preference restored: " + window.selectedTheme + " (" + (window.themePreferencesBridge.reduceMotion ? "reduced motion" : "full motion") + ")."
            if (typeof visualizationCanvas !== "undefined")
                visualizationCanvas.requestPaint()
        }
        function onReduceMotionChanged() {
            window.themePreferenceAnnouncement = window.themePreferencesBridge.reduceMotion
                ? "Reduced motion enabled for " + window.selectedTheme + "; static visualization snapshots remain available."
                : "Reduced motion disabled for " + window.selectedTheme + "; standard presentation transitions restored."
        }
    }
    Connections {
        target: window.overlayBridge
        function onAvailableChanged() {
            if (!window.overlayBridge)
                return
            if (!window.overlayBridge.available) {
                window.overlayOpacityEditInFlight = false
                window.overlayPendingOpacity = -1
                window.overlayLaunchFallbackActive = true
                window.overlayDisplayRecoveryPending = true
                window.overlayDisplaySelectionRequired = true
                window.announceCaptureRecoveryState("Overlay actions disabled; control-window fallback remains available until the native overlay recovers.")
                window.overlayWasVisibleBeforeAvailabilityLoss = djOverlay.visible
                if (djOverlay.visible) {
                    window.hideOverlayAndRestoreControls()
                    window.overlayHiddenByUser = false
                    window.overlayDisplayRecoveryPending = true
                    window.overlayDisplaySelectionRequired = true
                }
                // Keep keyboard recovery actionable after native bridge/display loss.
                if (typeof retryOverlay !== "undefined")
                    retryOverlay.forceActiveFocus()
            } else {
                const synced = window.syncOverlaySettingsFromBridge()
                window.overlayOpacityEditInFlight = false
                window.overlayPendingOpacity = -1
                if (synced) {
                    window.announceCaptureRecoveryState("Overlay actions restored; native display settings are available.")
                } else {
                    window.focusOverlayBridgeSettingsFailure("Overlay bridge recovered, but settings remain unavailable; control-window fallback remains active.", retryOverlay)
                }
                if (synced && window.overlayWasVisibleBeforeAvailabilityLoss) {
                    window.overlayWasVisibleBeforeAvailabilityLoss = false
                    if (!window.overlayRecoveryPausedByUser) {
                        window.recoverOverlayAfterDisplayChange()
                        window.focusOverlayRestoredState("DJ overlay display restored; retry is available while native overlay recovery completes.", retryOverlay)
                    }
                    else
                        window.announceCaptureRecoveryState("DJ overlay bridge recovered; recovery remains paused until Resume recovery is chosen.")
                }
            }
        }
    }
    Component.onCompleted: {
        if (overlayBridge && !overlayBridge.attach(djOverlay, window)) {
            window.focusOverlayAttachFailure("Native overlay attachment failed; control-window fallback remains active.")
        } else {
            const synced = window.syncOverlaySettingsFromBridge()
            if (!synced) {
                window.focusOverlayAttachFailure("Native overlay attached, but retained settings are unavailable; control-window fallback remains active.")
            }
        }
        if (window.overlayBridge && !window.captureMonitorBridge) {
            window.focusOverlayCaptureStartupFailure()
        }
        refreshCatalog()
        if (typeof libraryPreferences !== "undefined" && libraryPreferences.folder !== "") {
            browseFolder = libraryPreferences.folder
            refreshCatalog()
        }
    }
    function focusOverlayAttachFailure(message) {
        window.overlayLaunchFallbackActive = true
        window.overlayDisplayRecoveryPending = true
        window.overlayDisplaySelectionRequired = true
        window.overlayCaptureEvidenceStale = true
        return window.focusOverlayAvailabilityRecovery(message + " Live evidence is stale until fresh capture arrives.", overlayDisplay)
    }
    function focusOverlayCaptureStartupFailure() {
        window.overlayCaptureEvidenceStale = true
        window.overlayDisplayRecoveryPending = true
        window.announceCaptureRecoveryState("Overlay capture is unavailable; displayed estimates are stale until monitoring connects. Recovery remains available from the control window.")
        return true
    }
    function focusOverlayRecoveryAction(message) {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        window.announceCaptureRecoveryState(message + " " + window.overlayRecoveryEvidenceLabel + ".")
        resumeOverlayRecovery.forceActiveFocus()
        return true
    }

    function restoreOverlayDisplaySelectionFromBridge() {
        if (!window.overlayBridge) {
            window.overlayDisplayRecoveryPending = true
            window.overlayDisplaySelectionRequired = true
            window.overlayLaunchFallbackActive = true
            window.overlayCaptureEvidenceStale = true
            return false
        }
        if (window.overlayBridge.displayName !== "")
            window.overlayPreferredDisplayName = window.overlayBridge.displayName
        if (window.overlayBridge.screenIndex >= 0)
            window.overlayPreferredScreenIndex = window.overlayBridge.screenIndex
        const savedIndex = window.overlayPreferredDisplayName !== ""
            ? window.overlayBridge.screenNames.indexOf(window.overlayPreferredDisplayName)
            : window.overlayPreferredScreenIndex
        window.overlayDisplaySelectionRequired = !window.overlayBridge.available
            || window.overlayBridge.screenNames.length === 0 || savedIndex < 0
        window.overlayDisplayRecoveryPending = window.overlayBridge.screenNames.length === 0 || window.overlayDisplaySelectionRequired
        window.overlayLaunchFallbackActive = window.overlayDisplaySelectionRequired
        if (window.overlayDisplaySelectionRequired)
            window.overlayCaptureEvidenceStale = true
        return window.overlayBridge.available && savedIndex >= 0
    }
    function restoreOverlayVisualStateFromBridge() {
        if (!window.overlayBridge) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            return false
        }
        const savedGeometry = window.overlayBridge.geometry
        if (!savedGeometry || !isFinite(savedGeometry.x) || !isFinite(savedGeometry.y)
                || !isFinite(savedGeometry.width) || !isFinite(savedGeometry.height)
                || savedGeometry.width <= 0 || savedGeometry.height <= 0) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            const geometryDisplay = window.overlayPreferredDisplayName !== "" ? window.overlayPreferredDisplayName : "the saved display"
            const geometryReason = savedGeometry && (!isFinite(savedGeometry.x) || !isFinite(savedGeometry.y))
                    ? "position" : (savedGeometry && (!isFinite(savedGeometry.width) || !isFinite(savedGeometry.height))
                        ? "size" : "geometry")
            window.announceCaptureRecoveryState("Persisted overlay " + geometryReason + " unavailable for " + geometryDisplay + "; control-window fallback remains active.")
            return false
        }
        if (!window.overlayBridge.resizeOverlay(savedGeometry.width, savedGeometry.height)) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            window.announceCaptureRecoveryState("Persisted overlay geometry could not be restored; control-window fallback remains active.")
            return false
        }
        const savedOpacity = Number(window.overlayBridge.opacity)
        if (isFinite(savedOpacity) && savedOpacity >= 0.2 && savedOpacity <= 1.0) {
            if (!window.overlayBridge.setOpacity(savedOpacity)) {
                window.overlayLaunchFallbackActive = true
                window.overlayDisplayRecoveryPending = true
                window.announceCaptureRecoveryState("Persisted overlay opacity could not be restored; control-window fallback remains active.")
                return false
            }
        } else {
            if (!window.overlayBridge.setOpacity(0.9)) {
                window.overlayLaunchFallbackActive = true
                window.overlayDisplayRecoveryPending = true
                window.announceCaptureRecoveryState("Persisted overlay opacity unavailable; safe opacity could not be applied.")
                return false
            }
            window.announceCaptureRecoveryState("Persisted overlay opacity unavailable; safe 90 percent opacity applied.")
        }
        window.announceCaptureRecoveryState("Persisted overlay geometry and opacity restored.")
        window.overlayRecoveryPausedByUser = false
        window.syncOverlaySizePreferenceFromGeometry(savedGeometry)
        return true
    }
    function syncOverlaySizePreferenceFromGeometry(geometry) {
        if (!geometry || !isFinite(geometry.x) || !isFinite(geometry.y) || !isFinite(geometry.width) || !isFinite(geometry.height) || geometry.width <= 0 || geometry.height <= 0) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            return false
        }
        const sizes = [[250, 110], [310, 130], [430, 180]]
        let best = 0
        let bestDistance = Number.MAX_VALUE
        for (let index = 0; index < sizes.length; ++index) {
            const distance = Math.abs(geometry.width - sizes[index][0])
                    + Math.abs(geometry.height - sizes[index][1])
            if (distance < bestDistance) {
                best = index
                bestDistance = distance
            }
        }
        window.overlayPreferredSizeIndex = best
        return true
    }
    function syncOverlayGeometryStateFromBridge() {
        if (!window.overlayBridge || !window.overlayBridge.geometry
                || !isFinite(window.overlayBridge.geometry.x)
                || !isFinite(window.overlayBridge.geometry.y)
                || !isFinite(window.overlayBridge.geometry.width)
                || !isFinite(window.overlayBridge.geometry.height)
                || window.overlayBridge.geometry.width <= 0
                || window.overlayBridge.geometry.height <= 0) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            return false
        }
        const geometry = window.overlayBridge.geometry
        if (window.overlayBridge.displayName !== "")
            window.overlayPreferredDisplayName = window.overlayBridge.displayName
        if (window.overlayBridge.screenIndex >= 0)
            window.overlayPreferredScreenIndex = window.overlayBridge.screenIndex
        window.syncOverlaySizePreferenceFromGeometry(geometry)
        window.overlayGeometryRevision += 1
        return true
    }
    function syncOverlaySettingsStateFromBridge() {
        if (!window.overlayBridge) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            return false
        }
        const geometryReady = window.syncOverlayGeometryStateFromBridge()
        const opacity = Number(window.overlayBridge.opacity)
        if (!isFinite(opacity) || opacity < 0.2 || opacity > 1.0 || !geometryReady) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            return false
        }
        window.overlaySettingsRevision += 1
        return geometryReady
    }
    function syncOverlaySettingsFromBridge() {
        if (!window.overlayBridge) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            return false
        }
        window.restoreOverlayDisplaySelectionFromBridge()
        if (!window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            window.announceCaptureRecoveryState("Overlay startup state is unavailable; control-window fallback remains active until a display recovers.")
            return false
        }
        const visualStateReady = window.restoreOverlayVisualStateFromBridge()
        window.overlayLaunchFallbackActive = false
        if (visualStateReady) {
            window.overlayDisplayRecoveryPending = false
            window.announceCaptureRecoveryState("Overlay settings synchronized from native state.")
            return true
        }
        window.overlayLaunchFallbackActive = true
        window.overlayDisplayRecoveryPending = true
        window.announceCaptureRecoveryState("Overlay settings could not be synchronized; control-window fallback remains active.")
        return false
    }
    function reapplyOverlayVisualState() {
        return window.restoreOverlayVisualStateFromBridge()
    }
    function refreshCatalog() {
        window.recursiveCatalogActive = false
        window.recursiveCatalogRows = []
        if (typeof recursiveResultsView !== "undefined") recursiveResultsView.currentIndex = -1
        window.catalogRows = window.libraryCatalogBridge && window.browseFolder.toString() !== ""
            ? window.libraryCatalogBridge.browse(window.browseFolder, search.text) : []
        window.catalogDiagnostic = window.libraryCatalogBridge && window.libraryCatalogBridge.lastDiagnostic !== undefined
                ? window.libraryCatalogBridge.lastDiagnostic : ""
    }
    function resetLibraryBrowserSelection(message) {
        window.clearSelectedAudio()
        browser.currentIndex = -1
        window.announceVisualizationRecovery(message)
        browser.forceActiveFocus()
    }
    function returnFromRecursiveResults() {
        window.recursiveCatalogActive = false
        recursiveResultsView.currentIndex = -1
        search.text = ""
        window.catalogDiagnostic = ""
        window.resetLibraryBrowserSelection("Returned to the current folder browser; the filter, entry selection, and selected source were cleared.")
    }
    function returnLibraryFromRecursiveShortcut() {
        navigation.currentIndex = 0
        window.returnFromRecursiveResults()
    }
    function navigateLibraryParentFolder() {
        window.browseFolder = files.parentFolder
        search.text = ""
        window.recursiveCatalogActive = false
        window.recursiveCatalogRows = []
        window.resetLibraryBrowserSelection("Moved to the parent Library folder; choose an audio source to inspect.")
    }
    function openLibraryFolderChooser() {
        window.announceVisualizationRecovery("Choose a Library folder dialog opened.")
        folderPicker.open()
        chooseFolder.forceActiveFocus()
    }
    function handleLibraryFolderChooserRejected() {
        window.announceVisualizationRecovery("Library folder selection canceled; the current folder remains open.")
        chooseFolder.forceActiveFocus()
    }
    function toggleLibrarySort(target) {
        navigation.currentIndex = 0
        window.sortDescending = !window.sortDescending
        browser.currentIndex = -1
        window.announceVisualizationRecovery("Library sorted by file name in " + (window.sortDescending ? "descending" : "ascending") + " order.")
        target.forceActiveFocus()
    }
    function focusLibrarySortControl() {
        navigation.currentIndex = 0
        sortOrder.forceActiveFocus()
        window.announceVisualizationRecovery("Library sort control focused; activate to reverse name order.")
    }
    function clearLibraryFilter(target) {
        search.text = ""
        target.forceActiveFocus()
        window.announceVisualizationRecovery("Library filter cleared; " + files.count + " folder entries are visible.")
    }
    function clearLibrarySelectionAndFilter(target) {
        window.clearSelectedAudio()
        search.text = ""
        browser.currentIndex = -1
        target.forceActiveFocus()
        window.announceVisualizationRecovery("Library selection and filter cleared; all folder entries are visible.")
    }
    function focusLibraryFilter(message) {
        search.forceActiveFocus()
        search.selectAll()
        window.announceVisualizationRecovery(message === undefined ? "Library filter focused." : message)
    }
    function focusLibraryResults() {
        navigation.currentIndex = 0
        if (window.recursiveCatalogActive) {
            recursiveResultsView.forceActiveFocus()
            window.announceVisualizationRecovery("Recursive Library results focused.")
        } else {
            browser.forceActiveFocus()
            window.announceVisualizationRecovery("Library folder results focused.")
        }
    }
    function focusLibraryFolderChooser() {
        navigation.currentIndex = 0
        chooseFolder.forceActiveFocus()
        window.announceVisualizationRecovery("Library folder chooser focused.")
    }
    function focusLibraryRecursiveAction() {
        navigation.currentIndex = 0
        addFolderRecursively.forceActiveFocus()
        window.announceVisualizationRecovery("Library recursive catalog action focused.")
    }
    function focusLibraryFolderPath() {
        navigation.currentIndex = 0
        folderPath.forceActiveFocus()
        window.announceVisualizationRecovery("Current Library folder path focused.")
    }
    function focusLibraryParentAction() {
        navigation.currentIndex = 0
        parentFolder.forceActiveFocus()
        window.announceVisualizationRecovery("Library parent-folder action focused.")
    }
    function focusLibraryCatalogStatus() {
        navigation.currentIndex = 0
        catalogResultCount.forceActiveFocus()
        window.announceVisualizationRecovery("Library catalog result count focused.")
    }
    function focusLibraryRecursiveAnalysisAction() {
        navigation.currentIndex = 0
        analyzeRecursiveResults.forceActiveFocus()
        window.announceVisualizationRecovery("Recursive Library analysis action focused.")
    }
    function focusLibraryRecursiveReturnAction() {
        navigation.currentIndex = 0
        returnToFolderBrowser.forceActiveFocus()
        window.announceVisualizationRecovery("Library return-to-folder action focused.")
    }
    function focusLibrarySelectionStatus() {
        navigation.currentIndex = 0
        librarySelectionStatus.forceActiveFocus()
        window.announceVisualizationRecovery("Library selection status focused.")
    }
    function focusLibraryPlaybackAction() {
        navigation.currentIndex = 0
        playSelectedTrack.forceActiveFocus()
        window.announceVisualizationRecovery("Selected Library playback action focused.")
    }
    function focusLibraryAnalysisAction() {
        navigation.currentIndex = 0
        analyzeSelectedAudio.forceActiveFocus()
        window.announceVisualizationRecovery("Selected Library analysis action focused.")
    }
    function focusLibraryMetadataErrorGuidance() {
        navigation.currentIndex = 0
        metadataErrorGuidance.forceActiveFocus()
        window.announceVisualizationRecovery("Library metadata error guidance focused.")
    }
    function focusLibraryRecoveryNextAction() {
        navigation.currentIndex = 0
        analysisRecoveryNextAction.forceActiveFocus()
        window.announceVisualizationRecovery("Library recovery next action focused.")
    }
    function focusLibraryAnalysisQueue() {
        navigation.currentIndex = 0
        analysisQueueProgress.forceActiveFocus()
        window.announceVisualizationRecovery("Library analysis queue progress focused.")
    }
    function focusLibraryAnalysisFailure() {
        navigation.currentIndex = 0
        analysisFailureReason.forceActiveFocus()
        window.announceVisualizationRecovery("Library analysis failure status focused.")
    }
    function focusLibraryShortcutHelp() {
        navigation.currentIndex = 0
        libraryShortcutHelp.forceActiveFocus()
        window.announceVisualizationRecovery("Library keyboard shortcut help focused.")
    }
    function focusLibraryRefreshAction() {
        navigation.currentIndex = 0
        refreshFolder.forceActiveFocus()
        window.announceVisualizationRecovery("Library refresh action focused.")
    }
    function focusLibraryMetadataStatus() {
        navigation.currentIndex = 0
        metadataStatusLabel.forceActiveFocus()
        window.announceVisualizationRecovery("Selected Library metadata status focused.")
    }
    function reopenLibrarySelectionFromShortcut() {
        const accepted = window.reopenSelectedLibraryTrack()
        reopenSelectedPlayback.forceActiveFocus()
        return accepted
    }
    function playLibrarySelectionFromShortcut() {
        const accepted = window.playSelectedLibraryTrack()
        playSelectedTrack.forceActiveFocus()
        return accepted
    }
    function analyzeLibrarySelectionFromShortcut() {
        const accepted = window.analysisJobBridge.submitSelectedFile(window.selectedAudioUrl.toLocalFile())
        if (accepted)
            window.clearVisualizationForCaptureState("Analysis queued; previous visualization evidence is stale until fresh accepted analysis.")
        window.announceVisualizationRecovery(accepted ? "Selected audio analysis queued." : "Selected audio analysis could not be queued; check the source and retry.")
        analyzeSelectedAudio.forceActiveFocus()
        return accepted
    }
    function activateLibraryFocusedEntry() {
        window.activateEntry(browser.currentIndex)
        window.announceVisualizationRecovery("Activated the focused Library entry.")
        browser.forceActiveFocus()
    }
    function requestLibraryRecursiveCatalog() {
        addFolderRecursively.click()
    }
    function focusLibraryBatchProgress() {
        navigation.currentIndex = 0
        analysisBatchProgressBar.forceActiveFocus()
        window.announceVisualizationRecovery("Library analysis batch progress focused.")
    }
    function focusLibraryBatchResult() {
        navigation.currentIndex = 0
        analysisBatchLastResult.forceActiveFocus()
        window.announceVisualizationRecovery("Library analysis batch result focused.")
    }
    function focusLibraryBatchPublication() {
        navigation.currentIndex = 0
        analysisBatchPublicationSummary.forceActiveFocus()
        window.announceVisualizationRecovery("Library analysis batch publication summary focused.")
    }
    function focusLibraryBatchSource() {
        navigation.currentIndex = 0
        analysisBatchLastSource.forceActiveFocus()
        window.announceVisualizationRecovery("Library analysis batch source focused.")
    }
    function focusLibraryLatestPublishedSource() {
        navigation.currentIndex = 0
        selectLatestPublishedSource.forceActiveFocus()
        window.announceVisualizationRecovery("Latest published analysis source action focused.")
    }
    function focusLibraryBatchStatus() {
        navigation.currentIndex = 0
        analysisBatchStatus.forceActiveFocus()
        window.announceVisualizationRecovery("Library analysis batch status focused.")
    }
    function browserEmptyMessage() {
        if (!window.browseFolder.toString()) return "Choose a folder to browse."
        if (files.status === FolderListModel.Error)
            return "This folder could not be read. Choose another folder or check its permissions."
        if (files.status === FolderListModel.Loading)
            return "Loading folder contents…"
        if (window.catalogDiagnostic !== "") return "This folder could not be read: " + window.catalogDiagnostic
        return "No matching audio files or folders. Try a different search or choose another folder."
    }
    function markOverlayRecoveryStale(reason) {
        window.overlayCaptureEvidenceStale = true
        window.overlayDisplayRecoveryPending = true
        if (reason !== "")
            window.announceCaptureRecoveryState(reason + " Live evidence is stale until fresh capture arrives.")
    }
    function restoreOverlayFallbackControls(reason) {
        window.overlayLaunchFallbackActive = true
        window.overlayDisplayRecoveryPending = true
        window.overlayCaptureEvidenceStale = true
        window.overlayHiddenByUser = false
        window.show()
        window.raise()
        window.requestActivate()
        overlayDisplay.forceActiveFocus()
        if (reason !== "")
            window.announceCaptureRecoveryState(reason + " Live evidence is stale until fresh capture arrives.")
    }
    function showOverlayAndHideControls() {
        window.overlayHiddenByUser = false
        if (!window.overlayBridge || !window.overlayBridge.available) {
            window.overlayLaunchFallbackActive = true
            window.markOverlayRecoveryStale("DJ overlay unavailable; control window remains available as the tray/taskbar fallback.")
            const savedDisplay = window.overlayPreferredDisplayName !== "" ? window.overlayPreferredDisplayName : "the saved display"
            window.restoreOverlayFallbackControls("DJ overlay unavailable; control window remains available as the tray/taskbar fallback. Persisted settings for " + savedDisplay + " remain retained for the next retry.")
            window.focusOverlayRestoreFailure("DJ overlay unavailable; control window remains available as the tray/taskbar fallback and persisted settings remain retained.", overlayDisplay)
            return false
        }
        if (!window.restoreOverlayVisualStateFromBridge()) {
            window.overlayLaunchFallbackActive = true
            window.markOverlayRecoveryStale("DJ overlay visual state unavailable; control window remains available as the tray/taskbar fallback.")
            window.restoreOverlayFallbackControls("DJ overlay visual state unavailable; control window remains available as the tray/taskbar fallback and overlay settings are retained.")
            window.focusOverlayRestoreFailure("DJ overlay visual state unavailable; control window remains available as the tray/taskbar fallback and overlay settings remain retained.", overlayDisplay)
            return false
        }
        window.overlayWasVisibleBeforeAvailabilityLoss = false
        window.overlayDisplayRecoveryPending = false
        if (!window.overlayBridge.showOverlay()) {
            window.overlayLaunchFallbackActive = true
            window.markOverlayRecoveryStale("DJ overlay could not be restored; control window remains available as the tray/taskbar fallback.")
            window.restoreOverlayFallbackControls("")
            window.focusOverlayRestoreFailure("DJ overlay could not be restored; control window remains available as the tray/taskbar fallback and overlay settings are retained.", overlayDisplay)
            return false
        }
        const restoredGeometry = window.overlayBridge.geometry
        window.overlayRecoveryPausedByUser = false
        window.announceCaptureRecoveryState(restoredGeometry && restoredGeometry.width > 0
            ? "DJ overlay relaunched at its persisted position and size."
            : "DJ overlay shown; output monitoring remains bound to the selected display.")
        window.overlayHiddenByUser = false
        window.hide()
        return true
    }
    function unlockOverlayAndRestoreFocus() {
        if (!window.overlayBridge || !window.overlayBridge.clickThrough)
            return false
        if (!window.overlayBridge.setClickThrough(false)) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
            window.focusOverlayUnlockState("DJ overlay could not be unlocked; control-window fallback remains available.", unlockOverlay)
            return false
        }
        window.overlayLaunchFallbackActive = false
        window.overlayDisplayRecoveryPending = false
        window.focusOverlayUnlockState("DJ overlay unlocked; keyboard and pointer interaction restored.", unlockOverlay)
        return true
    }
    function reapplyOverlaySettingsAndRestore() {
        if (!window.overlayBridge || !window.overlayBridge.available) {
            window.focusOverlaySettingsRecoveryFailure("Persisted overlay settings unavailable; control window remains available as the tray/taskbar fallback.", overlayDisplay)
            return false
        }
        if (window.overlayBridge.screenIndex >= window.overlayBridge.screenNames.length) {
            window.focusOverlaySettingsRecoveryFailure("Persisted overlay display is no longer available; choose another display before restoring the overlay.", overlayDisplay)
            return false
        }
        if (window.overlayBridge.screenIndex >= 0)
            if (!window.overlayBridge.moveToScreen(window.overlayBridge.screenIndex)) {
                window.focusOverlaySettingsRecoveryFailure("Persisted overlay display is unavailable; choose another display before restoring the overlay.", overlayDisplay)
                return false
            }
        if (!window.restoreOverlayVisualStateFromBridge()) {
            window.focusOverlaySettingsRecoveryFailure("Persisted overlay visual state is unavailable; choose a display and try restoring again.", overlayDisplay)
            return false
        }
        window.announceCaptureRecoveryState("Persisted overlay display, geometry, and opacity reapplied.")
        return window.showOverlayAndHideControls()
    }
    function focusOverlayReapplyShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Reapply settings is unavailable until the native overlay and a destination display are ready.", overlayDisplay)
            return false
        }
        window.announceCaptureRecoveryState("Reapplying saved overlay display, size, position, and opacity. " + (window.overlayCaptureEvidenceStale ? "Live evidence remains stale until fresh capture arrives." : "Retained settings are current."))
        return window.handleOverlayReapplyAction()
    }
    function handleOverlayReapplyAction() {
        return window.handleOverlayReapplyButtonAction()
    }
    function handleOverlayReapplyButtonAction() {
        if (window.overlayDisplaySelectionRequired || !window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Choose an available display before reapplying saved overlay settings.", overlayDisplay)
            return false
        }
        const reapplied = window.reapplyOverlaySettingsAndRestore()
        if (reapplied)
            reapplyOverlaySettings.forceActiveFocus()
        return reapplied
    }
    function handleOverlayUnlockAction() {
        if (!window.overlayBridge || !window.overlayBridge.available || !window.overlayBridge.clickThrough) {
            window.focusOverlayAvailabilityRecovery("Overlay unlock is unavailable because click-through mode is not active or the native overlay is unavailable.", unlockOverlay)
            return false
        }
        const restored = window.unlockOverlayAndRestoreFocus()
        if (restored)
            overlayInteractionMode.forceActiveFocus()
        return restored
    }
    function focusOverlaySummaryReviewShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        overlaySettingsSummaryReviewAction.forceActiveFocus()
        window.announceCaptureRecoveryState("Overlay summary review control focused; settings revision " + window.overlaySettingsRevision + (window.overlayDisplayRecoveryPending ? ", recovery pending." : ".") + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : " Live evidence is current."))
        return true
    }
    function focusOverlayUnlockShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || !window.overlayBridge.clickThrough) {
            window.focusOverlayAvailabilityRecovery("Unlock overlay is unavailable because click-through mode is not active or the native overlay is unavailable.", unlockOverlay)
            return false
        }
        unlockOverlay.forceActiveFocus()
        window.announceCaptureRecoveryState("Unlock overlay control focused; press Enter to restore interaction." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function focusOverlayShowShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (window.overlayDisplaySelectionRequired || !window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Show overlay is unavailable until an available display is selected.", overlayDisplay)
            return false
        }
        showOverlay.forceActiveFocus()
        window.announceCaptureRecoveryState("Show overlay control focused." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function focusOverlayHideShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Hide overlay is unavailable because the native overlay or destination display is not available; control-window fallback remains active.", showOverlay)
            return false
        }
        if (window.overlayHiddenByUser)
            restoreOverlay.forceActiveFocus()
        else
            hideOverlay.forceActiveFocus()
        window.announceCaptureRecoveryState("Hide overlay control focused; " + (window.overlayHiddenByUser ? "Restore is ready." : "Hide is ready.") + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function focusOverlaySizeShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay size is unavailable until the native overlay and a destination display are ready.", overlaySize)
            return false
        }
        overlaySize.forceActiveFocus()
        window.announceCaptureRecoveryState("Overlay size control focused; current size is " + overlaySize.currentText + "." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function focusOverlayClickThroughShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay click-through is unavailable until the native overlay and a destination display are ready.", clickThroughToggle)
            return false
        }
        clickThroughToggle.forceActiveFocus()
        window.announceCaptureRecoveryState("Overlay click-through control focused; current mode is " + (window.overlayBridge.clickThrough ? "enabled" : "disabled") + "." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function focusOverlayOpacityShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay opacity is unavailable until the native overlay and a destination display are ready.", overlayOpacityControl)
            return false
        }
        overlayOpacityControl.forceActiveFocus()
        window.announceCaptureRecoveryState("Overlay opacity control focused; current value is " + Math.round(window.overlayBridge.opacity * 100) + " percent." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function handleOverlayOpacityResetResult(applied, resetOpacity) {
        if (applied) {
            window.overlayOpacityEditInFlight = true
            window.overlayPendingOpacity = resetOpacity
            window.overlaySettingsRevision += 1
            window.announceCaptureRecoveryState("DJ overlay opacity reset to 90 percent.")
        } else {
            window.overlayCaptureEvidenceStale = true
            window.syncOverlaySettingsStateFromBridge()
            window.announceCaptureRecoveryState("DJ overlay opacity could not be reset; persisted opacity remains active. Live evidence remains stale until fresh capture arrives.")
        }
        overlayOpacityControl.forceActiveFocus()
    }
    function handleOverlayOpacityResetAction(resetOpacity) {
        if (window.overlayOpacityEditInFlight) {
            window.focusOverlayOpacityEditRecovery("Overlay opacity update is already pending; no duplicate reset was sent.")
            return false
        }
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay opacity reset is unavailable until the native overlay and a destination display are ready.", overlayOpacityControl)
            return false
        }
        const applied = window.overlayBridge.setOpacity(resetOpacity)
        window.handleOverlayOpacityResetResult(applied, resetOpacity)
        return applied
    }
    function handleOverlaySizeShortcut() {
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay size shortcut is unavailable until the native overlay and a destination display are ready.", overlaySize)
            return false
        }
        const sizes = [[250, 110], [310, 130], [430, 180]]
        const nextIndex = (window.overlayPreferredSizeIndex + 1) % sizes.length
        const resized = window.overlayBridge.resizeOverlay(sizes[nextIndex][0], sizes[nextIndex][1])
        window.handleOverlaySizeChangeResult(resized, nextIndex)
        return resized
    }
    function focusOverlayOpacityEditRecovery(message) {
        overlayOpacityControl.forceActiveFocus()
        window.announceCaptureRecoveryState(message + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? " Recovery remains pending." : ""))
        return true
    }
    function focusOverlayDisplaySelectionRecovery(message) {
        overlayDisplay.forceActiveFocus()
        window.announceCaptureRecoveryState(message + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? " Recovery remains pending." : ""))
        return true
    }
    function focusOverlayDisplayRecoveryFailure(message, target) {
        window.overlayDisplayRecoveryPending = true
        window.overlayDisplaySelectionRequired = true
        window.overlayCaptureEvidenceStale = true
        return window.focusOverlayAvailabilityRecovery(message, target || overlayDisplay)
    }
    function focusOverlaySizeRecovery(message) {
        overlaySize.forceActiveFocus()
        window.announceCaptureRecoveryState(message + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? " Recovery remains pending." : ""))
        return true
    }
    function handleOverlayClickThroughButtonAction(requestedClickThrough) {
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay interaction mode is unavailable until the native overlay and a destination display are ready.", clickThroughToggle)
            return false
        }
        const applied = window.overlayBridge.setClickThrough(requestedClickThrough)
        clickThroughToggle.checked = window.overlayBridge.clickThrough
        if (!applied || clickThroughToggle.checked !== requestedClickThrough) {
            window.overlayCaptureEvidenceStale = true
            window.announceCaptureRecoveryState("DJ overlay could not change input mode; keep the control window available and try again. Live evidence remains stale until fresh capture arrives.")
        } else {
            window.overlaySettingsRevision += 1
            window.overlayLaunchFallbackActive = false
            window.overlayDisplayRecoveryPending = false
            window.announceCaptureRecoveryState(requestedClickThrough ? "DJ overlay click-through enabled; press Escape or use Unlock overlay to restore interaction." : "DJ overlay interaction restored; keyboard and pointer input are available.")
        }
        overlayInteractionMode.forceActiveFocus()
        return applied && clickThroughToggle.checked === requestedClickThrough
    }
    function handleOverlayClickThroughShortcutAction() {
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay interaction shortcut is unavailable until the native overlay and a destination display are ready.", clickThroughToggle)
            return false
        }
        const requested = !window.overlayBridge.clickThrough
        if (!requested)
            return window.unlockOverlayAndRestoreFocus()
        return window.handleOverlayClickThroughButtonAction(true)
    }
    function handleOverlayClickThroughShortcut() {
        return window.handleOverlayClickThroughShortcutAction()
    }
    function handleOverlayRetryShortcutAction() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        return window.handleOverlayRetryButtonAction()
    }
    function handleOverlayRetryShortcut() {
        return window.handleOverlayRetryShortcutAction()
    }
    function handleOverlayRetryAction() {
        if (window.overlayRecoveryPausedByUser) {
            window.focusOverlayRecoveryPauseState("Overlay recovery is paused; Resume recovery before retrying.", resumeOverlayRecovery)
            return false
        }
        const restored = window.showOverlayAndHideControls()
        if (!restored)
            retryOverlay.forceActiveFocus()
        return restored
    }
    function handleOverlayRetryButtonAction() {
        if (window.overlayRecoveryPausedByUser) {
            window.focusOverlayRecoveryPauseState("Overlay recovery is paused; Resume recovery before retrying.", resumeOverlayRecovery)
            return false
        }
        if (window.overlayDisplaySelectionRequired || !window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Overlay retry is unavailable until the native overlay and a destination display are ready.", overlayDisplay)
            return false
        }
        window.overlayRecoveryPausedByUser = false
        window.overlayRecoveryAttemptCount += 1
        window.announceCaptureRecoveryState("DJ overlay retry requested (attempt " + window.overlayRecoveryAttemptCount + "); control window remains visible until the overlay is restored. Live evidence remains stale until fresh capture arrives.")
        const retried = window.showOverlayAndHideControls()
        overlayDisplayStatus.forceActiveFocus()
        return retried
    }
    function handleOverlayShowButtonAction() {
        if (window.overlayDisplaySelectionRequired || !window.overlayBridge || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Choose an available display before showing the overlay.", overlayDisplay)
            return false
        }
        window.announceCaptureRecoveryState("DJ overlay launch requested on the selected display; restoring persisted settings." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return window.handleOverlayShowAction()
    }
    function handleOverlayShowAction() {
        if (window.overlayDisplaySelectionRequired || !window.overlayBridge || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Choose an available display before showing the overlay.", overlayDisplay)
            return false
        }
        const restored = window.showOverlayAndHideControls()
        if (!restored) {
            window.overlayCaptureEvidenceStale = true
            retryOverlay.forceActiveFocus()
        }
        return restored
    }
    function focusOverlayDisplayShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Display selection is unavailable until the native overlay and a destination display are ready.", overlayDisplay)
            return false
        }
        overlayDisplay.forceActiveFocus()
        window.announceCaptureRecoveryState((window.overlayDisplaySelectionRequired
            ? "Overlay display selection required; choose an available display."
            : "Overlay display selector focused.") + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function focusOverlayRetryShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Retry overlay control is unavailable until the native overlay and a destination display are ready.", retryOverlay)
            return false
        }
        retryOverlay.forceActiveFocus()
        window.announceCaptureRecoveryState("Retry overlay control focused." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return true
    }
    function handleOverlayResumeShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        return window.handleOverlayResumeRecoveryAction()
    }
    function handleOverlayResumeRecoveryAction() {
        if (!window.overlayRecoveryPausedByUser) {
            window.focusOverlayRecoveryPauseState("Overlay recovery is no longer paused; no resume request was sent.", retryOverlay)
            return false
        }
        if (window.overlayDisplaySelectionRequired || !window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Overlay recovery cannot resume until the native overlay and a destination display are ready.", overlayDisplay)
            return false
        }
        window.overlayRecoveryPausedByUser = false
        window.overlayRecoveryAttemptCount = 1
        window.announceCaptureRecoveryState("DJ overlay recovery resumed (attempt 1); control window remains visible until the overlay is restored. Live evidence remains stale until fresh capture arrives.")
        const resumed = window.showOverlayAndHideControls()
        overlayDisplayStatus.forceActiveFocus()
        return resumed
    }
    function handleOverlayClearSafeModeShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        return window.handleOverlayClearSafeModeAction()
    }
    function handleOverlayClearSafeModeAction() {
        if (window.overlayRecoveryAttemptCount < window.overlayRecoveryAttemptLimit) {
            window.focusOverlayRecoveryPauseState("Overlay safe mode is not active; Retry overlay remains available when the display is ready.", retryOverlay)
            return false
        }
        window.resetOverlayRecoveryAttempts()
        window.announceCaptureRecoveryState("Overlay safe mode cleared; Retry overlay is available and focused. " + (window.overlayCaptureEvidenceStale ? "Live evidence remains stale until fresh capture arrives." : ""))
        retryOverlay.forceActiveFocus()
        return true
    }
    function handleOverlayStopShortcut() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        return window.handleOverlayStopRecoveryAction()
    }
    function handleOverlayStopRecoveryAction() {
        if (window.overlayRecoveryPausedByUser) {
            window.focusOverlayRecoveryPauseState("Overlay recovery is already paused; Resume recovery remains available.", resumeOverlayRecovery)
            return false
        }
        window.stopOverlayRecoveryAttempts()
        window.announceCaptureRecoveryState("Overlay recovery paused; Resume recovery is available. Live evidence remains stale until fresh capture arrives.")
        resumeOverlayRecovery.forceActiveFocus()
        return true
    }
    function handleOverlayPausedPageShortcut() {
        return window.focusOverlayRecoveryAction("Paused overlay recovery page opened; Resume recovery is ready. Live evidence remains stale until fresh capture arrives.")
    }
    function handleOverlayPausedFocusShortcut() {
        return window.focusOverlayRecoveryAction("Paused overlay recovery; Resume recovery focused. Live evidence remains stale until fresh capture arrives.")
    }
    function handleOverlayHideToggleShortcut() {
        if (window.overlayHiddenByUser) {
            return window.handleOverlayRestoreAction()
        } else {
            return window.handleOverlayHideAction()
        }
    }
    function handleOverlayRestoreAction() {
        return window.handleOverlayRestoreButtonAction()
    }
    function handleOverlayRestoreButtonAction() {
        if (window.overlayDisplaySelectionRequired || !window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayDisplayRecoveryFailure("Choose an available display before restoring the overlay.", overlayDisplay)
            return false
        }
        window.announceCaptureRecoveryState("Restoring the user-hidden DJ overlay; persisted settings will be reapplied." + (window.overlayCaptureEvidenceStale ? " Live evidence remains stale until fresh capture arrives." : ""))
        return window.handleOverlayShowAction()
    }
    function handleOverlayHideAction() {
        if (window.overlayHiddenByUser) {
            window.focusOverlayHiddenState("Overlay is already hidden by user choice; Restore overlay remains available.", showOverlay)
            return false
        }
        if (!window.overlayBridge || !window.overlayBridge.available || window.overlayBridge.screenNames.length === 0) {
            window.focusOverlayAvailabilityRecovery("Overlay hide is unavailable because the native overlay or destination display is not available; control-window fallback remains active.", showOverlay)
            return false
        }
        return window.hideOverlayAndRestoreControls()
    }
    function recoverOverlayAfterDisplayChange() {
        if (!window.overlayBridge || !window.overlayBridge.available
                || window.overlayBridge.screenNames.length === 0) {
            window.overlayDisplayRecoveryPending = true
            window.overlayDisplaySelectionRequired = true
            window.overlayLaunchFallbackActive = true
            return false
        }
        const savedNameIndex = window.overlayPreferredDisplayName !== ""
            ? window.overlayBridge.screenNames.indexOf(window.overlayPreferredDisplayName) : -1
        const savedIndex = savedNameIndex >= 0 ? savedNameIndex : window.overlayPreferredScreenIndex
        if (savedIndex < 0 || savedIndex >= window.overlayBridge.screenNames.length) {
            window.overlayDisplayRecoveryPending = true
            window.overlayDisplaySelectionRequired = true
            window.overlayLaunchFallbackActive = true
            window.overlayCaptureEvidenceStale = true
            window.announceCaptureRecoveryState("Overlay display selection is stale; choose an available display before restoring the overlay. Live evidence is stale until fresh capture arrives.")
            overlayDisplay.forceActiveFocus()
            return false
        }
        if (!window.overlayBridge.moveToScreen(savedIndex)
                || !window.restoreOverlayVisualStateFromBridge()) {
            window.overlayDisplayRecoveryPending = true
            window.overlayDisplaySelectionRequired = true
            window.overlayLaunchFallbackActive = true
            window.overlayCaptureEvidenceStale = true
            window.announceCaptureRecoveryState("DJ overlay display restored, but persisted visual state could not be reapplied. Live evidence is stale until fresh capture arrives.")
            overlayDisplay.forceActiveFocus()
            return false
        }
        window.overlayPreferredScreenIndex = savedIndex
        window.overlayPreferredDisplayName = window.overlayBridge.screenNames[savedIndex]
        window.overlaySettingsRevision += 1
        window.overlayLaunchFallbackActive = false
        window.overlayWasVisibleBeforeAvailabilityLoss = false
        window.overlayDisplaySelectionRequired = false
        if (window.overlayDisplayRecoveryPending) {
            window.overlayDisplayRecoveryPending = false
            if (window.overlayHiddenByUser) {
                const hiddenGeometry = window.overlayBridge.geometry
                const hiddenGeometrySummary = hiddenGeometry && hiddenGeometry.width > 0 && hiddenGeometry.height > 0
                    ? " at position " + hiddenGeometry.x + ", " + hiddenGeometry.y + ", size " + hiddenGeometry.width + " by " + hiddenGeometry.height + " pixels" : ""
                window.focusOverlayRestoredState("DJ overlay display recovered" + hiddenGeometrySummary + "; it remains hidden by user request. Use Restore overlay when ready.", restoreOverlay)
                return true
            }
            if (!window.overlayBridge.showOverlay()) {
                window.overlayDisplayRecoveryPending = true
                window.overlayDisplaySelectionRequired = true
                window.overlayLaunchFallbackActive = true
                window.restoreOverlayFallbackControls("DJ overlay could not be restored after display recovery; choose another display or retry.")
                return false
            }
            const recoveredDisplay = window.overlayPreferredDisplayName !== ""
                ? window.overlayPreferredDisplayName : "the saved display"
            const recoveredGeometry = window.overlayBridge.geometry
            const geometrySummary = recoveredGeometry && recoveredGeometry.width > 0 && recoveredGeometry.height > 0
                ? ", position " + recoveredGeometry.x + ", " + recoveredGeometry.y + ", size " + recoveredGeometry.width + " by " + recoveredGeometry.height + " pixels" : ""
            window.announceCaptureRecoveryState("DJ overlay recovered on " + recoveredDisplay + " with persisted visual settings" + geometrySummary + ", including " + Math.round(window.overlayBridge.opacity * 100) + " percent opacity.")
        }
        return true
    }
    function forgetUnavailableOverlayDisplay() {
        window.overlayPreferredDisplayName = ""
        window.overlayPreferredScreenIndex = -1
        window.overlaySettingsRevision += 1
        window.focusOverlayDisplayRecoveryFailure("Saved overlay display cleared; choose an available display to restore monitoring.", overlayDisplay)
    }
    function persistOverlayDisplaySelection(index, name) {
        if (!window.overlayBridge || index < 0 || index >= window.overlayBridge.screenNames.length) {
            window.overlayPreferredScreenIndex = -1
            window.overlayPreferredDisplayName = ""
            window.focusOverlayDisplayRecoveryFailure("Overlay display selection could not be persisted; choose an available display before restoring monitoring.", overlayDisplay)
            return false
        }
        if (!window.overlayBridge.moveToScreen(index)) {
            window.focusOverlayDisplayRecoveryFailure("Overlay display selection could not be applied; control-window fallback remains active.", overlayDisplay)
            return false
        }
        if (!window.restoreOverlayVisualStateFromBridge()) {
            window.focusOverlayDisplayRecoveryFailure("Overlay display selected, but persisted visual state could not be reapplied.", overlayDisplay)
            return false
        }
        if (!window.syncOverlaySettingsStateFromBridge()) {
            window.focusOverlayDisplayRecoveryFailure("Overlay display selected, but settings synchronization failed; choose or reconnect a display.", overlayDisplay)
            return false
        }
        if (!window.restoreOverlayVisualStateFromBridge()) {
            window.markOverlayRecoveryStale("Overlay display selected, but persisted visual state could not be reapplied.")
            window.overlayDisplaySelectionRequired = true
            window.overlayLaunchFallbackActive = true
            window.show()
            window.raise()
            window.requestActivate()
            overlayDisplay.forceActiveFocus()
            return false
        }
        // Commit the destination only after native geometry, opacity, and
        // display state have synchronized successfully. Prefer the bridge's
        // canonical identity so a stale picker label cannot survive relaunch.
        window.overlayPreferredScreenIndex = index
        window.overlayPreferredDisplayName = window.overlayBridge.displayName !== ""
            ? window.overlayBridge.displayName : name
        window.overlaySettingsRevision += 1
        window.overlayLaunchFallbackActive = false
        window.overlayWasVisibleBeforeAvailabilityLoss = false
        window.overlayDisplaySelectionRequired = false
        window.overlayDisplayRecoveryPending = false
        window.focusOverlayRestoredState("DJ overlay display selection persisted for " + name + "; Show, Restore, and Reapply actions are enabled.", overlaySettingsRetentionReviewAction)
        return true
    }
    function hideOverlayAndRestoreControls() {
        window.overlayWasVisibleBeforeAvailabilityLoss = false
        window.overlayDisplayRecoveryPending = false
        window.overlayHiddenByUser = true
        const hidden = !window.overlayBridge || window.overlayBridge.hideOverlay()
        window.overlayHiddenByUser = hidden
        if (!hidden) {
            window.overlayLaunchFallbackActive = true
            window.overlayDisplayRecoveryPending = true
        }
        const message = hidden
            ? (window.overlayBridge ? "DJ overlay hidden; output monitoring remains available in the control window." : "DJ overlay bridge unavailable; control window restored as the fallback.")
            : "DJ overlay could not be hidden; control-window fallback remains active."
        window.focusOverlayHiddenState(message, hidden ? restoreOverlay : hideOverlay)
    }
    function focusOverlayRecoveryPauseState(message, target) {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        window.announceCaptureRecoveryState(message + " " + (window.overlayCaptureEvidenceStale ? "Live evidence is stale until fresh capture arrives." : "Live evidence is current."))
        if (target)
            target.forceActiveFocus()
        return target !== null && target !== undefined
    }
    function resetOverlayRecoveryAttempts() {
        window.overlayRecoveryAttemptCount = 0
        window.overlayRecoveryPausedByUser = false
        window.focusOverlayRecoveryPauseState("DJ overlay safe mode cleared; retry is available when a display is ready.", retryOverlay)
    }
    function stopOverlayRecoveryAttempts() {
        window.overlayRecoveryAttemptCount = 0
        window.overlayRecoveryPausedByUser = true
        window.overlayCaptureEvidenceStale = true
        window.focusOverlayRecoveryPauseState("DJ overlay recovery paused; monitoring continues and retry can be started again from this page.", resumeOverlayRecovery)
    }
    function quitApplication() {
        window.overlayWasVisibleBeforeAvailabilityLoss = false
        window.overlayDisplayRecoveryPending = false
        window.overlayHiddenByUser = false
        if (window.overlayBridge)
            window.overlayBridge.hideOverlay()
        window.announceCaptureRecoveryState("Beatglass is quitting; overlay hidden and persisted settings retained.")
        Qt.quit()
    }
    function focusOverlayPulseStatus() {
        navigation.currentIndex = 1
        window.show()
        window.raise()
        window.requestActivate()
        overlayPulseStatus.forceActiveFocus()
        window.announceCaptureRecoveryState("Overlay beat pulse status focused. " + window.overlayPulseAnnouncement)
    }
    property var libraryCatalogBridge: typeof libraryCatalog !== "undefined" ? libraryCatalog : null
    onBrowseFolderChanged: refreshCatalog()

    function handleOverlayWindowClosing(closeRequest) {
        closeRequest.accepted = false
        window.hideOverlayAndRestoreControls()
    }

    Window {
        id: djOverlay
        objectName: "djOverlay"
        title: "Beatglass overlay"
        onClosing: function(close) { window.handleOverlayWindowClosing(close) }
        visible: false
        width: 310; height: 130
        minimumWidth: 230; minimumHeight: 100
        color: "transparent"
        Rectangle {
            id: overlaySurface
            anchors.fill: parent; radius: 12
            color: window.themeBackgrounds[window.selectedThemeIndex]
            property color pulseColor: (window.selectedTheme === "Light" || window.selectedTheme === "Yellow")
                ? Qt.darker(window.themeBackgrounds[window.selectedThemeIndex], 1.35)
                : Qt.lighter(window.themeBackgrounds[window.selectedThemeIndex], 1.35)
            border.color: pulseOpacity > 0 ? pulseColor : window.themeAccentColors[window.selectedThemeIndex]
            property real pulseOpacity: 0.0
            Accessible.name: "Overlay beat pulse"
            Accessible.role: Accessible.Status
            Accessible.value: pulseOpacity > 0 ? "active" : "idle"
            Accessible.description: "Beat-synchronized visual metronome; " + (window.liveBridge.status === "Tracking" && isFinite(Number(window.liveBridge.tempo)) && Number(window.liveBridge.tempo) > 0 ? "tracking at " + window.liveBridge.tempo + " BPM" : "waiting for valid live BPM") + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "; reduced motion disables pulsing" : ".")
            border.width: 1 + pulseOpacity * 3
            Behavior on pulseOpacity { enabled: !window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion; NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
            Behavior on border.width { enabled: !window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion; NumberAnimation { duration: 100 } }
            Timer {
                id: beatPulse
                objectName: "beatPulse"
                repeat: true
                interval: { var bpm = Number(window.liveBridge.tempo); return isFinite(bpm) && bpm > 0 ? Math.max(120, 60000 / bpm) : 1000 }
                running: djOverlay.visible && (!window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion) && !window.overlayCaptureEvidenceStale && window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" && window.liveBridge.status === "Tracking" && isFinite(Number(window.liveBridge.tempo)) && Number(window.liveBridge.tempo) > 0
                onTriggered: { overlaySurface.pulseOpacity = 1.0; pulseFade.restart() }
            }
            Timer { id: pulseFade; interval: Math.min(180, Math.max(80, beatPulse.interval * 0.2)); onTriggered: overlaySurface.pulseOpacity = 0.0 }
            Connections {
                target: window.themePreferencesBridge
                function onReduceMotionChanged() {
                    if (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion) {
                        beatPulse.stop()
                        pulseFade.stop()
                        overlaySurface.pulseOpacity = 0.0
                        window.overlayPulseAnnouncement = "Beat pulse disabled because reduced motion is enabled."
                        window.themePreferenceAnnouncement = "Reduced motion enabled for " + window.selectedTheme + "; overlay beat pulse disabled."
                    } else if (djOverlay.visible && window.liveBridge.status === "Tracking" && !window.overlayCaptureEvidenceStale && window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" && isFinite(Number(window.liveBridge.tempo)) && Number(window.liveBridge.tempo) > 0) {
                        beatPulse.restart()
                        window.overlayPulseAnnouncement = "Beat pulse restored for fresh Monitoring capture."
                        window.themePreferenceAnnouncement = "Reduced motion disabled for " + window.selectedTheme + "; overlay beat pulse restored."
                    }
                }
            }
            Connections {
                target: window.liveBridge
                function onChanged() {
                    const bpm = Number(window.liveBridge.tempo)
                    const tracking = window.liveBridge.status === "Tracking" && !window.overlayCaptureEvidenceStale && window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" && isFinite(bpm) && bpm > 0
                    if (tracking && (!window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion))
                        beatPulse.restart()
                    else {
                        beatPulse.stop()
                        pulseFade.stop()
                        overlaySurface.pulseOpacity = 0.0
                        window.overlayPulseAnnouncement = "Beat pulse paused until fresh Monitoring capture and valid BPM are available."
                    }
                }
            }
            Connections {
                target: window.captureMonitorBridge
                function onChanged() {
                    const bpm = Number(window.liveBridge.tempo)
                    const tracking = window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" && window.liveBridge.status === "Tracking" && !window.overlayCaptureEvidenceStale && isFinite(bpm) && bpm > 0
                    if (tracking && (!window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion))
                        beatPulse.restart()
                    else {
                        beatPulse.stop()
                        pulseFade.stop()
                        overlaySurface.pulseOpacity = 0.0
                        window.overlayPulseAnnouncement = "Beat pulse paused while capture monitoring is unavailable or reacquiring."
                    }
                }
            }
            Connections {
                target: djOverlay
                function onVisibleChanged() {
                    if (!djOverlay.visible) {
                        beatPulse.stop()
                        pulseFade.stop()
                        overlaySurface.pulseOpacity = 0.0
                        window.overlayPulseAnnouncement = "Beat pulse cleared while the overlay is hidden."
                    } else if (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion) {
                        window.overlayPulseAnnouncement = "Overlay restored; beat pulse remains disabled because reduced motion is enabled."
                    } else if (window.overlayCaptureEvidenceStale || !window.captureMonitorBridge || window.captureMonitorBridge.status !== "Monitoring") {
                        window.overlayPulseAnnouncement = "Overlay restored; beat pulse is waiting for fresh Monitoring capture."
                    } else if (window.liveBridge.status !== "Tracking" || !isFinite(Number(window.liveBridge.tempo)) || Number(window.liveBridge.tempo) <= 0) {
                        window.overlayPulseAnnouncement = "Overlay restored; beat pulse is waiting for valid live BPM."
                    } else {
                        window.overlayPulseAnnouncement = "Overlay restored; beat pulse is ready for fresh live evidence."
                    }
                }
            }
            Connections {
                target: overlayPulseStatus
                function onActiveFocusChanged() {
                    if (overlayPulseStatus.activeFocus)
                        window.overlayPulseAnnouncement = "Beat pulse status focused; " + (beatPulse.running ? "pulse is active." : "pulse is idle.")
                }
            }
            MouseArea {
                anchors.fill: parent
                property bool fallbackMove: false
                property point lastGlobal: Qt.point(0, 0)
                onPressed: function(mouse) {
                    fallbackMove = typeof djOverlay.startSystemMove !== "function"
                    if (fallbackMove) lastGlobal = mapToGlobal(mouse.x, mouse.y)
                    else djOverlay.startSystemMove()
                }
                onPositionChanged: function(mouse) {
                    if (!pressed || !fallbackMove) return
                    const current = mapToGlobal(mouse.x, mouse.y)
                    djOverlay.x += current.x - lastGlobal.x
                    djOverlay.y += current.y - lastGlobal.y
                    lastGlobal = current
                }
            }
            ColumnLayout {
                objectName: "overlayLiveEvidenceGroup"
                anchors.fill: parent; anchors.margins: 14; spacing: 4
                Accessible.name: "Overlay live evidence; Ctrl+Alt+Shift+B focuses beat pulse status"
                Accessible.value: (window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable")
                    + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : "")
                    + "; beat pulse " + (beatPulse.running ? "active" : "idle")
                    + "; combined output may be ambiguous"
                    + "; Ctrl+Alt+Shift+B focuses beat pulse status"
                Accessible.role: Accessible.Grouping
                Accessible.description: "Capture-only DJ overlay: current live status, tempo, harmony, beat-pulse period, capture state, and recovery action. Live tempo, key, and chord estimates are provisional evidence, not calibrated certainty. Track and deck metadata are unavailable without an explicitly configured DJ-app integration. The selected output may also include system notifications and other routed audio; selecting one endpoint does not capture every device or bypassed audio path. Combined output may contain overlapping decks, so tempo, key, and chord evidence can be ambiguous. Ctrl+Alt+Shift+B focuses the beat pulse status." + (window.overlayCaptureEvidenceStale ? " Previously displayed evidence is stale until fresh capture arrives." : "")
                ToolTip.text: "Overlay live evidence and capture recovery"
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Beatglass · " + window.liveBridge.status; color: window.themeForegrounds[window.selectedThemeIndex]; Layout.fillWidth: true; elide: Text.ElideRight; Accessible.name: "Beatglass status: " + text; Accessible.role: Accessible.StaticText; Accessible.description: text }
                    Button {
                        text: "×"; objectName: "overlayWindowHide"; Accessible.name: "Hide overlay"; Accessible.role: Accessible.Button; Accessible.description: "Hide the overlay window and restore the control window"; ToolTip.text: "Hide the overlay and restore the control window"
                        visible: window.overlayBridge && !window.overlayBridge.clickThrough
                        onClicked: window.hideOverlayAndRestoreControls()
                    }
                }
                Label { text: "Live evidence stale until fresh capture arrives"; objectName: "overlayStaleEvidenceBanner"; visible: window.overlayCaptureEvidenceStale; color: window.themeForegrounds[window.selectedThemeIndex]; font.bold: true; Accessible.name: "Live evidence stale"; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: "Previously displayed tempo, key, and chord values are stale until fresh capture arrives."; ToolTip.text: "Previously displayed live estimates are stale until fresh capture arrives"; Layout.fillWidth: true }
                Label { text: "Capture-only · " + (window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable") + " · combined output; track/deck metadata unavailable"; objectName: "overlayCaptureScope"; color: window.themeForegrounds[window.selectedThemeIndex]; wrapMode: Text.WordWrap; Accessible.name: "Capture-only overlay scope"; Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : ""); Accessible.role: Accessible.Status; Accessible.description: "This overlay analyzes the selected output mix; capture lifecycle is " + (window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable") + ". It does not provide track or deck metadata without an explicitly configured DJ-app integration."; ToolTip.text: "The overlay analyzes selected output audio; current capture state: " + (window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable"); Layout.fillWidth: true }
                Label {
                    objectName: "overlayEscapeHint"
                    visible: window.overlayBridge && window.overlayBridge.clickThrough
                    text: "Press Escape to unlock overlay controls"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    Accessible.name: "Overlay recovery hint"
                    Accessible.value: text + (window.overlayBridge !== null ? "; retained-settings and summary review available" : "; retained-settings and summary review unavailable")
                    Accessible.role: Accessible.StaticText
                    ToolTip.text: "Press Escape to restore overlay keyboard and pointer controls"
                    Accessible.description: text
                }
                Label { text: "BPM " + window.liveBridge.tempo; objectName: "overlayTempo"; color: window.themeForegrounds[window.selectedThemeIndex]; font.pixelSize: 24; font.bold: true; Accessible.name: "Overlay tempo: " + text; Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready"); Accessible.role: Accessible.Status; Accessible.description: text + ". " + (window.overlayCaptureEvidenceStale ? "This estimate is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready."); ToolTip.text: "Current live tempo estimate; " + (window.overlayCaptureEvidenceStale ? "stale until fresh capture" : window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready") }
                    Label { text: "Beat pulse " + (beatPulse.running ? "active · " + beatPulse.interval + " ms · " + ((window.selectedTheme === "Light" || window.selectedTheme === "Yellow") ? "darkening" : "lightening") : (window.overlayCaptureEvidenceStale ? "idle · stale" : "idle") + " · " + (window.captureMonitorBridge ? window.captureMonitorBridge.status : "capture unavailable") + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? " · reduced" : "")); objectName: "overlayPulseStatus"; color: window.themeForegrounds[window.selectedThemeIndex]; activeFocusOnTab: true; padding: 3; background: Rectangle { color: "transparent"; border.width: overlayPulseStatus.activeFocus ? 2 : 0; border.color: window.themeAccentColors[window.selectedThemeIndex]; radius: 3 } Accessible.name: "Overlay beat pulse status"; Accessible.value: text + "; capture " + (window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable") + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "; reduced motion" : "") + (window.overlayCaptureEvidenceStale ? "; evidence stale" : "") + "; announcement: " + window.overlayPulseAnnouncement; Accessible.role: Accessible.Status; Accessible.focusable: true; Accessible.description: "Beat-synchronized visual metronome status; " + window.overlayPulseAnnouncement + " " + (beatPulse.running ? "Active for valid live BPM with a " + beatPulse.interval + " millisecond period and " + ((window.selectedTheme === "Light" || window.selectedTheme === "Yellow") ? "darkening" : "lightening") + " contrast" : "Idle until a Monitoring capture bridge, valid live BPM, and full motion are available") + ". Keyboard shortcut Ctrl+Alt+Shift+B focuses this status."; ToolTip.text: "Beat-synchronized visual pulse status; Ctrl+Alt+Shift+B focuses this status" }
                Label { text: "Key " + window.liveBridge.key + "    Chord " + window.liveBridge.chord; objectName: "overlayHarmony"; color: window.themeForegrounds[window.selectedThemeIndex]; Accessible.name: "Overlay harmony: " + text; Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready"); Accessible.role: Accessible.Status; Accessible.description: text + ". " + (window.overlayCaptureEvidenceStale ? "This estimate is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready."); ToolTip.text: "Current live key and chord estimate; " + (window.overlayCaptureEvidenceStale ? "stale until fresh capture" : window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready") }
                Label { text: window.captureMonitorBridge ? (window.captureMonitorBridge.status === "Error" ? "Capture error — retry monitoring" : "Capture " + window.captureMonitorBridge.status) : "Capture unavailable"; color: window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "#fca5a5" : window.themeForegrounds[window.selectedThemeIndex]; objectName: "overlayCaptureStatus"; Accessible.name: "Overlay capture status: " + text; Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready"); Accessible.role: Accessible.Status; Accessible.description: text + ". " + (window.overlayCaptureEvidenceStale ? "Previously displayed evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready."); ToolTip.text: "Current output capture status; " + (window.overlayCaptureEvidenceStale ? "evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready") }
                Button {
                    objectName: "overlayRetryCapture"
                    visible: window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" && window.captureMonitorBridge.start
                    text: "Retry capture"
                    ToolTip.text: "Retry monitoring the selected output source; " + (window.overlayCaptureEvidenceStale ? "previous evidence is stale until fresh capture" : window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Accessible.name: "Retry overlay capture"
                    Accessible.value: window.overlayCaptureEvidenceStale ? "available; previous evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "available; overlay recovery pending" : "available; overlay ready"
                    Accessible.role: Accessible.Button
                    Accessible.description: "Retry monitoring the selected output source; " + (window.overlayCaptureEvidenceStale ? "previous evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                    onClicked: window.captureMonitorBridge.start()
                }
            }
        }
    }

    function playSelectedLibraryTrack() {
        if (!window.playbackBridge || window.selectedAudioUrl.toString() === "") {
            window.announceCaptureRecoveryState("Select an audio source before starting playback.")
            return false
        }
        const accepted = window.playbackBridge.openSource(window.selectedAudioUrl, window.playbackBridge.epoch + 1)
        if (accepted)
            window.clearVisualizationForCaptureState("Selected source is playing; previous visualization evidence is stale until fresh accepted analysis.")
        window.announceCaptureRecoveryState(accepted
            ? "Playing selected Library source " + window.selectedName + "."
            : "Selected Library source could not be opened for playback.")
        return accepted
    }

    function reopenSelectedLibraryTrack() {
        if (!window.playbackBridge || window.selectedAudioUrl.toString() === "") {
            window.announceCaptureRecoveryState("Select an audio source before reopening playback.")
            return false
        }
        if (window.playbackMatchesSelection) {
            window.announceCaptureRecoveryState("Reopen selected playback is no longer needed; the playback source already matches the selection.")
            return false
        }
        const accepted = window.playbackBridge.openSource(window.selectedAudioUrl, window.playbackBridge.epoch + 1)
        if (accepted)
            window.clearVisualizationForCaptureState("Selected source reopened; previous visualization evidence is stale until fresh accepted analysis.")
        window.announceCaptureRecoveryState(accepted
            ? "Selected Library source reopened for playback."
            : "Selected Library source could not be reopened for playback.")
        return accepted
    }

    function activateEntry(index) {
        if (index < 0 || index >= files.count) return
        const url = files.get(index, "fileUrl")
        if (files.isFolder(index)) {
            window.clearSelectedAudio()
            window.recursiveCatalogActive = false
            window.recursiveCatalogRows = []
            browseFolder = url
            if (typeof libraryPreferences !== "undefined") libraryPreferences.folder = url.toString()
            browser.currentIndex = -1
            search.text = ""
            window.announceVisualizationRecovery("Opened Library folder " + files.get(index, "fileName") + ".")
        } else {
            window.clearVisualizationForCaptureState("Selection cleared because the audio source changed; the overlay awaits fresh accepted evidence.")
            selectedName = files.get(index, "fileName")
            selectedAudioUrl = url
            metadataRetryPending = false
            metadataBridge.inspect(url)
            window.announceVisualizationRecovery("Selected source changed; prior analysis presentation is stale until this source is analyzed.")
            analyzeSelectedAudio.forceActiveFocus()
        }
    }
    function clearSelectedAudio() {
        window.selectedName = ""
        window.selectedAudioUrl = ""
        window.metadataRetryPending = false
        window.metadataStatus = "Choose an audio file to inspect its metadata."
        window.clearVisualizationForCaptureState("Library selection cleared; choose another source for fresh analysis.")
        window.announceVisualizationRecovery("Library selection cleared; choose another source for fresh analysis.")
        browser.forceActiveFocus()
    }
    function clearVisualizationForCaptureState(reason) {
        window.visualizationSelectionResetReason = reason !== undefined ? reason : (window.liveBridge.status === "Error"
            ? "Selection cleared after a capture error."
            : window.liveBridge.status === "Reacquiring"
            ? "Selection cleared while capture is reacquiring."
            : window.liveBridge.status === "Starting"
            ? "Selection cleared while capture is starting."
            : "Selection cleared because capture stopped.")
        window.visualizationRecoveryAnnouncement = window.visualizationSelectionResetReason
        if (typeof visualizationCanvas !== "undefined") {
            visualizationCanvas.selectedPosition = -1
            visualizationCanvas.selectedSpectrumBin = -1
            visualizationCanvas.renderedRevision = -1
        visualizationCanvas.acceptedPlaybackEpoch = -1
        visualizationCanvas.acceptedPlaybackSource = ""
            visualizationCanvas.renderedWaveform = []
            visualizationCanvas.renderedSpectrum = []
            visualizationCanvas.requestPaint()
            visualizationCanvas.forceActiveFocus()
        }
        if (typeof visualizationPresentation !== "undefined"
                && visualizationPresentation.clear)
            visualizationPresentation.clear()
    }
    function announceVisualizationRecovery(message) {
        window.visualizationRecoveryAnnouncement = message
        if (typeof visualizationCanvas !== "undefined")
            visualizationCanvas.forceActiveFocus()
    }
    function announceCaptureRecoveryState(message) {
        window.clearVisualizationForCaptureState(message)
        window.announceVisualizationRecovery(message)
    }
    function retryVisualizationAnalysis() {
        window.overlayPersistedDiagnosticDismissed = true
        window.activeWorkerOutcomeAnnouncementKey = ""
        if (window.analysisJobBridge && window.analysisJobBridge.retry !== undefined && window.analysisJobBridge.canRetry) {
            window.analysisJobBridge.retry()
            window.clearVisualizationForCaptureState("Analysis retry queued; previous visualization evidence is stale until completion.")
            window.announceVisualizationRecovery("Analysis retry queued; focus will return after a fresh accepted result.")
            return true
        }
        if (typeof storageRecovery !== "undefined" && storageRecovery.retryAnalysis !== undefined && storageRecovery.analysisRetryable) {
            storageRecovery.retryAnalysis()
            window.clearVisualizationForCaptureState("Analysis retry admitted; previous visualization evidence is stale until completion.")
            window.announceVisualizationRecovery("Analysis retry admitted; focus will return after a fresh accepted result.")
            return true
        }
        window.announceVisualizationRecovery("Analysis retry is no longer available; the worker state changed before admission.")
        if (typeof retryAnalysis !== "undefined")
            retryAnalysis.forceActiveFocus()
        return false
    }
    Connections {
        target: window.analysisJobBridge
        function onStatusChanged() {
            const status = window.analysisJobBridge.status
            if (status === "Complete" && window.libraryAnalysisResultReady) {
                window.activeWorkerOutcomeAnnouncementKey = ""
                window.announceVisualizationRecovery("Fresh analysis result accepted; visualization recovery focus restored.")
            } else if (status !== "Failed" && status !== "Cancelled") {
                window.activeWorkerOutcomeAnnouncementKey = ""
            } else if (window.activeWorkerOutcomeAnnouncementKey === status) {
                return
            } else if (window.analysisJobBridge.canRetry) {
                window.activeWorkerOutcomeAnnouncementKey = status
                window.announceVisualizationRecovery("Analysis " + status.toLowerCase() + "; Retry analysis is available.")
                retryAnalysis.forceActiveFocus()
            }
        }
        function onCanRetryChanged() {
            if (!window.analysisJobBridge.canRetry)
                window.activeWorkerOutcomeAnnouncementKey = ""
        }
    }
    function clearVisualizationPositionAndFocus() {
        visualizationCanvas.selectedPosition = -1
        visualizationCanvas.selectedSpectrumBin = -1
        visualizationCanvas.requestPaint()
        visualizationCanvas.forceActiveFocus()
    }
    function visualizationPositionToFrame(position, durationFrames) {
        if (!isFinite(position) || !isFinite(durationFrames) || durationFrames < 0)
            return -1
        const clamped = Math.max(0, Math.min(1, position))
        return Math.min(durationFrames, Math.floor(clamped * durationFrames))
    }
    function seekPlaybackToVisualizationPosition(position, durationFrames) {
        const frame = visualizationPositionToFrame(position, durationFrames)
        if (frame < 0 || visualizationCanvas.acceptedResultIdentity === ""
                || !visualizationCanvas.presentationReady
                || !window.playbackBridge || !window.playbackMatchesSelection
                || window.playbackBridge.state === "Stopped")
            return false
        return window.playbackBridge.seek(frame, window.playbackBridge.epoch + 1)
    }

    function stopPlaybackWithRecovery(message) {
        if (!window.playbackBridge || window.playbackBridge.state === "Stopped")
            return false
        window.playbackBridge.stop()
        window.announceVisualizationRecovery(message)
        return true
    }

    function togglePlaybackPause() {
        if (!window.playbackBridge)
            return false
        const paused = window.playbackBridge.state === "Paused"
        const changed = paused ? window.playbackBridge.resume() : window.playbackBridge.pause()
        window.announceCaptureRecoveryState(changed
            ? (paused ? "Playback resumed." : "Playback paused.")
            : "Playback pause or resume failed; state was unchanged.")
        return changed
    }

    function refreshPlaybackOutputs() {
        if (!window.playbackBridge)
            return false
        window.playbackBridge.refreshDevices()
        window.announceCaptureRecoveryState("Playback outputs refreshed; choose an available device before starting native playback.")
        return true
    }

    function playbackDeviceIdAt(index, label) {
        if (index < 0 || !label)
            return -1
        const separator = label.indexOf(":")
        const parsed = separator > 0 ? Number(label.slice(0, separator)) : Number.NaN
        return isFinite(parsed) ? parsed : index
    }

    function startNativePlaybackOutput() {
        if (!window.playbackBridge)
            return false
        const started = window.playbackBridge.startNative(window.playbackBridge.epoch + 1, playbackSampleRate.value, playbackChannels.value, 4096)
        window.announceCaptureRecoveryState(started
            ? "Native playback started on the selected output device."
            : "Native playback could not start; check the selected output device and format.")
        return started
    }

    function reopenSelectedPlayback() {
        if (!window.playbackBridge || !window.selectedAudioUrl || window.selectedAudioUrl.toString() === "") {
            window.announceVisualizationRecovery("Select a Library track before reopening playback.")
            return false
        }
        const nextEpoch = window.playbackBridge.epoch + 1
        const reopened = window.playbackBridge.state === "Stopped"
            ? window.playbackBridge.openSource(window.selectedAudioUrl, nextEpoch)
            : window.playbackBridge.replaceSource(window.selectedAudioUrl, nextEpoch)
        if (reopened)
            window.clearVisualizationForCaptureState("Playback source reopened; previous visualization evidence is stale until fresh accepted analysis.")
        window.announceVisualizationRecovery(reopened
            ? "Selected Library track reopened; visualization selection was reset."
            : "Selected Library track could not be reopened; playback state was unchanged.")
        return reopened
    }

    function retryNativePlaybackOutput() {
        if (!window.playbackBridge) {
            window.announceCaptureRecoveryState("Native playback is unavailable in this build.")
            return false
        }
        const retried = window.playbackBridge.retryNative()
        window.announceCaptureRecoveryState(retried
            ? "Native playback output recovered on the selected device."
            : "Native playback output retry failed; check the selected device and format.")
        return retried
    }
    function syncVisualizationSelectionFromPlayback(frame, durationFrames) {
        if (!isFinite(frame) || !isFinite(durationFrames) || durationFrames <= 0
                || visualizationCanvas.acceptedResultIdentity === ""
                || !visualizationCanvas.presentationReady)
            return false
        visualizationCanvas.selectedPosition = Math.max(0, Math.min(1, frame / durationFrames))
        visualizationCanvas.requestPaint()
        return true
    }
    Connections {
        target: window.liveBridge
        function onStatusChanged() {
            if (window.liveBridge.status === "Stopped" || window.liveBridge.status === "Error"
                    || window.liveBridge.status === "Starting"
                    || window.liveBridge.status === "Reacquiring")
                window.clearVisualizationForCaptureState()
            if (window.liveBridge.status === "Stopped" || window.liveBridge.status === "Error"
                    || window.liveBridge.status === "Starting" || window.liveBridge.status === "Reacquiring")
                window.overlayCaptureEvidenceStale = true
            if (window.liveBridge.status === "Stopped")
                window.announceVisualizationRecovery("Output monitoring stopped; start monitoring from the DJ controls when an output endpoint is available.")
            else if (window.liveBridge.status === "Starting")
                window.announceVisualizationRecovery("Output monitoring is starting; waiting for the first accepted visualization snapshot.")
            else if (window.liveBridge.status === "Reacquiring")
                window.announceVisualizationRecovery("Output monitoring is reacquiring; previous visualization selection was cleared.")
            else if (window.liveBridge.status === "Error")
                window.announceVisualizationRecovery("Output monitoring reported an error; retry monitoring after checking the selected output and permission state.")
            else if (window.liveBridge.status === "Tracking") {
                if (window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring"
                        && window.overlayCaptureEvidenceStale)
                    window.transitionCaptureEvidence(false, "Overlay live analysis recovered; fresh accepted evidence is available.", overlayLiveMonitoringState)
                window.announceVisualizationRecovery("Output monitoring is tracking; visualization updates will appear as accepted data arrives.")
            }
        }
    }
    Connections {
        target: window.playbackBridge
        function onSourceChanged() { window.clearVisualizationForCaptureState("Selection cleared because the playback source changed.") }
        function onEpochChanged() { window.clearVisualizationForCaptureState("Selection cleared because the playback epoch changed.") }
        function onStateChanged() {
            if (window.playbackBridge.state === "Stopped")
                window.clearVisualizationForCaptureState("Playback stopped; accepted visualization snapshot cleared.")
        }
        function onChanged() {
            if (visualizationCanvas.acceptedPlaybackSource !== ""
                    && window.playbackBridge.source.toString() !== visualizationCanvas.acceptedPlaybackSource) {
                window.clearVisualizationForCaptureState("Selection cleared because the playback source changed.")
                return
            }
            if (visualizationCanvas.acceptedPlaybackEpoch >= 0
                    && window.playbackBridge.epoch !== visualizationCanvas.acceptedPlaybackEpoch) {
                window.clearVisualizationForCaptureState("Selection cleared because the playback epoch changed.")
                return
            }
            if (window.playbackBridge.state !== "Stopped"
                    && typeof visualizationPresentation !== "undefined"
                    && visualizationCanvas.renderedWaveform.length > 0)
                window.syncVisualizationSelectionFromPlayback(window.playbackBridge.currentFrame, visualizationCanvas.renderedWaveform.length)
        }
    }
    Connections {
        target: window.endpointSelectionBridge
        function onChanged() { window.handleOverlayEndpointChanged() }
    }
    function handleOverlayEndpointChanged() {
        window.overlayCaptureEvidenceStale = true
        window.overlayDisplayRecoveryPending = true
        window.clearVisualizationForCaptureState("Selection cleared because the capture endpoint changed.")
        if (window.endpointSelectionBridge.status.indexOf("Error") >= 0)
            window.announceVisualizationRecovery("Capture endpoint retry failed; check permission and reacquire a device. Overlay evidence is stale until fresh capture arrives.")
        else if (window.endpointSelectionBridge.endpoints.length === 0)
            window.announceVisualizationRecovery("No capture endpoint is available; connect a device and refresh. Overlay evidence is stale until fresh capture arrives.")
        else
            window.announceVisualizationRecovery("Capture endpoint changed; waiting for a fresh accepted snapshot. Overlay evidence is stale until fresh capture arrives.")
        endpointSelector.forceActiveFocus()
    }
    Connections {
        target: window.captureMonitorBridge
        function onChanged() { window.handleCaptureMonitorStatusChanged() }
    }
    function handleCaptureMonitorStatusChanged() {
        if (!window.captureMonitorBridge)
            return
        if (window.captureMonitorBridge.status === "Error") {
            window.transitionCaptureEvidence(true, "No output device or permission; retry monitoring to acquire a fresh snapshot.", overlayCaptureRecoveryAction)
        } else if (window.captureMonitorBridge.status === "Starting") {
            window.transitionCaptureEvidence(true, "Overlay capture starting; live evidence remains stale until fresh audio arrives.", overlayLiveMonitoringState)
        } else if (window.captureMonitorBridge.status === "Reacquiring") {
            window.transitionCaptureEvidence(true, "Output device reacquiring; visualization focus restored when fresh audio arrives.", overlayLiveMonitoringState)
        } else if (window.captureMonitorBridge.status === "Stopped") {
            window.transitionCaptureEvidence(true, "Output monitoring stopped; previously displayed live evidence is stale until monitoring resumes.", overlayLiveMonitoringState)
        } else if (window.captureMonitorBridge.status === "Monitoring" && window.overlayCaptureEvidenceStale && !window.overlayDisplayRecoveryPending && !window.overlayHiddenByUser && !window.overlayRecoveryPausedByUser && window.overlayRecoveryAttemptCount < window.overlayRecoveryAttemptLimit && !window.overlayLaunchFallbackActive && !window.overlayVisualStateRecoveryRequired && !window.overlayWasVisibleBeforeAvailabilityLoss) {
            window.transitionCaptureEvidence(false, "Overlay capture recovered; fresh accepted snapshot is available.", overlayLiveMonitoringState)
        }
    }
    Connections {
        target: window.overlayBridge
        function onAvailableChanged() {
            if (window.overlayBridge && !window.overlayBridge.available) {
                window.overlayDisplayRecoveryPending = true
                window.overlayCaptureEvidenceStale = true
                clickThroughToggle.checked = false
                window.focusOverlayAvailabilityRecovery("DJ overlay unavailable; output monitoring remains unavailable until the display recovers.", retryOverlay)
            } else if (window.overlayBridge) {
                window.overlayRecoveryAttemptCount = 0
                window.overlayRecoveryPausedByUser = false
                clickThroughToggle.checked = window.overlayBridge.clickThrough
                window.syncOverlaySettingsStateFromBridge()
                if (window.overlayBridge.screenNames.length === 0) {
                    window.focusOverlayNoDisplayRecovery()
                } else if (window.overlayDisplaySelectionRequired) {
                    window.focusOverlayAvailabilityRecovery("DJ overlay recovered; choose a display to re-enable overlay actions.", overlayDisplay)
                } else {
                    window.overlayDisplayRecoveryPending = false
                    window.focusOverlayRestoredState("DJ overlay recovered; Show, Restore, and Reapply actions are available.", overlaySettingsRetentionReviewAction)
                }
            }
        }
    function focusOverlayNoDisplayRecovery() {
        window.overlayDisplayRecoveryPending = true
        window.overlayDisplaySelectionRequired = true
        window.overlayCaptureEvidenceStale = true
        return window.focusOverlayAvailabilityRecovery("DJ overlay recovered without a display; overlay actions remain disabled until a display is selected.", overlayDisplay)
    }
        function onScreensChanged() {
            if (window.overlayBridge && window.overlayBridge.displayName !== "")
                window.overlayPreferredDisplayName = window.overlayBridge.displayName
            if (!window.overlayBridge || window.overlayBridge.screenNames.length === 0)
                window.focusOverlayDisplayLoss()
            else {
                const savedNameIndex = window.overlayPreferredDisplayName !== ""
                    ? window.overlayBridge.screenNames.indexOf(window.overlayPreferredDisplayName) : -1
                const savedIndex = savedNameIndex >= 0 ? savedNameIndex : window.overlayPreferredScreenIndex
                if (savedIndex >= 0 && savedIndex < window.overlayBridge.screenNames.length) {
                    window.overlayRecoveryAttemptCount = 0
                    window.overlayPreferredScreenIndex = savedIndex
                    window.overlayDisplaySelectionRequired = false
                    if (!window.overlayRecoveryPausedByUser)
                        window.recoverOverlayAfterDisplayChange()
                    else {
                        window.overlayDisplayRecoveryPending = false
                        window.focusOverlayAvailabilityRecovery("DJ overlay display restored; recovery remains paused until Resume recovery is chosen.", resumeOverlayRecovery)
                    }
                    if (!window.overlayRecoveryPausedByUser)
                        retryOverlay.forceActiveFocus()
                } else {
                    window.focusOverlayDisplaySelectionRequired()
                }
            }
        }
        function onClickThroughChanged() {
            window.handleOverlayInteractionStateChanged()
        }
        function onOpacityChanged() {
            window.handleOverlayOpacityChanged()
        }
        function onGeometryChanged() {
            if (!window.syncOverlaySettingsStateFromBridge())
                window.focusOverlayGeometrySyncFailure()
            if (window.overlayDisplayRecoveryPending && window.overlayBridge
                    && window.overlayBridge.screenNames.length > 0
                    && !window.overlayDisplaySelectionRequired) {
                window.handleOverlayGeometryRecovery()
            }
            window.announceOverlayGeometryState()
        }
    }
    function handleOverlayInteractionStateChanged() {
        window.overlaySettingsRevision += 1
        if (!window.overlayBridge)
            return
        window.overlayCaptureEvidenceStale = true
        window.announceCaptureRecoveryState(window.overlayBridge.clickThrough
            ? "DJ overlay is click-through; unlock it to restore keyboard and pointer interaction. Live evidence is stale until fresh capture arrives."
            : "DJ overlay interaction restored; keyboard and pointer input are available. Live evidence remains stale until fresh capture arrives.")
        if (window.overlayBridge.clickThrough)
            unlockOverlay.forceActiveFocus()
        else
            overlayInteractionMode.forceActiveFocus()
    }
    function handleOverlayOpacityChanged() {
        if (!window.overlayBridge)
            return
        if (window.overlayOpacityEditInFlight
                && Math.abs(Number(window.overlayBridge.opacity) - window.overlayPendingOpacity) < 0.001) {
            window.overlayOpacityEditInFlight = false
            window.overlayPendingOpacity = -1
        } else if (!window.overlayOpacityEditInFlight) {
            window.overlaySettingsRevision += 1
            overlayOpacityControl.forceActiveFocus()
        }
        window.overlayCaptureEvidenceStale = true
        window.announceCaptureRecoveryState("DJ overlay opacity set to " + Math.round(window.overlayBridge.opacity * 100) + " percent. Live evidence remains stale until fresh capture arrives.")
    }
    function handleOverlaySizeChangeResult(resized, nextIndex) {
        if (resized) {
            window.overlayPreferredSizeIndex = nextIndex
            window.overlaySettingsRevision += 1
            window.announceCaptureRecoveryState("DJ overlay size set to " + ["Compact", "Medium", "Large"][nextIndex] + ".")
        } else {
            window.overlayCaptureEvidenceStale = true
            window.syncOverlaySettingsStateFromBridge()
            window.announceCaptureRecoveryState("DJ overlay size could not be changed; persisted size remains active. Live evidence remains stale until fresh capture arrives.")
        }
        overlaySize.forceActiveFocus()
    }
    function handleOverlayGeometryRecovery() {
        window.overlayCaptureEvidenceStale = true
        if (!window.overlayRecoveryPausedByUser)
            window.recoverOverlayAfterDisplayChange()
        else
            window.announceCaptureRecoveryState("DJ overlay geometry restored; recovery remains paused until Resume recovery is chosen. Live evidence remains stale until fresh capture arrives.")
        if (window.overlayRecoveryPausedByUser)
            resumeOverlayRecovery.forceActiveFocus()
        else
            retryOverlay.forceActiveFocus()
    }
    function focusOverlayDisplayLoss() {
        window.overlayDisplayRecoveryPending = true
        window.overlayDisplaySelectionRequired = true
        window.overlayCaptureEvidenceStale = true
        return window.focusOverlayAvailabilityRecovery("DJ overlay display disappeared; overlay actions disabled and controls restored until a display is available.", overlayDisplay)
    }
    function announceOverlayGeometryState() {
        const geometry = window.overlayBridge ? window.overlayBridge.geometry : null
        if (!geometry || !isFinite(geometry.x) || !isFinite(geometry.y)
                || !isFinite(geometry.width) || !isFinite(geometry.height)
                || geometry.width <= 0 || geometry.height <= 0) {
            window.focusOverlayGeometrySyncFailure()
            return false
        }
        window.overlayCaptureEvidenceStale = true
        window.announceCaptureRecoveryState("DJ overlay geometry restored at " + geometry.width + " by " + geometry.height + " pixels. Live evidence remains stale until fresh capture arrives.")
        return true
    }
    function focusOverlayGeometrySyncFailure() {
        window.overlayDisplayRecoveryPending = true
        window.overlayDisplaySelectionRequired = true
        window.overlayCaptureEvidenceStale = true
        return window.focusOverlayAvailabilityRecovery("Overlay geometry changed, but retained settings could not be synchronized; choose a display before retrying.", overlayDisplay)
    }
    function focusOverlayDisplaySelectionRequired() {
        window.overlayDisplaySelectionRequired = true
        window.overlayDisplayRecoveryPending = true
        window.overlayCaptureEvidenceStale = true
        return window.focusOverlayAvailabilityRecovery("DJ overlay display restored; choose a display to re-enable overlay actions and resume monitoring.", overlayDisplay)
    }
    Connections {
        target: typeof storageRecovery !== "undefined" ? storageRecovery : null
        function onAnalysisAdmissionChanged() {
            if (storageRecovery.analysisStatus === "Analysis failed" || storageRecovery.analysisStatus === "Analysis cancelled")
                window.overlayPersistedDiagnosticDismissed = false
        }
        function onAnalysisStatusChanged() {
            if (storageRecovery.analysisStatus === "Analysis failed" || storageRecovery.analysisStatus === "Analysis cancelled")
                window.overlayPersistedDiagnosticDismissed = false
        }
    }

    FolderListModel {
        id: files
        objectName: "fileModel"
        folder: window.browseFolder
        showDirs: true; showDirsFirst: true; showDotAndDotDot: false
        nameFilters: ["*" + search.text + "*.wav", "*" + search.text + "*.flac", "*" + search.text + "*.mp3", "*" + search.text + "*.aiff", "*" + search.text + "*.aif"]
        caseSensitive: false
        sortField: FolderListModel.Name
        sortReversed: window.sortDescending
    }
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label { text: "Beatglass"; font.pixelSize: 20; font.bold: true; Layout.leftMargin: 16; Layout.rightMargin: 24; Accessible.name: "Beatglass"; Accessible.role: Accessible.Heading; ToolTip.text: "Beatglass desktop audio workspace" }
            TabBar {
                id: navigation
                objectName: "navigation"
                Layout.fillWidth: true
                Accessible.name: "Primary navigation"
                Accessible.role: Accessible.Toolbar
                Accessible.description: "Switch between Library, DJ, and Settings pages. Use Ctrl+Shift+1 for Library, Ctrl+Shift+2 for DJ, or Ctrl+Shift+3 for Settings."
                ToolTip.text: "Primary workspace navigation; current page: " + (currentIndex === 0 ? "Library" : (currentIndex === 1 ? "DJ" : (currentIndex === 2 ? "Settings" : "none")))
                TabButton { id: libraryTab; text: "Library"; objectName: "libraryTab"; activeFocusOnTab: true; Accessible.role: Accessible.PageTab; ToolTip.text: checked ? "Library is selected; browse local audio files" : "Browse local audio files"; Accessible.name: "Library"; Accessible.description: "Browse local audio files. Shortcut Ctrl+Shift+1; focus returns to this tab."; background: Rectangle { color: libraryTab.checked ? "#273b5d" : "transparent"; border.width: libraryTab.activeFocus ? 3 : 1; border.color: libraryTab.activeFocus ? "#60a5fa" : "#718096"; radius: 5 } }
                TabButton { id: djTab; text: "DJ"; objectName: "djTab"; activeFocusOnTab: true; Accessible.name: "DJ"; Accessible.description: "View live DJ controls and overlay settings. Shortcut Ctrl+Shift+2; focus returns to this tab."; Accessible.role: Accessible.PageTab; ToolTip.text: checked ? "DJ is selected; open live controls and overlay settings" : "Open live DJ controls and overlay settings"; background: Rectangle { color: djTab.checked ? "#273b5d" : "transparent"; border.width: djTab.activeFocus ? 3 : 1; border.color: djTab.activeFocus ? "#60a5fa" : "#718096"; radius: 5 } }
                TabButton { id: settingsTab; text: "Settings"; objectName: "settingsTab"; activeFocusOnTab: true; Accessible.name: "Settings"; Accessible.description: "Configure appearance, audio and overlay settings. Shortcut Ctrl+Shift+3; focus returns to this tab."; Accessible.role: Accessible.PageTab; ToolTip.text: checked ? "Settings is selected; configure audio, appearance, and overlay" : "Open audio, appearance, and overlay settings"; background: Rectangle { color: settingsTab.checked ? "#273b5d" : "transparent"; border.width: settingsTab.activeFocus ? 3 : 1; border.color: settingsTab.activeFocus ? "#60a5fa" : "#718096"; radius: 5 } }
            }
            ComboBox {
                id: themeSelector
                objectName: "themeSelector"
                model: window.themeNames
                currentIndex: window.selectedThemeIndex
                activeFocusOnTab: true
                Accessible.role: Accessible.ComboBox
                Accessible.name: "Color theme"
                Accessible.description: "Choose one of six color themes for the Beatglass desktop; current theme " + currentText
                ToolTip.text: "Choose a color theme; current theme: " + currentText
                onActivated: {
                    window.selectThemeIndex(currentIndex)
                }
            }
            Switch {
                id: reduceMotion
                objectName: "reduceMotion"
                text: "Reduce motion"
                checked: window.themePreferencesBridge ? window.themePreferencesBridge.reduceMotion : false
                activeFocusOnTab: true
                Accessible.name: "Reduce motion"
                Accessible.role: Accessible.CheckBox
                Accessible.description: "Reduce animation and transition effects in the desktop presentation; currently " + (checked ? "enabled" : "disabled")
                ToolTip.text: checked ? "Reduced motion is enabled; disable to restore transitions" : "Enable reduced motion to minimize animation and transitions"
                onClicked: window.setReduceMotionPreference(checked)
            }
            Label {
                objectName: "themePreferenceAnnouncement"
                text: window.themePreferenceAnnouncement
                visible: text !== ""
                Accessible.name: "Theme preference announcement: " + text
                Accessible.role: Accessible.Status
                Accessible.description: "Live appearance and reduced-motion preference state."
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            Button {
                objectName: "resetAppearance"
                text: "Reset appearance"
                visible: !window.compactHeader
                activeFocusOnTab: true
                Accessible.name: "Reset appearance settings"
                Accessible.role: Accessible.Button
                Accessible.description: "Restore the default Light theme and turn off reduced motion. Current settings: " + window.selectedTheme + ", " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced motion" : "full motion") + "."
                ToolTip.text: "Reset " + window.selectedTheme + " and " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced" : "full") + " motion to defaults"
                onClicked: {
                    window.resetAppearancePreferences()
                    themeSelector.currentIndex = 0
                    resetAppearance.forceActiveFocus()
                }
            }
            Row {
                id: themePreviews
                objectName: "themePreviews"
                visible: !window.compactHeader
                spacing: 3
                Accessible.name: "Theme previews"
                Accessible.value: window.selectedTheme
                Accessible.role: Accessible.Grouping
                Accessible.description: "Choose a color theme preview; the selected swatch is marked as selected. Current theme: " + window.selectedTheme + "."
                ToolTip.text: "Theme preview choices; current theme: " + window.selectedTheme
                Repeater {
                    model: window.themeNames.length
                    delegate: Button {
                        id: themePreviewButton
                        required property int index
                        objectName: "themePreview" + index
                        width: 18; height: 18
                        padding: 0
                        checkable: true
                        activeFocusOnTab: true
                        checked: window.selectedThemeIndex === index
                        Accessible.name: window.themeNames[index] + " theme preview"
                        Accessible.description: "Select " + window.themeNames[index] + " theme; "
                                              + (checked ? "selected" : "not selected")
                                              + ". Palette background " + window.themeBackgrounds[index]
                                              + ", accent " + window.themeAccentColors[index]
                                              + ", foreground " + window.themeForegrounds[index]
                                              + ". Motion is " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced" : "full") + ". Use Left and Right to move, Home and End to jump, or Escape to return to the theme selector."
                        Accessible.role: Accessible.RadioButton
                        Accessible.value: (checked ? "selected" : "not selected")
                                              + "; " + window.themeAccentColors[index] + " accent"
                        ToolTip.text: checked ? window.themeNames[index] + " theme is selected" : "Select " + window.themeNames[index] + " theme"
                        background: Rectangle {
                            objectName: "themePreviewSurface" + index
                            radius: 3
                            color: window.themeBackgrounds[index]
                            border.width: window.selectedThemeIndex === index ? 2 : 1
                            border.color: window.themeAccentColors[index]
                            // Keep keyboard focus visible even when a theme's
                            // palette is dark or low contrast.  The ring is
                            // deliberately outside the swatch so it cannot be
                            // mistaken for the selected-state border.
                            Rectangle {
                                objectName: "themePreviewFocusRing" + index
                                anchors.fill: parent
                                anchors.margins: -3
                                radius: parent.radius + 2
                                color: "transparent"
                                border.width: 3
                                border.color: window.themeAccentColors[index]
                                opacity: themePreviewButton.activeFocus ? 1 : 0
                                visible: opacity > 0
                                Behavior on opacity { enabled: !window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion; NumberAnimation { duration: 120 } }
                            }
                            Rectangle {
                                objectName: "themePreviewAccent" + index
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 3
                                anchors.bottomMargin: 3
                                width: parent.width - 6
                                height: 3
                                radius: 1
                                color: window.themeAccentColors[index]
                            }
                            Rectangle {
                                objectName: "themePreviewForeground" + index
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.rightMargin: 3
                                anchors.topMargin: 3
                                width: 5
                                height: 5
                                radius: 2
                                color: window.themeForegrounds[index]
                            }
                        }
                        Keys.onLeftPressed: {
                            const nextIndex = (index - 1 + window.themeNames.length) % window.themeNames.length
                            window.selectThemeIndex(nextIndex)
                            themeSelector.currentIndex = nextIndex
                            themePreviews.children[nextIndex].forceActiveFocus()
                            event.accepted = true
                        }
                        Keys.onRightPressed: {
                            const nextIndex = (index + 1) % window.themeNames.length
                            window.selectThemeIndex(nextIndex)
                            themeSelector.currentIndex = nextIndex
                            themePreviews.children[nextIndex].forceActiveFocus()
                            event.accepted = true
                        }
                        Keys.onHomePressed: {
                            window.selectThemeIndex(0)
                            themeSelector.currentIndex = 0
                            themePreviews.children[0].forceActiveFocus()
                            event.accepted = true
                        }
                        Keys.onPressed: {
                            if (event.key === Qt.Key_End) {
                                const lastIndex = window.themeNames.length - 1
                                window.selectThemeIndex(lastIndex)
                                themeSelector.currentIndex = lastIndex
                                themePreviews.children[lastIndex].forceActiveFocus()
                                event.accepted = true
                            }
                        }
                        Keys.onEscapePressed: {
                            themeSelector.forceActiveFocus()
                            event.accepted = true
                        }
                        onClicked: {
                            window.selectThemeIndex(index)
                            themeSelector.currentIndex = index
                        }
                    }
                }
            }
        }
    }
    background: Rectangle { color: window.themeBackgrounds[window.selectedThemeIndex] }
    StackLayout {
        anchors.fill: parent
        currentIndex: navigation.currentIndex
        Item {
            objectName: "libraryPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 10
                Label { text: "Library"; color: window.themeForegrounds[window.selectedThemeIndex]; font.pixelSize: 28; font.bold: true; Accessible.name: "Library"; Accessible.role: Accessible.Heading; ToolTip.text: "Browse local audio files and folders" }
                Label {
                    objectName: "catalogStatus"
                    text: window.browseFolder ? (files.count + " catalog entries shown. Use the search field or open a folder to browse.") : "Choose a folder to browse local audio files."
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                    Accessible.name: "Library catalog status: " + text
                    Accessible.value: text + "; summary review available"
                    Accessible.role: Accessible.StaticText
                    ToolTip.text: "Current catalog result count"
                    Accessible.description: text
                }
                Label {
                    id: librarySelectionStatus
                    objectName: "librarySelectionStatus"
                    activeFocusOnTab: true
                    text: window.selectedAudioUrl.toString() === "" ? "No Library source selected." : "Selected source: " + window.selectedName
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    Accessible.name: "Library selection status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: window.selectedAudioUrl.toString() === "" ? "No audio source is selected. Choose an audio file from the browser." : text + ". Playback and analysis actions apply to this source."
                    ToolTip.text: Accessible.description
                }
                Label {
                    id: libraryShortcutHelp
                    objectName: "libraryShortcutHelp"
                    activeFocusOnTab: true
                    text: "Shortcuts: Ctrl+Alt+Shift+H help · Ctrl+F filter · Ctrl+Alt+Shift+Backspace clear filter · Ctrl+Alt+Shift+I folder path · Ctrl+Alt+Shift+W parent action · Ctrl+Alt+Shift+Z recursive action · Ctrl+Alt+Shift+J return action · Ctrl+Alt+Shift+B return now · Ctrl+Alt+Shift+K recursive analysis · Ctrl+Alt+Shift+U batch status · Ctrl+Alt+Shift+V recovery status · Ctrl+Alt+Shift+X admission status · Ctrl+Alt+Shift+G analysis progress · Ctrl+Alt+Shift+T analysis source · Ctrl+Alt+Shift+0 analysis freshness · Ctrl+Alt+Shift+9 analysis provenance · Ctrl+Alt+Shift+8 analysis uncertainty · Ctrl+Alt+Shift+7 analysis failure · Ctrl+Alt+Shift+6 queue progress · Ctrl+Alt+Shift+5 batch progress · Ctrl+Alt+Shift+4 batch result · Ctrl+Alt+Shift+3 publication summary · Ctrl+Alt+Shift+2 batch source · Ctrl+Alt+Shift+1 recovery action · Ctrl+Alt+Shift+F metadata error · Ctrl+Alt+L browser · Ctrl+Alt+Shift+L results · Ctrl+Alt+Shift+O sort control · Ctrl+Alt+Shift+M metadata status · Ctrl+Alt+Shift+N analysis action · Ctrl+Alt+Shift+Y playback action · Ctrl+Alt+Shift+Q refresh action · Ctrl+Alt+Shift+D folder chooser · Ctrl+Alt+Shift+C catalog status · Ctrl+Alt+Shift+E selection status · Ctrl+Alt+R recursive catalog · Ctrl+Alt+Up parent folder · Ctrl+R refresh folder · Ctrl+Alt+S reverse name order · Ctrl+Alt+Backspace clear selection"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    Accessible.name: "Library keyboard shortcut help"
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: "Keyboard shortcuts for filtering, focusing results, reversing sort order, and clearing the current selection."
                    ToolTip.text: Accessible.description
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { id: chooseFolder; text: "Choose folder…"; objectName: "chooseFolder"; activeFocusOnTab: true; Accessible.name: "Choose audio folder"; Accessible.value: window.browseFolder.toString() || "No folder selected"; Accessible.role: Accessible.Button; Accessible.description: "Choose a local folder to browse"; ToolTip.text: "Choose a local folder to browse"; onClicked: window.openLibraryFolderChooser() }
                    Button {
                        id: addFolderRecursively
                        text: "Add folder recursively"; objectName: "addFolderRecursively"; activeFocusOnTab: true
                        enabled: window.libraryCatalogBridge !== null && window.browseFolder.toString() !== ""
                        Accessible.value: enabled ? "available" : "choose a folder first"
                        Accessible.name: "Add folder recursively"; Accessible.role: Accessible.Button
                        Accessible.description: "Enumerate supported audio files in this folder and all descendant folders without following links."
                        ToolTip.text: "Enumerate supported audio files in this folder and all descendant folders"
                        onClicked: {
                            window.clearSelectedAudio()
                            browser.currentIndex = -1
                            window.recursiveCatalogRows = window.libraryCatalogBridge.browseRecursive(window.browseFolder, search.text)
                            window.recursiveCatalogActive = true
                            window.catalogDiagnostic = window.libraryCatalogBridge.lastDiagnostic
                            window.announceVisualizationRecovery(window.recursiveCatalogRows.length + " recursive catalog entries found; use the folder browser to inspect or select a source.")
                            recursiveResultsView.forceActiveFocus()
                        }
                    }
                    Button {
                        id: parentFolder; text: "Up"; objectName: "parentFolder"
                        activeFocusOnTab: true
                        enabled: window.browseFolder.toString() !== "" && files.parentFolder.toString() !== ""
                        Accessible.value: enabled ? "available" : "at folder root"
                        onClicked: window.navigateLibraryParentFolder()
                        Accessible.name: "Go to parent folder"
                        Accessible.role: Accessible.Button
                        Accessible.description: "Go to the parent folder"
                        ToolTip.text: "Go to the parent folder"
                    }
                    Button {
                        id: refreshFolder; text: "Refresh"; objectName: "refreshFolder"; activeFocusOnTab: true
                        enabled: window.browseFolder.toString() !== ""
                        Accessible.name: "Refresh folder contents"; Accessible.value: enabled ? "available; clears the current selection" : "unavailable; choose a folder first"; Accessible.role: Accessible.Button
                        Accessible.description: "Re-read the current folder so newly added, removed, or changed audio files are reflected."
                        ToolTip.text: "Refresh the current folder contents"
                        onClicked: { files.refresh(); window.refreshCatalog(); window.resetLibraryBrowserSelection("Folder contents refreshed; entry selection cleared.") }
                    }
                    Button { id: clearSavedFolder; objectName: "clearSavedFolder"; text: "Forget folder"; enabled: typeof libraryPreferences !== "undefined" && libraryPreferences.folder !== ""; Accessible.name: "Forget saved Library folder"; Accessible.value: enabled ? "available" : "no saved folder"; Accessible.description: "Clear the persisted Library folder without modifying audio files."; onClicked: { libraryPreferences.clearFolder(); window.clearSelectedAudio(); window.browseFolder = ""; window.announceVisualizationRecovery("Saved Library folder cleared; choose a folder to browse."); clearSavedFolder.forceActiveFocus() } }
                    Label {
                        id: folderPath
                        objectName: "folderPath"
                        activeFocusOnTab: true
                        text: window.browseFolder.toString() || "No folder selected"
                        elide: Text.ElideMiddle; Layout.fillWidth: true
                        Accessible.name: "Current folder: " + text
                        Accessible.value: text + "; settings revision " + window.overlaySettingsRevision
                        Accessible.role: Accessible.StaticText
                        ToolTip.text: "Current folder path"
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField { id: search; objectName: "search"; placeholderText: "Filter file names"; Accessible.name: "Filter file names"; Accessible.role: Accessible.EditableText; Accessible.description: "Filter the current folder by audio or folder name"; ToolTip.text: "Filter the current folder by audio or folder name"; Layout.fillWidth: true; onTextChanged: { window.refreshCatalog(); browser.currentIndex = -1 } onAccepted: { navigation.currentIndex = 0; window.announceVisualizationRecovery(files.count + " Library entries match the filter."); browser.forceActiveFocus(); event.accepted = true } Keys.onEscapePressed: { window.clearLibraryFilter(browser); event.accepted = true } }
                    Button { id: sortOrder; objectName: "sortOrder"; text: window.sortDescending ? "Name ↓" : "Name ↑"; activeFocusOnTab: true; Accessible.name: "Reverse name sort order"; Accessible.value: window.sortDescending ? "descending" : "ascending"; Accessible.role: Accessible.Button; Accessible.description: "Current order: " + (window.sortDescending ? "descending" : "ascending") + ". Activate to reverse file-name order."; ToolTip.text: "Reverse file-name sort order"; onClicked: window.toggleLibrarySort(sortOrder) }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Name"; font.bold: true; Layout.fillWidth: true; Accessible.name: "File name column"; Accessible.role: Accessible.ColumnHeader; ToolTip.text: "Audio file or folder name" }
                    Label { text: "Type"; font.bold: true; Layout.preferredWidth: 100; Accessible.name: "File type column"; Accessible.role: Accessible.ColumnHeader; ToolTip.text: "Audio format or folder type" }
                }
                Label {
                    id: catalogResultCount
                    objectName: "catalogResultCount"
                    activeFocusOnTab: true
                        text: window.browseFolder.toString() === "" ? "Catalog: choose a folder" : window.recursiveCatalogActive ? "Recursive catalog: " + window.recursiveCatalogRows.length + " entries" : "Catalog: " + window.catalogRows.length + " matching entries"
                    Accessible.name: "Catalog results: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Current filtered catalog result count"
                }
                Label {
                    objectName: "libraryBatchRecoveryBanner"
                    visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchAcceptedCount > 0
                    text: visible ? ("Analysis batch: " + window.analysisJobBridge.status
                          + " · accepted " + window.analysisJobBridge.batchAcceptedCount
                          + " · complete " + window.analysisJobBridge.batchCompletedCount
                          + " · failed " + window.analysisJobBridge.batchFailedCount
                          + " · cancelled " + window.analysisJobBridge.batchCancelledCount
                          + ((window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled") ? " · recovery actions available below" : "")) : ""
                    Accessible.name: "Library analysis batch recovery: " + text
                    Accessible.value: visible ? text : ""
                    Accessible.role: Accessible.Status
                    Accessible.description: visible && (window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled") ? text + ". Retry or discard the retained batch in the selected source panel; the prior result is cleared before retry." : visible ? text + ". Progress updates as worker outcomes are published." : "No analysis batch is active."
                    ToolTip.text: visible ? Accessible.description : "Library analysis batch status"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "libraryStorageRecoveryHealth"
                    visible: typeof storageRecovery !== "undefined"
                    text: visible ? ("Library storage: " + storageRecovery.status
                          + (storageRecovery.retryableRunIds.length > 0 ? " · " + storageRecovery.retryableRunIds.length + " saved run(s)" : " · no saved runs")) : ""
                    Accessible.name: "Library storage recovery health"
                    Accessible.value: visible ? text : ""
                    Accessible.role: Accessible.Status
                    Accessible.description: visible && (storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0)
                        ? text + ". Reopen storage before retrying or discarding saved analysis runs."
                        : visible ? text + ". Saved recovery state is synchronized with the Library controls." : "Storage recovery health is unavailable."
                    ToolTip.text: visible ? Accessible.description : "Library storage recovery health"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "libraryPlaybackRecoveryStatus"
                    visible: window.playbackBridge !== null && window.selectedAudioUrl.toString() !== ""
                    text: visible ? ("Library playback: " + window.playbackBridge.state + (window.playbackMatchesSelection ? " · selected source" : " · another source is open")) : ""
                    Accessible.name: "Library playback recovery status"
                    Accessible.value: visible ? text : ""
                    Accessible.role: Accessible.Status
                    Accessible.description: visible && !window.playbackMatchesSelection ? text + ". Reopen selected playback to align the source before seeking." : visible ? text + ". Playback controls operate on the selected source." : "Playback status is unavailable."
                    ToolTip.text: visible ? Accessible.description : "Library playback recovery status"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                Frame {
                    objectName: "browserFrame"
                    Layout.fillWidth: true; Layout.fillHeight: true
                    visible: !window.recursiveCatalogActive
                    Accessible.name: "Library folder browser"
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Browse folders and audio files in the current location."
                    ToolTip.text: "Library folder and audio-file browser"
                    ListView {
                        id: browser
                        objectName: "browser"
                        anchors.fill: parent
                        clip: true
                        model: window.browseFolder.toString() === "" ? null : files
                        currentIndex: -1
                        keyNavigationEnabled: true
                        activeFocusOnTab: true
                        Accessible.name: "Folder contents"
                        Accessible.value: currentIndex >= 0 ? (currentIndex + 1) + " of " + count : "No entry selected"
                        Accessible.role: Accessible.List
                        Accessible.description: "Use Up and Down to select an entry, then Enter or Space to open a folder or select an audio file."
                        ToolTip.text: "Folder contents; use arrow keys to navigate"
                        Keys.onReturnPressed: { if (currentIndex >= 0) window.activateEntry(currentIndex); else window.announceVisualizationRecovery("Choose a Library entry before activating."); event.accepted = true }
                        Keys.onEnterPressed: { if (currentIndex >= 0) window.activateEntry(currentIndex); else window.announceVisualizationRecovery("Choose a Library entry before activating."); event.accepted = true }
                        Keys.onSpacePressed: { if (currentIndex >= 0) window.activateEntry(currentIndex); else window.announceVisualizationRecovery("Choose a Library entry before activating."); event.accepted = true }
                        Keys.onBackspacePressed: {
                            if (files.parentFolder.toString() !== "") {
                                window.clearSelectedAudio()
                                window.browseFolder = files.parentFolder
                                search.text = ""
                                window.recursiveCatalogActive = false
                                window.recursiveCatalogRows = []
                                window.announceVisualizationRecovery("Moved to the parent Library folder; choose an audio source to inspect.")
                                browser.currentIndex = -1
                                event.accepted = true
                            } else {
                                window.announceVisualizationRecovery("Already at the top Library folder; parent navigation is unavailable.")
                                event.accepted = true
                            }
                        }
                        Keys.onEscapePressed: { window.clearLibrarySelectionAndFilter(search); event.accepted = true }
                        ScrollBar.vertical: ScrollBar {}
                        delegate: ItemDelegate {
                            required property int index
                            required property string fileName
                            required property bool fileIsDir
                            width: ListView.view.width
                            highlighted: ListView.isCurrentItem
                            activeFocusOnTab: true
                            Accessible.name: fileName + (fileIsDir ? ", folder" : ", audio file")
                            Accessible.value: fileIsDir ? "folder" : "audio file"
                            Accessible.role: Accessible.ListItem
                            Accessible.description: fileIsDir ? "Open this folder" : "Select this audio file for metadata, analysis and playback"
                            ToolTip.text: fileIsDir ? "Open folder " + fileName : "Select audio file " + fileName
                            onClicked: { browser.currentIndex = index; window.activateEntry(index); event.accepted = true }
                            onDoubleClicked: { browser.currentIndex = index; window.activateEntry(index); if (!fileIsDir) playSelectedTrack.click(); event.accepted = true }
                            contentItem: RowLayout {
                                Label { text: fileName; elide: Text.ElideMiddle; Layout.fillWidth: true; Accessible.ignored: true }
                                Label { text: fileIsDir ? "Folder" : "Audio file"; Layout.preferredWidth: 100; Accessible.ignored: true }
                            }
                        }
                        Column {
                            anchors.centerIn: parent
                            objectName: "browserEmptyState"
                            visible: !window.browseFolder.toString() || (files.status === FolderListModel.Ready && files.count === 0)
                            spacing: 8
                            Accessible.name: "Library browser empty state"
                            Accessible.value: window.browserEmptyMessage()
                            Accessible.role: Accessible.Grouping
                            Accessible.description: "No folder entries are available; choose a folder to recover browsing."
                            ToolTip.text: "Library browser empty or unavailable state"
                            Label {
                                objectName: "browserEmptyMessage"
                                text: window.browserEmptyMessage()
                                Accessible.name: "Library browser empty state: " + text
                                Accessible.value: text
                                Accessible.role: Accessible.StaticText
                                Accessible.description: text + " Use Choose folder to recover from an empty or unreadable location."
                                ToolTip.text: "Library browser recovery state"
                            }
                            Button {
                                objectName: "chooseFolderEmpty"
                                text: "Choose folder…"
                                Accessible.name: "Choose a folder"
                                Accessible.value: "Recovery action available"
                                Accessible.role: Accessible.Button
                                Accessible.description: "Choose a local folder to populate the Library. Current state: " + window.browserEmptyMessage()
                                ToolTip.text: "Choose a local folder to recover the library"
                                onClicked: { navigation.currentIndex = 0; window.announceVisualizationRecovery("Choose a Library folder dialog opened from the empty state."); folderPicker.open(); chooseFolderEmpty.forceActiveFocus() }
                            }
                        }
                        RowLayout {
                            objectName: "playbackControls"
                            Layout.fillWidth: true
                            Accessible.name: "Playback controls"
                            Accessible.value: window.playbackBridge ? window.playbackBridge.state : "unavailable"
                            Accessible.role: Accessible.Grouping
                            Accessible.description: "Open, configure, and control playback for the selected audio source."
                            ToolTip.text: window.playbackBridge ? "Playback controls; current state: " + window.playbackBridge.state : "Playback controls are unavailable"
                            Button { id: openPlayback; text: "Open"; objectName: "openPlayback"; activeFocusOnTab: true; Accessible.name: "Open selected playback source"; Accessible.value: window.selectedAudioUrl.toString() === "" ? "unavailable; select an audio source" : (!window.playbackBridge ? "unavailable; playback unavailable" : "available; " + window.selectedName); Accessible.role: Accessible.Button; Accessible.description: window.selectedAudioUrl.toString() === "" ? "Select an audio source before opening playback" : "Open the selected audio source for playback"; ToolTip.text: "Open the selected audio source for playback"; enabled: window.selectedAudioUrl.toString() !== "" && window.playbackBridge; onClicked: { const accepted = window.playbackBridge.openSource(window.selectedAudioUrl, window.playbackBridge.epoch + 1); if (accepted) window.clearVisualizationForCaptureState("Playback source opened; previous visualization evidence is stale until fresh accepted analysis."); window.announceCaptureRecoveryState(accepted ? "Selected Library playback source opened." : "Selected Library playback source could not be opened."); openPlayback.forceActiveFocus() } }
                            Button { text: "Play selected"; objectName: "playSelectedTrack"; activeFocusOnTab: true; Accessible.name: "Play selected Library track"; Accessible.value: window.selectedAudioUrl.toString() === "" ? "unavailable; select an audio source" : (!window.playbackBridge ? "unavailable; playback unavailable" : "available; " + window.selectedName); Accessible.role: Accessible.Button; Accessible.description: window.selectedAudioUrl.toString() === "" ? "Select an audio source before starting playback" : "Open and play the selected Library source"; ToolTip.text: "Play the selected Library track"; enabled: window.selectedAudioUrl.toString() !== "" && window.playbackBridge; onClicked: { window.playSelectedLibraryTrack(); playSelectedTrack.forceActiveFocus() } }
                            SpinBox { objectName: "playbackSampleRate"; activeFocusOnTab: true; from: 8000; to: 192000; value: 44100; editable: true; Accessible.name: "Playback sample rate"; Accessible.value: value + " hertz"; Accessible.role: Accessible.SpinBox; Accessible.description: "Native playback sample rate in hertz"; ToolTip.text: "Choose the native playback sample rate in hertz" }
                            SpinBox { objectName: "playbackChannels"; activeFocusOnTab: true; from: 1; to: 8; value: 2; editable: true; Accessible.name: "Playback channels"; Accessible.value: value + " channels"; Accessible.role: Accessible.SpinBox; Accessible.description: "Native playback channel count"; ToolTip.text: "Choose the native playback channel count" }
                            Button { id: startNativePlayback; text: "Start native"; objectName: "startNativePlayback"; enabled: window.playbackBridge && playbackDevice.currentIndex >= 0; Accessible.name: "Start native playback"; Accessible.role: Accessible.Button; Accessible.description: window.playbackBridge ? "Start native playback at " + playbackSampleRate.value + " hertz with " + playbackChannels.value + " channels on the selected output device" : "Native playback is unavailable in this build"; ToolTip.text: "Start native playback using the selected format"; onClicked: { window.startNativePlaybackOutput(); startNativePlayback.forceActiveFocus() } }
                            Label { text: window.playbackBridge && window.playbackBridge.nativeRunning ? "Native output active" : "Native output stopped"; objectName: "nativePlaybackState"; Accessible.name: "Native playback state: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current native output running state" }
                            Label { text: window.playbackBridge ? window.playbackBridge.nativeError : ""; objectName: "nativePlaybackError"; Accessible.name: "Native playback error: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Native playback error details"; visible: text !== ""; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Button { id: retryNativePlayback; text: "Retry native"; objectName: "retryNativePlayback"; enabled: window.playbackBridge && window.playbackBridge.nativeError !== ""; Accessible.name: "Retry native playback"; Accessible.role: Accessible.Button; Accessible.description: "Retry starting native playback after an output error"; ToolTip.text: "Retry native playback after the output error"; onClicked: { window.retryNativePlaybackOutput(); retryNativePlayback.forceActiveFocus() } }
                            Button { text: "Reopen selected"; objectName: "reopenSelectedPlayback"; visible: window.selectedAudioUrl.toString() !== "" && window.playbackBridge && !window.playbackMatchesSelection; enabled: visible; Accessible.name: "Reopen selected playback source"; Accessible.value: "available; replace playback with " + window.selectedName; Accessible.role: Accessible.Button; Accessible.description: "Replace the current playback source with the selected audio source: " + window.selectedName; ToolTip.text: "Replace playback with the selected audio source"; onClicked: { window.reopenSelectedLibraryTrack(); reopenSelectedPlayback.forceActiveFocus() } }
                            Button { id: pausePlayback; text: window.playbackBridge && window.playbackBridge.state === "Paused" ? "Resume" : "Pause"; objectName: "pausePlayback"; Accessible.name: text + " playback"; Accessible.role: Accessible.Button; Accessible.description: window.playbackBridge && window.playbackBridge.state === "Paused" ? "Resume the selected audio source." : "Pause the selected audio source."; ToolTip.text: "Pause or resume playback"; enabled: window.playbackBridge && (window.playbackBridge.state === "Playing" || window.playbackBridge.state === "Paused"); onClicked: { window.togglePlaybackPause(); pausePlayback.forceActiveFocus() } }
                            Button { id: stopPlayback; text: "Stop"; objectName: "stopPlayback"; Accessible.name: "Stop playback"; Accessible.role: Accessible.Button; Accessible.description: "Stop playback and clear the current source."; ToolTip.text: "Stop playback and clear the current source"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: { window.stopPlaybackWithRecovery("Playback stopped; the source is ready to reopen or select another file."); stopPlayback.forceActiveFocus() } }
                            SpinBox { id: seekSeconds; objectName: "seekSeconds"; from: 1; to: 60; value: 5; editable: true; activeFocusOnTab: true; Accessible.name: "Seek step seconds"; Accessible.value: value + " seconds"; Accessible.role: Accessible.SpinBox; Accessible.description: "Choose the number of seconds moved by the backward and forward seek buttons"; ToolTip.text: "Choose the playback seek step in seconds" }
                            Button { id: seekBackPlayback; text: "−" + seekSeconds.value + " s"; objectName: "seekBackPlayback"; Accessible.name: seekSeconds.value === 5 ? "Seek backward five seconds" : "Seek backward " + seekSeconds.value + " seconds"; Accessible.role: Accessible.Button; Accessible.description: "Move playback backward by the selected number of seconds at the selected sample rate"; ToolTip.text: "Seek backward " + seekSeconds.value + " seconds"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: { if (window.playbackBridge && window.playbackBridge.state !== "Stopped") { window.playbackBridge.seek(Math.max(0, window.playbackBridge.currentFrame - seekSeconds.value * playbackSampleRate.value), window.playbackBridge.epoch + 1); window.announceCaptureRecoveryState("Playback sought backward " + seekSeconds.value + " seconds."); seekBackPlayback.forceActiveFocus() } else window.announceCaptureRecoveryState("Seek backward is no longer available because playback stopped.") } }
                            Button { id: seekForwardPlayback; text: "+" + seekSeconds.value + " s"; objectName: "seekForwardPlayback"; Accessible.name: seekSeconds.value === 5 ? "Seek forward five seconds" : "Seek forward " + seekSeconds.value + " seconds"; Accessible.role: Accessible.Button; Accessible.description: "Move playback forward by the selected number of seconds at the selected sample rate"; ToolTip.text: "Seek forward " + seekSeconds.value + " seconds"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: { if (window.playbackBridge && window.playbackBridge.state !== "Stopped") { window.playbackBridge.seek(window.playbackBridge.currentFrame + seekSeconds.value * playbackSampleRate.value, window.playbackBridge.epoch + 1); window.announceCaptureRecoveryState("Playback sought forward " + seekSeconds.value + " seconds."); seekForwardPlayback.forceActiveFocus() } else window.announceCaptureRecoveryState("Seek forward is no longer available because playback stopped.") } }
                            SpinBox { id: seekFrame; objectName: "seekFrame"; from: 0; to: 2147483647; value: window.playbackBridge ? Math.min(seekFrame.to, window.playbackBridge.currentFrame) : 0; editable: true; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; Accessible.name: "Seek target frame"; Accessible.value: value; Accessible.role: Accessible.SpinBox; Accessible.description: "Enter an audio frame and apply it as the playback position"; ToolTip.text: "Enter a source frame, then choose Seek" }
                            Button { id: seekToFrame; text: "Seek"; objectName: "seekToFrame"; enabled: seekFrame.enabled; Accessible.name: "Seek to entered frame"; Accessible.role: Accessible.Button; Accessible.description: "Seek playback to the entered audio frame"; ToolTip.text: "Seek playback to the entered source frame"; onClicked: { if (window.playbackBridge && window.playbackBridge.state !== "Stopped") { window.playbackBridge.seek(seekFrame.value, window.playbackBridge.epoch + 1); window.announceCaptureRecoveryState("Playback seek requested to frame " + seekFrame.value + "."); seekToFrame.forceActiveFocus() } else window.announceCaptureRecoveryState("Seek is no longer available because playback stopped.") } }
                            ComboBox { objectName: "playbackDevice"; model: window.playbackBridge ? window.playbackBridge.devices : []; activeFocusOnTab: true; enabled: window.playbackBridge; Accessible.name: "Playback output device"; Accessible.role: Accessible.ComboBox; Accessible.value: currentText; Accessible.description: currentText !== "" ? "Current output device: " + currentText + ". Choose the native playback output device." : "No output device selected. Choose the native playback output device."; ToolTip.text: "Choose the output device used for native playback"; onActivated: if (window.playbackBridge) window.playbackBridge.deviceId = window.playbackDeviceIdAt(currentIndex, currentText) }
                            Button { text: "Refresh outputs"; objectName: "refreshPlaybackDevices"; activeFocusOnTab: true; enabled: window.playbackBridge; Accessible.name: "Refresh playback outputs"; Accessible.role: Accessible.Button; Accessible.description: "Refresh native playback output devices"; ToolTip.text: "Refresh the available playback output devices"; onClicked: { window.refreshPlaybackOutputs(); playbackDevice.forceActiveFocus() } }
                            Label { text: window.playbackBridge ? "Playback: " + window.playbackBridge.state : "Playback unavailable"; objectName: "playbackState"; Accessible.name: "Playback state: " + text; Accessible.value: window.playbackBridge ? window.playbackBridge.state : "unavailable"; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current native playback state" }
                            Label { text: window.playbackBridge && window.playbackBridge.source.toString() !== "" ? "Source: " + window.playbackBridge.source.fileName : "No source opened"; objectName: "playbackSource"; Accessible.name: "Playback source: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current playback source"; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Label { text: window.playbackBridge ? "Frame " + window.playbackBridge.currentFrame + " · Epoch " + window.playbackBridge.epoch : ""; objectName: "playbackPosition"; Accessible.name: "Playback position: " + text; Accessible.value: window.playbackBridge ? window.playbackBridge.currentFrame : 0; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current source frame and playback epoch" }
                            Label { text: window.playbackBridge && window.playbackBridge.state !== "Stopped" ? "Seek uses audio frames; track duration is not available yet." : ""; objectName: "playbackSeekLimitation"; Accessible.name: "Playback seek limitation: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Playback seek currently uses source audio frames"; visible: text !== ""; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Label { text: window.playbackBridge === null ? "Seek unavailable until playback is connected" : (window.playbackBridge.state === "Stopped" ? "Seek unavailable while playback is stopped" : (!window.playbackMatchesSelection ? "Seek unavailable until the selected source is reopened" : "Seek ready for the selected source")); objectName: "playbackSeekActionStatus"; Accessible.name: "Playback seek action status: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text + ". Seek operates on source audio frames and does not modify the source file."; ToolTip.text: "Current playback seek availability"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            Label { text: window.playbackMatchesSelection ? "Playing selected source" : (window.selectedAudioUrl.toString() !== "" && window.playbackBridge && window.playbackBridge.source.toString() !== "" ? "Playback is on another source" : ""); objectName: "playbackSelectionStatus"; Accessible.name: "Playback selection status: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Whether playback matches the selected library source"; visible: text !== "" }
                            Label {
                                objectName: "playbackSourceIdentityStatus"
                                visible: window.playbackBridge !== null && window.playbackBridge.source.toString() !== ""
                                text: visible ? (window.playbackMatchesSelection ? "Playback source identity: selected Library source" : "Playback source identity: differs from selected Library source") : ""
                                Accessible.name: "Playback source identity: " + text
                                Accessible.value: visible ? text : ""
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? text + ". Reopen selected playback when the identities differ; source audio remains unchanged." : "No playback source identity is available."
                                ToolTip.text: "Playback source identity compared with Library selection"
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "playbackRecoveryStatus"
                                visible: window.playbackBridge !== null && (window.playbackBridge.nativeError !== "" || !window.playbackMatchesSelection)
                                text: visible && window.playbackBridge.nativeError !== "" ? "Playback recovery: " + window.playbackBridge.nativeError : visible ? "Playback recovery: reopen the selected source to align playback with the Library selection" : ""
                                Accessible.name: "Playback recovery status: " + text
                                Accessible.value: visible ? text : ""
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? text + ". The selected audio source remains unchanged." : "Playback recovery is not required."
                                ToolTip.text: "Playback recovery and source alignment status"
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                        }
                        Label {
                            objectName: "playbackUnavailable"
                            text: "Playback controls are unavailable in this build; metadata and analysis details remain read-only."
                            visible: !window.playbackBridge
                            Accessible.name: "Playback availability: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.Status
                            Accessible.description: text + " Treat this as provisional evidence until source-linked analysis is accepted."
                            ToolTip.text: "Playback controls unavailable; metadata remains read-only"
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                    }
                    Button {
                        id: analyzeRecursiveResults
                        objectName: "analyzeRecursiveResults"
                        anchors.left: parent.left; anchors.bottom: parent.bottom
                        text: "Analyze first files (up to 2 of " + window.recursiveAudioResultCount + ")"
                        enabled: window.analysisJobBridge !== null && window.analysisJobBridge.submitSelectedFiles !== undefined && window.recursiveAudioResultCount > 0 && window.analysisJobBridge.status !== "Queued" && window.analysisJobBridge.status !== "Running"
                        Accessible.value: enabled ? "available; " + window.recursiveAudioResultCount + " audio results found" : "unavailable; no audio results or analysis is active"
                        Accessible.name: "Analyze recursive audio results"
                        Accessible.role: Accessible.Button
                        Accessible.description: enabled ? "Fingerprint and queue up to two of the first available recursive audio results; the bounded queue reports accepted and rejected items." : "An analysis batch is already active; wait for completion or cancel before submitting another batch."
                        ToolTip.text: "Queue the first recursive audio results for offline analysis"
                        onClicked: {
                            navigation.currentIndex = 0
                            window.clearSelectedAudio()
                            const paths = []
                            for (let i = 0; i < window.recursiveCatalogRows.length && paths.length < 2; ++i) {
                                const row = window.recursiveCatalogRows[i]
                                if (row.audio) paths.push(row.path)
                            }
                            const accepted = window.analysisJobBridge.submitSelectedFiles(paths)
                            if (accepted > 0)
                                window.clearVisualizationForCaptureState("Recursive analysis queued; previous visualization evidence is stale until fresh accepted analysis.")
                            window.announceVisualizationRecovery(accepted + " recursive audio files accepted for analysis; " + window.analysisJobBridge.batchRejectedCount + " rejected; queue capacity " + window.analysisJobBridge.batchCapacity + "; status " + window.analysisJobBridge.status + ".")
                            analysisBatchStatus.forceActiveFocus()
                            recursiveResultsView.forceActiveFocus()
                        }
                    }
                }
                Button {
                    id: returnToFolderBrowser
                    objectName: "returnToFolderBrowser"
                    visible: window.recursiveCatalogActive
                    text: "Return to folder browser"
                    activeFocusOnTab: true
                    Accessible.name: "Return to folder browser"
                    Accessible.role: Accessible.Button
                    Accessible.description: "Close recursive results and return to the current folder browser."
                    ToolTip.text: "Return to the current folder browser"
                    onClicked: window.returnFromRecursiveResults()
                }
                Frame {
                    objectName: "recursiveResultsFrame"
                    visible: window.recursiveCatalogActive
                    Layout.fillWidth: true; Layout.preferredHeight: 220
                    Accessible.name: "Recursive library results"
                    Accessible.value: window.recursiveCatalogRows.length + " results"
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Supported audio files found below the selected folder. Select an audio file to inspect its metadata and queue analysis."
                    ListView {
                        id: recursiveResultsView
                        anchors.fill: parent; clip: true; model: window.recursiveCatalogRows
                        activeFocusOnTab: true; keyNavigationEnabled: true
                        Keys.onEscapePressed: { window.returnFromRecursiveResults(); event.accepted = true }
                        Keys.onReturnPressed: { if (currentIndex >= 0) currentItem.click(); else window.announceVisualizationRecovery("Choose a recursive Library result before activating."); event.accepted = true }
                        Keys.onEnterPressed: { if (currentIndex >= 0) currentItem.click(); else window.announceVisualizationRecovery("Choose a recursive Library result before activating."); event.accepted = true }
                        Keys.onSpacePressed: { if (currentIndex >= 0) currentItem.click(); else window.announceVisualizationRecovery("Choose a recursive Library result before activating."); event.accepted = true }
                        Accessible.name: "Recursive audio results"
                        Accessible.role: Accessible.List
                        delegate: ItemDelegate {
                            required property string name
                            required property string path
                            required property bool folder
                            width: ListView.view.width
                            enabled: !folder
                            activeFocusOnTab: true
                            Accessible.name: name + (folder ? ", folder" : ", audio file")
                            Accessible.value: folder ? "folder" : "audio file"
                            ToolTip.text: path
                            Accessible.role: Accessible.ListItem
                            Accessible.description: folder ? "Folder discovered recursively" : "Select this audio file for metadata and analysis"
                            text: name + (folder ? "  (folder)" : "")
                            onClicked: {
                                recursiveResultsView.currentIndex = index
                                navigation.currentIndex = 0
                                window.clearVisualizationForCaptureState("Selection cleared because the audio source changed; the overlay awaits fresh accepted evidence.")
                                window.selectedName = name
                                window.selectedAudioUrl = Qt.resolvedUrl(path)
                                if (window.metadataBridge) window.metadataBridge.inspect(window.selectedAudioUrl)
                                window.announceVisualizationRecovery("Selected recursive audio source " + name + "; prior analysis presentation is stale until this source is analyzed.")
                                analyzeSelectedAudio.forceActiveFocus()
                                event.accepted = true
                            }
                            onDoubleClicked: { if (!folder) { onClicked(); playSelectedTrack.click() }; event.accepted = true }
                        }
                    }
                }
                Frame {
                    objectName: "selectedAudioPanel"
                    Layout.fillWidth: true
                    Accessible.name: "Selected audio panel"
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Read-only details and recovery actions for the selected audio source."
                    ToolTip.text: "Selected audio source details"
                    ColumnLayout {
                        anchors.fill: parent
                        objectName: "selectedAudioDetails"
                        Accessible.name: "Selected audio details"
                        Accessible.value: window.selectedName !== "" ? window.selectedName + ", " + window.metadataStatus : "no audio selected"
                        Accessible.role: Accessible.Grouping
                        Accessible.description: "Selected audio source, metadata, analysis, and recovery details."
                        ToolTip.text: "Selected audio details and recovery state"
                        Label {
                            objectName: "selectedAudioName"
                            text: window.selectedName || "No file selected"
                            font.bold: true; elide: Text.ElideMiddle; Layout.fillWidth: true
                            Accessible.name: "Selected audio file: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: window.metadataStatus.indexOf("Error:") === 0 ? text + ". Retry metadata to attempt decoding again." : text
                            ToolTip.text: "Currently selected audio file"
                        }
                        Label {
                            objectName: "selectedAudioSource"
                            text: window.selectedAudioUrl.toString() !== "" ? "Source: " + window.selectedAudioUrl : "Source unavailable"
                            elide: Text.ElideMiddle; Layout.fillWidth: true
                            Accessible.name: "Selected audio source: " + text
                            ToolTip.text: "Selected source path"
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text + ". This path is read-only; Beatglass never writes tags or source audio here."
                        }
                        Label {
                            objectName: "selectedAudioDuration"
                            text: window.selectedAudioUrl.toString() === "" ? "Duration unavailable" : (window.metadataStatus.indexOf(" | ") >= 0 ? "Metadata: " + window.metadataStatus : "Duration: unknown until decoded metadata is available")
                            Accessible.name: "Selected audio duration: " + text
                            ToolTip.text: "Selected source duration"
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: window.metadataStatus.indexOf("Error:") === 0 ? text + ". Retry metadata to attempt decoding again." : text
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Button {
                            id: clearSelectedAudio
                            objectName: "clearSelectedAudio"
                            text: "Clear selection"
                            activeFocusOnTab: true
                            enabled: window.selectedName !== ""
                            Accessible.name: "Clear selected audio"
                            Accessible.value: enabled ? window.selectedName : "no selection"
                            Accessible.role: Accessible.Button
                            Accessible.description: window.selectedName !== "" ? "Clear the selected audio source and return analysis details to the empty state: " + window.selectedName : "No audio source is selected."
                            ToolTip.text: window.selectedName !== "" ? "Clear " + window.selectedName : "No audio source selected"
                            onClicked: { window.clearSelectedAudio(); clearSelectedAudio.forceActiveFocus() }
                        }
                        Button {
                            objectName: "analyzeSelectedAudio"
                            text: "Analyze selected file"
                            activeFocusOnTab: true
                            enabled: window.selectedAudioUrl.toString() !== "" && window.analysisJobBridge !== null && window.analysisJobBridge.submitSelectedFile !== undefined && window.analysisJobBridge.status !== "Queued" && window.analysisJobBridge.status !== "Running"
                            Accessible.name: "Analyze selected audio file"
                            Accessible.value: enabled ? "available; " + window.selectedName : (window.analysisJobBridge && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running") ? "unavailable; analysis active" : "unavailable; select an audio file")
                            Accessible.role: Accessible.Button
                            Accessible.description: enabled ? "Fingerprint the selected local file and queue real offline analysis." : (window.analysisJobBridge && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running") ? "Analysis is already active for the selected source; wait for completion or cancel before submitting again." : "Select a local audio file before queueing analysis.")
                            ToolTip.text: "Fingerprint and queue analysis for the selected audio file"
                            onClicked: {
                                const accepted = window.analysisJobBridge.submitSelectedFile(window.selectedAudioUrl.toLocalFile())
                                if (accepted)
                                    window.clearVisualizationForCaptureState("Fresh Library analysis queued; previous visualization evidence is stale until completion.")
                                window.announceVisualizationRecovery(accepted ? "Selected audio analysis queued." : "Selected audio analysis could not be queued; check the source and retry.")
                                analyzeSelectedAudio.forceActiveFocus()
                            }
                        }
                        Label {
                            id: metadataStatusLabel
                            objectName: "metadataStatus"; text: window.metadataStatus
                            activeFocusOnTab: true
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                            Accessible.name: "Audio metadata status: " + text
                            ToolTip.text: "Current metadata status"
                            Accessible.value: text
                            Accessible.role: Accessible.Status
                            Accessible.description: window.metadataStatus.indexOf("Error:") === 0 ? text + ". Retry metadata is available for this selected source." : text
                        }
                        Button {
                            id: retryMetadata
                            objectName: "retryMetadata"
                            text: "Retry metadata"
                            visible: window.selectedAudioUrl.toString() !== "" && window.metadataStatus.indexOf("Error:") === 0
                            activeFocusOnTab: true
                            enabled: visible && !window.metadataRetryPending
                            Accessible.name: "Retry metadata"
                            Accessible.value: enabled ? "available; " + window.metadataStatus : "unavailable; retry cooldown active"
                            Accessible.role: Accessible.Button
                            Accessible.description: "Retry reading metadata for the selected audio source after an error. Current status: " + window.metadataStatus
                            ToolTip.text: "Retry metadata for the selected source"
                            onClicked: {
                                window.metadataRetryPending = true
                                metadataRetryCooldown.restart()
                                window.metadataBridge.inspect(window.selectedAudioUrl)
                                window.announceVisualizationRecovery("Metadata retry requested for " + window.selectedName + ".")
                                retryMetadata.forceActiveFocus()
                            }
                        }
                        Label {
                            id: metadataErrorGuidance
                            objectName: "metadataErrorGuidance"
                            activeFocusOnTab: true
                            text: "Metadata could not be read. Retry this file or clear the selection and choose another source."
                            visible: window.metadataStatus.indexOf("Error:") === 0
                            Accessible.name: "Metadata error recovery guidance: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.Status
                            Accessible.description: text + " Use Retry metadata to try the selected source again, or Clear selection to choose another source."
                            ToolTip.text: "Metadata error recovery guidance"
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            objectName: "detailsAvailability"
                            ToolTip.text: "Current analysis and playback detail status"
                            text: window.selectedName ? ("BPM " + analysisPresentation.bpm + " · Key " + analysisPresentation.key + " · Beats " + analysisPresentation.beatCount + " · Observed frames " + analysisPresentation.observedFrames) : "Select an audio file to inspect its read-only details."
                            Layout.fillWidth: true; wrapMode: Text.WordWrap
                            Accessible.name: "Analysis and playback status: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text + (window.libraryAnalysisResultReady ? " Analysis is source-linked and ready." : " Analysis is provisional until an accepted source-linked result is available.")
                        }
                        Label {
                            id: analysisUncertainty
                            objectName: "analysisUncertainty"
                            activeFocusOnTab: true
                            ToolTip.text: "Analysis uncertainty and evidence limitations"
                            text: window.selectedName
                                  ? "Track key: Unknown · No validated track-level key. Window evidence is provisional and source-attributed."
                                  : "Track key: Unknown until a source is selected."
                            Accessible.name: "Analysis uncertainty: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text + " Treat this as provisional evidence until source-linked analysis is accepted."
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            id: analysisFreshness
                            objectName: "analysisFreshness"
                            activeFocusOnTab: true
                            ToolTip.text: "Analysis freshness and source-link status"
                            text: window.selectedName && window.analysisJobBridge && window.analysisJobBridge.status === "Complete" ? "Analysis freshness: source-linked analysis complete." : "Analysis freshness: waiting for source-linked analysis evidence."
                            Accessible.name: "Analysis freshness: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.Status
                            Accessible.description: text + (window.selectedName && window.analysisJobBridge && window.analysisJobBridge.status === "Complete" ? " This result is ready for source-linked presentation." : " Wait for accepted source-linked analysis before treating details as current.")
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            id: analysisProvenance
                            objectName: "analysisProvenance"
                            activeFocusOnTab: true
                            visible: window.analysisJobBridge !== null && window.analysisJobBridge.engineVersion !== ""
                            text: visible ? ("Analysis provenance: " + window.analysisJobBridge.engineVersion + " · " + window.analysisJobBridge.configurationId + " · " + window.analysisJobBridge.analysisMode) : ""
                            Accessible.name: "Analysis provenance: " + text
                            Accessible.value: visible ? text : ""
                            Accessible.role: Accessible.Status
                            Accessible.description: visible ? "Engine, configuration, and mode used for the selected analysis; this identifies the evidence provenance." : "Analysis provenance is unavailable until analysis is queued."
                            ToolTip.text: "Engine, configuration, and mode for the selected analysis"
                            Layout.fillWidth: true; wrapMode: Text.WordWrap
                        }
                        Label {
                            objectName: "sourceWorkflowStatus"
                            ToolTip.text: "Selected source playback and metadata workflow status"
                            text: window.selectedAudioUrl.toString() === ""
                                  ? "Select an audio source to inspect and play it."
                                  : "Source workflow · playback: " + (window.playbackBridge ? window.playbackBridge.state : "unavailable") + " · analysis: " + (window.analysisJobBridge === null ? "unavailable" : ((window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running") ? "in progress" : (window.analysisJobBridge.status === "Complete" ? "ready" : "retry available"))) + " · " + window.metadataStatus
                            Accessible.name: "Selected source workflow status"
                            Accessible.value: text
                            Accessible.role: Accessible.Status
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        RowLayout { Layout.fillWidth: true
                            Label {
                                text: !window.selectedName ? "Queued · select an audio source to begin analysis." : (window.analysisJobBridge ? (window.analysisJobBridge.status + " · " + (window.analysisJobBridge.progressKnown ? Math.round(window.analysisJobBridge.progressFraction * 100) + "% · " : "") + "observed " + window.analysisJobBridge.frames + (window.analysisJobBridge.progressKnown ? " of " + window.analysisJobBridge.totalFrames : "") + " frames.") : "Analysis progress unavailable")
                                id: analysisProgress
                                objectName: "analysisProgress"
                                activeFocusOnTab: true
                                Accessible.name: "Analysis progress: " + text
                                ToolTip.text: "Current analysis progress and observed frame count"
                                Accessible.value: text
                                Accessible.role: Accessible.Status
                                Accessible.description: text
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                id: analysisSourcePath
                                objectName: "analysisSourcePath"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.sourcePath !== ""
                                text: visible ? ("Analysis source: " + window.analysisJobBridge.sourcePath) : ""
                                Accessible.name: "Analysis source"
                                Accessible.description: visible ? text : "No analysis source is selected."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap; elide: Text.ElideMiddle
                            }
                            Label {
                            id: analysisFailureReason
                            objectName: "analysisFailureReason"
                            activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.failureReason !== ""
                                text: visible ? ("Analysis issue: " + window.analysisJobBridge.failureReason) : ""
                                Accessible.name: "Analysis failure reason"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text + ". Use Retry analysis or discard recovery when the status permits." : "No analysis failure has been reported."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                            id: analysisRecoveryStatus
                            objectName: "analysisRecoveryStatus"
                            activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.recoveryStatus !== ""
                                text: visible ? ("Recovery status: " + window.analysisJobBridge.recoveryStatus) : ""
                                Accessible.name: "Analysis recovery status"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text + ". Review admission and queue status before choosing Retry or discard recovery." : "No recovery reset has been performed."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Button {
                                objectName: "reopenStorage"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.open !== undefined
                                enabled: visible && (storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0)
                                text: "Reopen storage"
                                Accessible.name: "Reopen analysis storage"
                                Accessible.description: "Retry opening the configured storage database and refresh persisted recovery runs."
                                ToolTip.text: Accessible.description
                                onClicked: { storageRecovery.open(); window.announceVisualizationRecovery("Storage reopen requested; persisted recovery will refresh when the database is available."); forceActiveFocus() }
                            }
                            Label {
                                objectName: "persistedRecoveryResetStatus"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.resetStatus !== undefined && storageRecovery.resetStatus !== ""
                                text: visible ? ("Persisted recovery status: " + storageRecovery.resetStatus) : ""
                                Accessible.name: "Persisted recovery reset status"
                                Accessible.description: visible ? text : "No persisted recovery reset has been performed."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                            id: analysisBatchStatus
                            objectName: "analysisBatchStatus"
                            activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchAcceptedCount !== undefined
                                text: visible ? ("Batch queue · accepted " + window.analysisJobBridge.batchAcceptedCount
                                      + " · rejected " + window.analysisJobBridge.batchRejectedCount
                                      + " · published " + window.analysisJobBridge.batchResultsPublished
                                      + " · failed " + window.analysisJobBridge.batchFailedCount
                                      + " · cancelled " + window.analysisJobBridge.batchCancelledCount
                                      + " · complete " + window.analysisJobBridge.batchCompletedCount
                                      + " · queued " + window.analysisJobBridge.batchQueuedCount
                                      + (window.analysisJobBridge.batchCapacity !== undefined ? (" · capacity " + window.analysisJobBridge.batchCapacity) : "")
                                      + (typeof storageRecovery !== "undefined" && storageRecovery.analysisError !== "" ? (" · recovery: " + storageRecovery.analysisError) : "")) : ""
                                Accessible.name: "Batch analysis queue status: " + text
                                Accessible.value: visible ? text : "unavailable; analysis bridge not reporting"
                                ToolTip.text: "Accepted, rejected, and capacity limits for the analysis queue"
                                Accessible.role: Accessible.StaticText
                                Accessible.description: visible ? "Live batch lifecycle and result counts: " + text + ". Rejected work indicates admission limits; terminal failures use Retry or discard recovery." : "Batch queue status is unavailable until the analysis bridge reports it."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                id: analysisQueueProgress
                                objectName: "analysisQueueProgress"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null
                                text: visible ? ("Queue state: " + window.analysisJobBridge.status + " · frames " + window.analysisJobBridge.frames
                                      + (window.analysisJobBridge.canRetry ? " · retry available" : "")
                                      + (typeof storageRecovery !== "undefined" && storageRecovery.analysisError !== "" ? (" · recovery: " + storageRecovery.analysisError) : "")) : ""
                                Accessible.name: "Analysis queue progress"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? "Live worker progress and retry state: " + text + ". Use Cancel, Retry, or discard recovery according to the reported state." : "Analysis queue progress is unavailable."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            ProgressBar {
                                id: analysisBatchProgressBar
                                objectName: "analysisBatchProgressBar"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchAcceptedCount > 0
                                Layout.fillWidth: true
                                from: 0; to: 1
                                value: visible && window.analysisJobBridge.batchProgressKnown
                                       ? Math.min(1, Math.max(0, window.analysisJobBridge.batchProgressFraction))
                                       : visible && window.analysisJobBridge.batchAcceptedCount > 0
                                         ? Math.min(1, (window.analysisJobBridge.batchCompletedCount + window.analysisJobBridge.batchFailedCount + window.analysisJobBridge.batchCancelledCount) / window.analysisJobBridge.batchAcceptedCount)
                                       : 0
                                indeterminate: visible && !window.analysisJobBridge.batchProgressKnown && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running") && value === 0
                                Accessible.name: "Analysis batch completion"
                                Accessible.value: visible ? (indeterminate ? "in progress; " : "") + Math.round(value * 100) + "% complete; " + window.analysisJobBridge.batchCompletedCount + " completed, " + window.analysisJobBridge.batchFailedCount + " failed, " + window.analysisJobBridge.batchCancelledCount + " cancelled" : "unavailable; no accepted batch items"
                                Accessible.role: Accessible.ProgressBar
                                Accessible.description: visible ? (window.analysisJobBridge.batchProgressKnown ? "Determinate source-frame progress for the accepted analysis batch." : "Completed, failed, or cancelled items out of the accepted analysis batch; source-frame total is not available yet.") + " Cancel is available while running; retry or discard recovery is available after terminal failure." : "Batch progress is unavailable."
                                ToolTip.text: visible ? "Batch completion: " + Math.round(value * 100) + "%" : "Batch progress unavailable"
                            }
                            Label {
                                id: analysisBatchLastResult
                                objectName: "analysisBatchLastResult"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchLastResult !== ""
                                text: visible ? ("Latest batch result: " + window.analysisJobBridge.batchLastResult) : ""
                                Accessible.name: "Latest batch analysis result"
                                Accessible.value: visible ? text : "unavailable; no batch result"
                                Accessible.description: visible ? text + ". This result is source-linked; review batch status for retry or discard recovery." : "No batch analysis result is available."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap; elide: Text.ElideRight
                            }
                            Label {
                                id: analysisBatchPublicationSummary
                                objectName: "analysisBatchPublicationSummary"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchAcceptedCount > 0
                                text: visible ? ("Verified batch publication: " + window.analysisJobBridge.batchResultsPublished + " result(s) published · " + window.analysisJobBridge.batchQueuedCount + " queued · " + window.analysisJobBridge.batchFailedCount + " failed · " + window.analysisJobBridge.batchCancelledCount + " cancelled") : ""
                                Accessible.name: "Analysis batch publication summary: " + text
                                Accessible.value: visible ? text : "unavailable; no accepted items"
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? "Counts distinguish results published after verification from work still queued or terminal without publication. Retry or discard recovery applies only to terminal failures." : "No analysis batch is active; no accepted items are available for publication."
                                ToolTip.text: "Verified publication and terminal batch counts"
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                id: analysisBatchLastSource
                                objectName: "analysisBatchLastSource"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchLastSource !== ""
                                text: visible ? "Latest published source: " + window.analysisJobBridge.batchLastSource : ""
                                Accessible.name: "Latest published analysis source: " + text
                                Accessible.value: visible ? text : "unavailable; no verified batch source"
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? "Source path associated with the most recently verified published batch result. The source remains read-only and tags are never written." : "No verified batch source has been published."
                                ToolTip.text: "Source path for the latest verified batch result"
                                Layout.fillWidth: true; wrapMode: Text.WordWrap; elide: Text.ElideMiddle
                            }
                            Button {
                                objectName: "selectLatestPublishedSource"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchLastSource !== ""
                                text: "Select latest published source"
                                enabled: visible
                                Accessible.name: "Select latest published analysis source"
                                Accessible.value: enabled ? "available; " + window.analysisJobBridge.batchLastSource : "unavailable; no verified published source"
                                Accessible.role: Accessible.Button
                                Accessible.description: "Select the source associated with the latest verified batch result and refresh its metadata details."
                                ToolTip.text: "Select the source associated with the latest verified batch result"
                                onClicked: {
                                    window.selectedAudioUrl = Qt.resolvedUrl(window.analysisJobBridge.batchLastSource)
                                    window.selectedName = window.selectedAudioUrl.fileName
                                    if (window.metadataBridge) window.metadataBridge.inspect(window.selectedAudioUrl)
                                    window.latestPublishedSourceSelectionStatus = "Selected latest published source: " + window.selectedName
                                    window.announceVisualizationRecovery("Latest published analysis source selected.")
                                    selectLatestPublishedSource.forceActiveFocus()
                                }
                            }
                            Label {
                                objectName: "latestPublishedSourceSelectionStatus"
                                visible: window.latestPublishedSourceSelectionStatus !== ""
                                text: visible ? window.latestPublishedSourceSelectionStatus : ""
                                Accessible.name: "Latest published source selection status"
                                Accessible.value: visible ? text : "no latest published source selected"
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? text + ". Metadata refresh was requested for the selected source." : "No latest published source selection has been made."
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "persistedRecoveryStatus"
                                visible: typeof storageRecovery !== "undefined"
                                text: visible ? (storageRecovery.status + (storageRecovery.retryableRunIds.length > 0 ? (" · retryable runs " + storageRecovery.retryableRunIds.length) : "")) : ""
                                + (visible && storageRecovery.status.indexOf("retryable recovery runs discarded") >= 0 ? " · " + storageRecovery.status : "")
                                Accessible.name: "Persisted analysis recovery status: " + text
                                Accessible.value: visible ? text : ""
                                ToolTip.text: "Saved analysis recovery status and retryable runs"
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? text : "Persisted recovery status is unavailable."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                id: analysisRecoveryNextAction
                                objectName: "analysisRecoveryNextAction"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null && (window.analysisJobBridge.status === "Cancelled" || window.analysisJobBridge.status === "Failed")
                                text: visible ? (window.analysisJobBridge.status + " analysis retained for recovery. Use Retry batch to resume accepted work or Discard batch to clear the terminal state.") : ""
                                Accessible.name: "Analysis recovery next action: " + text
                                Accessible.value: visible ? text : ""
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? "The terminal analysis state remains actionable; " + window.analysisJobBridge.batchAcceptedCount + " accepted item(s) can be retried, or discard can clear recovery metadata without changing source audio." : "No terminal analysis recovery action is pending."
                                ToolTip.text: "Retry or discard the retained analysis recovery state"
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "persistedRecoveryRejectionReason"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.status.indexOf("Retry rejected:") >= 0
                                text: visible ? ("Persisted retry decision: " + storageRecovery.status) : ""
                                Accessible.name: "Persisted retry rejection reason"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "No persisted retry rejection has been reported."
                                Accessible.role: Accessible.Status
                                ToolTip.text: "Why persisted analysis was kept retryable instead of being started"
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                            id: analysisAdmissionStatus
                            objectName: "analysisAdmissionStatus"
                            activeFocusOnTab: true
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.analysisStatus !== undefined
                                text: visible ? ("Analysis admission: " + storageRecovery.analysisStatus) : ""
                                Accessible.name: "Analysis admission status"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text + ". Queued or running work can be cancelled; terminal failures can be retried or discarded." : "Analysis admission status is unavailable."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisQueueError"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.analysisError !== ""
                                text: visible ? storageRecovery.analysisError : ""
                                Accessible.name: "Analysis queue error"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "No analysis queue error is present."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "persistedAnalysisProgress"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.analysisFrames !== undefined
                                text: visible ? ("Persisted analysis progress: " + storageRecovery.analysisFrames + " frames") : ""
                                Accessible.name: "Persisted analysis progress"
                                Accessible.value: visible ? text : ""
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisCacheStatus"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.cachedResult !== undefined
                                text: visible ? (storageRecovery.cacheHit ? "Cached analysis ready" : "No cached analysis for the requested identity") : ""
                                Accessible.name: "Analysis cache status"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "Analysis cache status is unavailable."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisCacheResult"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.cacheHit
                                text: visible ? ("Cached result: " + storageRecovery.cachedResult) : ""
                                Accessible.name: "Cached analysis result"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "No completed cached analysis result is selected."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap; elide: Text.ElideRight
                            }
                            Label {
                                objectName: "analysisWorkerResult"
                                visible: window.analysisResultMatchesSelection && window.analysisJobBridge.result !== ""
                                text: visible ? ("Worker result: " + window.analysisJobBridge.result) : ""
                                Accessible.name: "Worker analysis result"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "No worker analysis result is available."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap; elide: Text.ElideRight
                            }
                            Label {
                                objectName: "analysisQueueOutcome"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.status !== undefined
                                text: visible ? ("Analysis queue: " + window.analysisJobBridge.status
                                      + (window.analysisJobBridge.canRetry ? " · retry available" : "")) : ""
                                Accessible.name: "Analysis queue outcome"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "Analysis queue outcome is unavailable."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisRecoveryGuidance"
                                visible: typeof storageRecovery !== "undefined"
                                text: visible && storageRecovery.status.indexOf("error") >= 0 ? "Analysis recovery unavailable: check storage configuration." : visible && storageRecovery.recoveryAvailable ? "Analysis recovery available: retryable persisted work is waiting." : "Analysis recovery is clear."
                                Accessible.name: "Analysis recovery guidance"
                                Accessible.value: text
                                Accessible.description: text
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "storageTransitionStatus"
                                visible: typeof storageRecovery !== "undefined" && (storageRecovery.status === "Opening storage" || storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0)
                                text: visible ? ("Storage transition: " + storageRecovery.status + ". Persisted recovery controls were refreshed for this database state.") : ""
                                Accessible.name: "Storage transition status"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "No storage transition is active."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisRetryOutcome"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.recoveryAvailable
                                text: visible ? ("Persisted retry outcome: " + storageRecovery.retryableRunIds.length + " retryable run(s)") : ""
                                Accessible.name: "Persisted retry outcome"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "No persisted retryable analysis runs are available."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisStaleState"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.analysisStatus !== undefined && (!window.analysisResultMatchesSelection || storageRecovery.analysisStatus === "Analysis cancelled" || storageRecovery.analysisStatus === "Analysis failed")
                                text: visible && !window.analysisResultMatchesSelection ? "Prior analysis result hidden because the selected source changed; analyze this source to publish fresh evidence." : visible && (storageRecovery.analysisStatus === "Analysis cancelled" || storageRecovery.analysisStatus === "Analysis failed") ? "Prior analysis result cleared; retry requires fresh source evidence." : ""
                                Accessible.name: "Analysis stale-result status"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible ? text : "No stale analysis result is present."
                                Accessible.role: Accessible.Status
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisRecoveryTransition"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.analysisStatus !== undefined
                                text: visible && (storageRecovery.analysisStatus === "Analysis queued" || storageRecovery.analysisStatus === "Analysis running") ? "Recovery transition active; previous source labels are not current." : ""
                                Accessible.name: "Analysis recovery transition"
                                Accessible.description: visible ? text : "No source or endpoint recovery transition is active."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisCompletionResult"
                                visible: window.analysisResultMatchesSelection && typeof storageRecovery !== "undefined" && storageRecovery.analysisResult !== ""
                                text: visible ? ("Completed analysis result: " + storageRecovery.analysisResult) : ""
                                Accessible.name: "Completed analysis result"
                                Accessible.description: visible ? text : "No completed analysis result is available."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap; elide: Text.ElideRight
                            }
                            RowLayout {
                                objectName: "analysisLifecycleControls"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.analysisStatus !== undefined
                                Label { text: visible ? storageRecovery.analysisStatus : ""; Accessible.name: "Analysis lifecycle status"; Layout.fillWidth: true }
                                Button { id: cancelAdmittedAnalysis; objectName: "cancelAdmittedAnalysis"; text: "Cancel analysis"; activeFocusOnTab: true; visible: parent.visible && storageRecovery.cancelAnalysis !== undefined; enabled: visible && (storageRecovery.analysisStatus === "Analysis queued" || storageRecovery.analysisStatus === "Analysis running"); onClicked: { storageRecovery.cancelAnalysis(); window.clearVisualizationForCaptureState("Analysis cancelled; previous visualization evidence is stale until a replacement accepted snapshot."); window.announceVisualizationRecovery("Analysis cancelled; waiting for a replacement accepted snapshot."); cancelAdmittedAnalysis.forceActiveFocus() } Accessible.name: "Cancel admitted analysis"; Accessible.value: enabled ? "available; " + storageRecovery.analysisStatus : "unavailable; " + storageRecovery.analysisStatus; Accessible.description: "Cancel and keep keyboard focus on the analysis recovery controls." }
                                Button { id: retryAdmittedAnalysis; objectName: "retryAdmittedAnalysis"; text: "Retry analysis"; activeFocusOnTab: true; visible: parent.visible && storageRecovery.retryAnalysis !== undefined; enabled: visible && storageRecovery.analysisRetryable; onClicked: { window.retryVisualizationAnalysis(); retryAdmittedAnalysis.forceActiveFocus() } Accessible.name: "Retry admitted analysis"; Accessible.value: enabled ? "available; retryable" : "unavailable; " + storageRecovery.analysisStatus; Accessible.description: "Retry admitted with fresh evidence; stale result cleared and focus restored after acceptance." }
                            }
                            RowLayout {
                                objectName: "analysisCacheLookupControls"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.findCached !== undefined
                                Layout.fillWidth: true
                                spacing: 6
                                TextField { id: cacheContent; objectName: "analysisCacheContent"; placeholderText: "content identity"; onTextChanged: window.analysisCacheIdentityDirty = true; Accessible.name: "Analysis cache content identity"; Accessible.description: "Content identity used to look up or invalidate a completed analysis cache entry."; Layout.fillWidth: true }
                                TextField { id: cacheEngine; objectName: "analysisCacheEngine"; placeholderText: "engine"; onTextChanged: window.analysisCacheIdentityDirty = true; Accessible.name: "Analysis cache engine"; Accessible.description: "Analysis engine identity used for the cache lookup."; Layout.fillWidth: true }
                                TextField { id: cacheConfig; objectName: "analysisCacheConfig"; placeholderText: "configuration"; onTextChanged: window.analysisCacheIdentityDirty = true; Accessible.name: "Analysis cache configuration"; Accessible.description: "Analysis configuration identity used for the cache lookup."; Layout.fillWidth: true }
                                TextField { id: cacheMode; objectName: "analysisCacheMode"; placeholderText: "offline/live"; onTextChanged: window.analysisCacheIdentityDirty = true; validator: RegularExpressionValidator { regularExpression: /^(offline|live)$/ } Accessible.name: "Analysis cache mode"; Accessible.value: acceptableInput ? text : "invalid; use offline or live"; Accessible.description: "Cache mode identity. Enter exactly offline or live."; Layout.preferredWidth: 100 }
                                Button { objectName: "clearAnalysisCacheIdentity"; text: "Clear identity"; enabled: cacheContent.text !== "" || cacheEngine.text !== "" || cacheConfig.text !== "" || cacheMode.text !== ""; Accessible.name: "Clear analysis cache identity"; Accessible.value: enabled ? "available; clears the cached identity" : "already clear"; Accessible.description: "Clear all cache identity fields and require a fresh lookup."; onClicked: { cacheContent.clear(); cacheEngine.clear(); cacheConfig.clear(); cacheMode.clear(); window.analysisCacheIdentityDirty = true; window.announceVisualizationRecovery("Analysis cache identity cleared; run Check cache before using cache actions again."); clearAnalysisCacheIdentity.forceActiveFocus() } }
                                Button {
                                    objectName: "lookupAnalysisCache"
                                    text: "Check cache"
                                    enabled: window.analysisCacheIdentityReady && typeof storageRecovery !== "undefined" && storageRecovery.storageOpen === true
                                    Accessible.name: "Check analysis cache"
                                    Accessible.value: enabled ? "ready; identity fields complete" : "unavailable; enter content, engine, configuration, and mode"
                                    Accessible.description: "Look up a completed result without starting decoder or analysis work."
                                    onClicked: { storageRecovery.findCached(cacheContent.text, cacheEngine.text, cacheConfig.text, cacheMode.text); window.analysisCacheIdentityDirty = false; window.announceVisualizationRecovery("Cache lookup requested for the selected analysis identity."); lookupAnalysisCache.forceActiveFocus() }
                                }
                                Button {
                                    objectName: "invalidateAnalysisCache"
                                    text: "Forget cached result"
                                    enabled: window.analysisCacheIdentityReady && !window.analysisCacheIdentityDirty && typeof storageRecovery !== "undefined" && storageRecovery.storageOpen === true && storageRecovery.cacheHit === true
                                    Accessible.name: "Forget cached analysis"
                                    Accessible.value: enabled ? "available; completed cache hit" : "unavailable; run Check cache and confirm a hit"
                                    Accessible.description: enabled ? "Remove this completed cache identity so the next analysis runs again. Source files are not changed." : "Run Check cache and confirm a completed cache hit before forgetting an identity."
                                    onClicked: { invalidateCacheDialog.open(); window.announceVisualizationRecovery("Confirm forgetting the cached result; source audio remains unchanged.") }
                                }
                                Button {
                                    objectName: "reanalyzeInvalidatedCache"
                                    text: "Run reanalysis"
                                    visible: storageRecovery.analysisStatus === "Analysis idle · reanalysis required"
                                    enabled: visible && window.analysisCacheIdentityReady && typeof storageRecovery !== "undefined" && storageRecovery.storageOpen === true && window.selectedAudioUrl.toLocalFile() !== ""
                                    Accessible.name: "Run fresh analysis"
                                    Accessible.value: enabled ? "available; reanalysis required" : "unavailable; select a source and complete cache identity"
                                    Accessible.description: "Invalidate this identity and admit fresh analysis work to the queue."
                                    onClicked: { reanalyzeCacheDialog.open(); window.announceVisualizationRecovery("Confirm fresh analysis for the selected cache identity; source audio remains unchanged.") }
                                }
                            }
                            Dialog {
                                id: reanalyzeCacheDialog
                                objectName: "reanalyzeCacheDialog"
                                title: "Run fresh analysis?"
                                modal: true
                                standardButtons: Dialog.Cancel | Dialog.Ok
                                Accessible.name: "Run fresh analysis confirmation"
                                Accessible.description: "Confirm invalidating this cache identity and queueing fresh analysis. Source audio is not changed."
                                contentItem: Label { text: "This invalidates the selected cached identity and queues fresh analysis. The source audio file is not changed."; wrapMode: Text.WordWrap; Accessible.role: Accessible.StaticText }
                                onOpened: { window.announceVisualizationRecovery("Confirm fresh analysis; source audio remains unchanged."); standardButton(Dialog.Cancel).forceActiveFocus() }
                                onAccepted: { storageRecovery.reanalyze("reanalyze-" + cacheContent.text, window.selectedAudioUrl.toLocalFile(), cacheContent.text, cacheEngine.text, cacheConfig.text, cacheMode.text); window.announceVisualizationRecovery("Fresh analysis queued; source audio was not changed."); reanalyzeCacheDialog.close(); reanalyzeInvalidatedCache.forceActiveFocus() }
                                onRejected: reanalyzeInvalidatedCache.forceActiveFocus()
                            }
                            Dialog {
                                id: invalidateCacheDialog
                                objectName: "invalidateCacheDialog"
                                title: "Forget cached analysis?"
                                modal: true
                                standardButtons: Dialog.Cancel | Dialog.Ok
                                Accessible.name: "Forget cached analysis confirmation"
                                Accessible.description: "Confirm removal of the selected completed cache identity. Source audio is not modified."
                                contentItem: Label { text: "This removes only the completed cached result for the entered identity. The source audio file is not changed."; wrapMode: Text.WordWrap; Accessible.role: Accessible.StaticText }
                                onOpened: { window.announceVisualizationRecovery("Confirm forgetting the cached result; source audio remains unchanged."); standardButton(Dialog.Cancel).forceActiveFocus() }
                                onAccepted: { storageRecovery.invalidateCached(cacheContent.text, cacheEngine.text, cacheConfig.text, cacheMode.text); window.announceVisualizationRecovery("Cached analysis identity forgotten; source audio was not changed."); invalidateCacheDialog.close(); invalidateAnalysisCache.forceActiveFocus() }
                                onRejected: invalidateAnalysisCache.forceActiveFocus()
                            }
                            ComboBox {
                                objectName: "persistedRecoveryRuns"
                                id: persistedRecoveryRuns
                                activeFocusOnTab: true
                                visible: typeof storageRecovery !== "undefined"
                                model: visible ? storageRecovery.retryableRunIds : []
                                enabled: visible && count > 0 && !window.storageReopenPending
                                Accessible.name: "Persisted retryable analysis runs (" + count + ")"
                                Accessible.role: Accessible.ComboBox
                                Accessible.value: enabled ? "available; " + storageRecovery.selectedRunId : "unavailable; " + (storageRecovery && storageRecovery.selectedRunId !== "" ? storageRecovery.analysisStatus : "no persisted run selected")
                                Accessible.description: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                    ? "Selected persisted recovery run " + storageRecovery.selectedRunId + ". " + (storageRecovery.resetStatus !== undefined && storageRecovery.resetStatus !== "" ? "Recovery status: " + storageRecovery.resetStatus + ". " : "") + "Retry and discard controls are available when the run is retryable. Press Tab to reach them."
                                    : (count > 0 ? "Choose one of " + count + " saved analysis runs to recover. Press Tab to reach retry and discard actions after selection." : "No saved analysis runs available.")
                                ToolTip.text: count > 0
                                    ? "Choose a saved analysis run to recover" + (typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== "" ? ". Selected: " + storageRecovery.selectedRunId : "")
                                    : "No saved analysis runs available"
                                onActivated: { storageRecovery.selectRun(currentText); window.persistedRecoverySelectionActivated = true; window.announceVisualizationRecovery("Selected persisted recovery run " + currentText + ". Press Tab to reach retry and discard controls when the run is retryable."); persistedRecoveryRuns.forceActiveFocus() }
                            }
                            Label {
                                objectName: "persistedRecoverySelectionStatus"
                                visible: typeof storageRecovery !== "undefined"
                                text: visible && storageRecovery.selectedRunId !== "" ? ("Recovery selection: " + storageRecovery.selectedRunId + (storageRecovery.analysisRetryable ? " · retry available" : " · waiting for terminal recovery state")) : "No persisted recovery run selected"
                                Accessible.name: "Persisted recovery selection status"
                                Accessible.value: visible ? text : ""
                                Accessible.role: Accessible.Status
                                Accessible.description: visible && storageRecovery.selectedRunId !== "" ? text + ". Retry and discard actions follow this status; source audio is never modified." : "Select a saved run to expose its recovery actions."
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "persistedRecoveryRunDetails"
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.retryableRunDetails !== undefined && storageRecovery.retryableRunDetails.length > 0
                                text: visible ? storageRecovery.retryableRunDetails.join("\n") : ""
                                Accessible.name: "Persisted retryable analysis identities"
                                Accessible.value: visible ? text : ""
                                Accessible.description: visible && typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                    ? "Diagnostic details for selected persisted recovery run " + storageRecovery.selectedRunId + ". Includes content identity, source path, and decoder snapshot."
                                    : "Run ID, content identity, source path, and decoder snapshot for each saved retryable analysis."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap; elide: Text.ElideRight
                            }
                            Label {
                                objectName: "persistedRecoveryActionStatus"
                                visible: typeof storageRecovery !== "undefined"
                                text: visible && storageRecovery.selectedRunId !== ""
                                    ? ("Recovery actions: " + (storageRecovery.analysisRetryable ? "retry available" : "retry unavailable")
                                       + (storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0 ? " · reopen storage" : "")
                                       + (storageRecovery.analysisStatus === "Analysis queued" || storageRecovery.analysisStatus === "Analysis running" ? " · active work" : ""))
                                    : "Select a saved run to expose recovery actions"
                                Accessible.name: "Persisted recovery action status"
                                Accessible.value: visible ? text : ""
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? text + ". Retry and discard never modify source audio." : "No persisted recovery action is selected."
                                ToolTip.text: "Current persisted recovery action availability"
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            Button {
                                objectName: "discardPersistedRecovery"
                                id: discardPersistedRecoveryButton
                                activeFocusOnTab: true
                                visible: typeof storageRecovery !== "undefined"
                                enabled: visible && !window.storageReopenPending && storageRecovery.selectedRunId !== "" && storageRecovery.status !== "Storage closed" && storageRecovery.status.indexOf("Storage error:") !== 0 && storageRecovery.analysisStatus !== "Analysis complete" && storageRecovery.analysisStatus !== "Analysis queued" && storageRecovery.analysisStatus !== "Analysis running"
                                text: "Discard saved run"
                                Accessible.name: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                    ? "Discard saved retryable analysis run " + storageRecovery.selectedRunId
                                    : "Discard saved retryable analysis run"
                                Accessible.value: enabled ? "available; " + storageRecovery.selectedRunId : "unavailable; " + (storageRecovery && storageRecovery.selectedRunId !== "" ? storageRecovery.analysisStatus : "no persisted run selected")
                                ToolTip.text: "Discard the selected saved analysis run"
                                Accessible.description: typeof storageRecovery !== "undefined" && (storageRecovery.analysisStatus === "Analysis queued" || storageRecovery.analysisStatus === "Analysis running") ? "The selected run is active; discard is unavailable until terminal resolution." : typeof storageRecovery !== "undefined" && storageRecovery.analysisStatus === "Analysis complete" ? "The selected analysis is complete; no persisted retry record can be discarded." : typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                    ? "Open confirmation to remove retry record " + storageRecovery.selectedRunId + ". The source audio file is never changed."
                                    : "Open confirmation to remove the selected retryable run record. The source audio file is never changed."
                                onClicked: { discardRecoveryDialog.open(); window.announceVisualizationRecovery("Confirm discarding the selected saved recovery run; source audio remains unchanged.") }
                            }
                            Button {
                                objectName: "retryPersistedRecovery"
                                activeFocusOnTab: true
                                visible: typeof storageRecovery !== "undefined"
                                enabled: visible && !window.storageReopenPending && storageRecovery.selectedRunId !== "" && storageRecovery.status !== "Storage closed" && storageRecovery.status.indexOf("Storage error:") !== 0 && storageRecovery.analysisRetryable && storageRecovery.analysisStatus !== "Analysis queued" && storageRecovery.analysisStatus !== "Analysis running"
                                text: "Retry selected run"
                                Accessible.value: enabled ? "available; " + storageRecovery.selectedRunId : "unavailable; " + (storageRecovery && storageRecovery.selectedRunId !== "" ? storageRecovery.analysisStatus : "no persisted run selected")
                                onClicked: { const runId = storageRecovery.selectedRunId; if (window.storageReopenPending || storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0) { window.announceVisualizationRecovery("Storage became unavailable; retry of " + runId + " was canceled. Reopen storage to refresh recovery runs."); reopenStorage.forceActiveFocus() } else { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.retrySelectedRun(); window.clearVisualizationForCaptureState("Persisted recovery retry admitted; previous visualization evidence is stale until completion."); window.announceVisualizationRecovery("Persisted recovery retry admitted for " + runId + "; reset status cleared."); retryPersistedRecovery.forceActiveFocus() } }
                                Accessible.name: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                    ? "Retry selected persisted analysis run " + storageRecovery.selectedRunId
                                    : "Retry selected persisted analysis run"
                                Accessible.description: typeof storageRecovery !== "undefined" && (storageRecovery.analysisStatus === "Analysis queued" || storageRecovery.analysisStatus === "Analysis running") ? "The selected run is active; retry is unavailable until it reaches a terminal state." : typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                    ? "Retry persisted run " + storageRecovery.selectedRunId + " when recovery state confirms retryable work. A persisted discard status clears when retry is admitted."
                                    : "Retry the selected persisted run only when recovery state confirms retryable work. A persisted discard status clears when retry is admitted."
                            }
                            Button {
                                objectName: "discardAllPersistedRecovery"
                                activeFocusOnTab: true
                                visible: typeof storageRecovery !== "undefined" && storageRecovery.discardRuns !== undefined
                                enabled: visible && !window.storageReopenPending && storageRecovery.recoveryAvailable
                                text: "Discard all saved runs"
                                Accessible.name: "Discard all saved retryable analysis runs"
                                Accessible.value: enabled ? "available; " + storageRecovery.retryableRunIds.length + " runs" : "unavailable; no retryable runs"
                                Accessible.description: "Remove every persisted retry record currently shown. Source audio files are never changed."
                                onClicked: { discardAllRecoveryDialog.open(); window.announceVisualizationRecovery("Confirm discarding all saved recovery runs; source audio remains unchanged.") }
                            }
                            Dialog {
                                id: discardAllRecoveryDialog
                                objectName: "discardAllRecoveryDialog"
                                title: "Discard all saved analysis runs?"
                                modal: true
                                standardButtons: Dialog.Ok | Dialog.Cancel
                                Accessible.name: "Discard all saved recovery runs confirmation"
                                Accessible.description: "Confirm removing every persisted retry record currently shown. Source audio files are never changed. Cancel is safe."
                                contentItem: Label {
                                    text: "This removes " + (typeof storageRecovery !== "undefined" ? storageRecovery.retryableRunIds.length : 0) + " saved retry record(s). Source audio files are not changed."
                                    wrapMode: Text.WordWrap
                                }
                                onAccepted: { if (window.storageReopenPending || storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0) { window.announceVisualizationRecovery("Storage became unavailable; bulk discard was canceled. Reopen storage to refresh recovery runs."); reopenStorage.forceActiveFocus() } else { storageRecovery.discardRuns(storageRecovery.retryableRunIds); window.announceVisualizationRecovery("All saved recovery runs discarded."); discardAllPersistedRecovery.forceActiveFocus() } }
                                onRejected: discardAllPersistedRecovery.forceActiveFocus()
                            }
                            Label { objectName: "selectedPersistedRecoveryResetStatus"; visible: typeof storageRecovery !== "undefined" && storageRecovery.resetStatus !== undefined && storageRecovery.resetStatus !== ""; text: visible ? ("Selected recovery: " + storageRecovery.resetStatus) : ""; Accessible.name: "Selected persisted recovery status"; Accessible.description: visible ? text : "No selected persisted recovery reset has been performed."; Accessible.role: Accessible.Status; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            Dialog {
                                id: discardRecoveryDialog
                                objectName: "discardRecoveryDialog"
                                title: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                    ? "Discard saved analysis run " + storageRecovery.selectedRunId + "?"
                                    : "Discard saved analysis run?"
                                modal: true
                                standardButtons: Dialog.Ok | Dialog.Cancel
                                Accessible.role: Accessible.Dialog
                                Accessible.name: "Discard saved analysis run confirmation"
                                Accessible.description: "Confirm to remove only the selected retry record. Cancel is focused first; press Tab to reach the destructive OK action, Enter confirms, and Escape cancels. The source audio file is never changed. Cancel returns focus to the discard control."
                                Accessible.value: typeof storageRecovery !== "undefined" ? storageRecovery.selectedRunId : ""
                                contentItem: Label {
                                    text: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                        ? "This removes only retry record " + storageRecovery.selectedRunId + ". The source audio file is not changed."
                                        : "This removes only the retry record. The source audio file is not changed."
                                    wrapMode: Text.WordWrap
                                    Accessible.name: typeof storageRecovery !== "undefined" && storageRecovery.selectedRunId !== ""
                                        ? "Discard retryable run " + storageRecovery.selectedRunId + " confirmation"
                                        : "Discard retryable run confirmation"
                                    Accessible.description: "The selected retry record will be removed; source audio remains unchanged."
                                    Accessible.role: Accessible.StaticText
                                }
                    Label {
                        objectName: "visualizationLayoutState"
                        text: window.compactHeader ? "Compact visualization layout" : "Expanded visualization layout"
                        Accessible.name: "Visualization layout: " + text
                        Accessible.value: text + "; live overlay evidence stale"
                        Accessible.role: Accessible.Status
                        Accessible.description: "Responsive visualization layout selected from the current control-window width."
                        ToolTip.text: text
                    }
                                onOpened: { window.announceVisualizationRecovery("Confirm discard for persisted recovery run " + storageRecovery.selectedRunId + ". Source audio will remain unchanged."); standardButton(Dialog.Cancel).forceActiveFocus() }
                                onAccepted: { const runId = storageRecovery.selectedRunId; if (window.storageReopenPending || storageRecovery.status === "Storage closed" || storageRecovery.status.indexOf("Storage error:") === 0) { window.announceVisualizationRecovery("Storage became unavailable; discard of " + runId + " was canceled. Reopen storage to refresh recovery runs."); reopenStorage.forceActiveFocus() } else { window.overlayPersistedDiagnosticDismissed = true; storageRecovery.discardSelectedRun(); window.clearVisualizationForCaptureState("Persisted recovery discard requested; previous visualization evidence is stale until a new accepted result."); window.announceVisualizationRecovery("Persisted recovery discard requested for " + runId + "; the status will report if active work prevents removal."); discardPersistedRecoveryButton.forceActiveFocus() } }
                                onRejected: { const runId = storageRecovery.selectedRunId; window.announceVisualizationRecovery("Persisted recovery discard canceled for " + runId + "; selected run was kept."); discardPersistedRecoveryButton.forceActiveFocus() }
                            }
                            Label {
                                objectName: "analysisBatchActionStatus"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchAcceptedCount > 0
                                text: visible ? ("Batch actions: "
                                      + ((window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running") ? "cancel available" : "cancel unavailable")
                                      + ((window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled") ? " · retry or discard available" : "")) : ""
                                Accessible.name: "Analysis batch action status: " + text
                                Accessible.value: visible ? text : ""
                                Accessible.role: Accessible.Status
                                Accessible.description: visible ? text + ". " + window.analysisJobBridge.batchAcceptedCount + " accepted item(s) are represented; actions retain source audio and return focus after completion." : "No analysis batch actions are available."
                                ToolTip.text: "Current analysis batch action availability"
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            Button {
                                objectName: "retryAnalysisBatch"
                                id: retryAnalysisBatchButton
                                activeFocusOnTab: true
                                text: "Retry batch"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.retryBatch !== undefined && window.analysisJobBridge.batchAcceptedCount > 0
                                enabled: visible && (window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled")
                                Accessible.name: "Retry analysis batch"
                                Accessible.value: visible ? (enabled ? "available; " : "unavailable; ") + window.analysisJobBridge.status + "; accepted items " + window.analysisJobBridge.batchAcceptedCount : ""
                                Accessible.description: visible ? "Retry cancelled or failed analysis items retained in the current batch. Current state: " + window.analysisJobBridge.status + "; accepted items: " + window.analysisJobBridge.batchAcceptedCount + "." : "Retry batch is unavailable."
                                ToolTip.text: Accessible.description
                                onClicked: { if (!window.analysisJobBridge || (window.analysisJobBridge.status !== "Failed" && window.analysisJobBridge.status !== "Cancelled")) { window.announceVisualizationRecovery("Analysis batch is no longer retryable; no batch retry was dispatched."); retryAnalysisBatchButton.forceActiveFocus() } else { const accepted = window.analysisJobBridge.retryBatch(); if (accepted) window.clearVisualizationForCaptureState("Analysis batch retry queued; previous visualization evidence is stale until completion."); window.announceVisualizationRecovery(accepted ? "Analysis batch retry queued; focus will return after a fresh accepted result." : "Analysis batch retry was rejected; the retained operations are no longer retryable."); retryAnalysisBatchButton.forceActiveFocus() } analysisBatchStatus.forceActiveFocus() }
                            }
                            Button {
                                objectName: "discardAnalysisBatch"
                                id: discardAnalysisBatchButton
                                activeFocusOnTab: true
                                text: "Discard batch"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.discardBatch !== undefined && window.analysisJobBridge.batchAcceptedCount > 0
                                enabled: visible && (window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled")
                                Accessible.name: "Discard analysis batch"
                                Accessible.value: visible ? (enabled ? "available; " : "unavailable; ") + window.analysisJobBridge.status + "; accepted items " + window.analysisJobBridge.batchAcceptedCount : ""
                                Accessible.description: visible ? "Clear the terminal batch recovery state without changing source audio. Current state: " + window.analysisJobBridge.status + "; accepted items: " + window.analysisJobBridge.batchAcceptedCount + ". Confirmation is required; focus returns here after the dialog." : "Discard batch is unavailable."
                                ToolTip.text: Accessible.description
                                onClicked: discardAnalysisBatchDialog.open()
                            }
                            Dialog {
                                id: discardAnalysisBatchDialog
                                objectName: "discardAnalysisBatchDialog"
                                modal: true
                                title: "Discard analysis batch?"
                                standardButtons: Dialog.Ok | Dialog.Cancel
                                Accessible.role: Accessible.Dialog
                                Accessible.name: "Discard analysis batch confirmation"
                                Accessible.description: "Clear the retained batch recovery state without changing source audio. Cancel is focused first; Tab reaches the destructive OK action."
                                contentItem: Label { text: "This clears the retained batch recovery state. Source audio is not changed."; wrapMode: Text.WordWrap; Accessible.name: "Discard analysis batch confirmation"; Accessible.role: Accessible.StaticText }
                                onOpened: { window.announceVisualizationRecovery("Confirm discard of the analysis batch; source audio will remain unchanged."); standardButton(Dialog.Cancel).forceActiveFocus() }
                                onAccepted: { if (!window.analysisJobBridge || (window.analysisJobBridge.status !== "Failed" && window.analysisJobBridge.status !== "Cancelled")) { window.announceVisualizationRecovery("Analysis batch is no longer discardable; no batch recovery was discarded."); retryAnalysisBatchButton.forceActiveFocus() } else { window.analysisJobBridge.discardBatch(); window.clearVisualizationForCaptureState("Analysis batch recovery discarded; previous visualization evidence is stale until a new accepted result."); window.announceVisualizationRecovery("Analysis batch recovery discarded; focus remains on the recovery controls."); discardAnalysisBatchButton.forceActiveFocus() } analysisBatchStatus.forceActiveFocus() }
                                onRejected: discardAnalysisBatchButton.forceActiveFocus()
                            }
                            Button {
                                objectName: "cancelAnalysisBatch"
                                activeFocusOnTab: true
                                text: "Cancel batch"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.cancelBatch !== undefined && window.analysisJobBridge.batchAcceptedCount > 0
                                enabled: visible && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running")
                                Accessible.name: "Cancel analysis batch"
                                Accessible.value: visible ? (enabled ? "available; " : "unavailable; ") + window.analysisJobBridge.status + "; accepted items " + window.analysisJobBridge.batchAcceptedCount : ""
                                Accessible.description: visible && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running") ? "Cancel all queued or running analysis items retained in the current batch. Current state: " + window.analysisJobBridge.status + "; accepted items: " + window.analysisJobBridge.batchAcceptedCount + "." : "Batch cancel is unavailable because the current batch is no longer active."
                                ToolTip.text: Accessible.description
                                onClicked: { if (!window.analysisJobBridge || (window.analysisJobBridge.status !== "Queued" && window.analysisJobBridge.status !== "Running")) { window.announceVisualizationRecovery("Analysis batch is no longer active; no batch cancel was dispatched."); retryAnalysisBatchButton.forceActiveFocus() } else { window.analysisJobBridge.cancelBatch(); window.clearVisualizationForCaptureState("Analysis batch cancelled; previous visualization evidence is stale until replacement analysis completes."); window.announceVisualizationRecovery("Analysis batch cancelled; waiting for a replacement accepted snapshot."); cancelAnalysisBatch.forceActiveFocus() } analysisBatchStatus.forceActiveFocus() }
                            }
                            Button {
                                id: retryAnalysis
                                text: "Retry analysis"
                                objectName: "retryAnalysis"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null
                                enabled: visible && window.analysisJobBridge.canRetry
                                Accessible.name: "Retry analysis"
                                Accessible.description: "Retry the last source-attributed analysis operation after cancellation or failure."
                                onClicked: { window.retryVisualizationAnalysis(); retryAnalysis.forceActiveFocus() }
                                ToolTip.text: Accessible.description
                            }
                            Button {
                                id: discardAnalysisButton
                                objectName: "discardAnalysis"
                                activeFocusOnTab: true
                                text: "Discard recovery"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.discard !== undefined
                                enabled: visible && window.analysisJobBridge.canRetry
                                Accessible.name: "Discard analysis recovery"
                                Accessible.description: "Clear the failed or cancelled analysis recovery state without changing source audio. Confirmation is required."
                                ToolTip.text: Accessible.description
                                onClicked: { window.announceVisualizationRecovery("Confirm discard of analysis recovery; source audio will remain unchanged."); discardAnalysisDialog.open() }
                            }
                            Dialog {
                                id: discardAnalysisDialog
                                modal: true
                                title: "Discard analysis recovery?"
                                standardButtons: Dialog.Ok | Dialog.Cancel
                                contentItem: Label { text: "This clears the retained analysis recovery state. Source audio is not changed."; wrapMode: Text.WordWrap; Accessible.name: "Discard analysis recovery confirmation"; Accessible.role: Accessible.StaticText }
                                onOpened: standardButton(Dialog.Cancel).forceActiveFocus()
                                onRejected: discardAnalysisButton.forceActiveFocus()
                                onAccepted: { if (!window.analysisJobBridge || !window.analysisJobBridge.canRetry) { window.announceVisualizationRecovery("Analysis recovery changed before discard; no recovery was discarded. Retry remains available when the worker reaches a terminal state."); retryAnalysis.forceActiveFocus() } else { window.analysisJobBridge.discard(); window.clearVisualizationForCaptureState("Analysis recovery discarded; previous visualization evidence is stale until a new accepted result."); window.announceVisualizationRecovery("Analysis recovery discarded; focus remains on the recovery controls."); retryAnalysis.forceActiveFocus() } }
                            }
                            Button {
                                objectName: "cancelAnalysis"
                                text: "Cancel analysis"
                                activeFocusOnTab: true
                                visible: window.analysisJobBridge !== null
                                enabled: visible && window.analysisJobBridge.canCancel
                                Accessible.name: "Cancel analysis"
                                Accessible.description: "Cancel the active source-attributed analysis operation."
                                ToolTip.text: Accessible.description
                                onClicked: { if (!window.analysisJobBridge || !window.analysisJobBridge.canCancel) { window.announceVisualizationRecovery("Analysis is no longer active; cancel was not dispatched."); retryAnalysis.forceActiveFocus() } else { window.analysisJobBridge.cancel(); window.announceVisualizationRecovery("Analysis cancelled; waiting for a replacement accepted snapshot."); cancelAnalysis.forceActiveFocus() } }
                            }
                            BusyIndicator {
                                objectName: "analysisWaitingIndicator"
                                running: window.selectedName !== "" && window.analysisJobBridge && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running")
                                visible: running
                                Accessible.name: "Analysis waiting indicator"
                                Accessible.description: "Analysis is waiting for the first observed frame; progress percentage is not available yet."
                                ToolTip.text: "Analysis is waiting for the first observed frame"
                            }
                        }
                    }
                }
            }
        }
        Item {
            objectName: "djPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16
                Label { text: "DJ"; font.pixelSize: 28; font.bold: true; Accessible.name: "DJ monitoring"; Accessible.role: Accessible.Heading; ToolTip.text: "Live output monitoring and visualization" }
                RowLayout {
                    objectName: "monitoringControls"
                    Accessible.name: "Live monitoring controls"
                    Accessible.value: window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable"
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Start, stop, or retry live output monitoring."
                    ToolTip.text: "Live monitoring controls; current state is announced when available"
                    Button { text: window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" ? "Stop monitoring" : (window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "Retry monitoring" : (window.captureMonitorBridge && window.captureMonitorBridge.status === "Starting" ? "Starting…" : (window.captureMonitorBridge && window.captureMonitorBridge.status === "Reacquiring" ? "Reacquiring…" : "Start monitoring"))); objectName: "toggleMonitoring"; activeFocusOnTab: true; enabled: window.captureMonitorBridge !== null && window.captureMonitorBridge.status !== "Starting" && window.captureMonitorBridge.status !== "Reacquiring"; Accessible.name: text; Accessible.role: Accessible.Button; Accessible.description: window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "Retry monitoring after a capture error." : "Start or stop live output monitoring."; ToolTip.text: window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" ? "Stop live output monitoring" : window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "Retry live output monitoring" : "Start live output monitoring"; onClicked: window.captureMonitorBridge.status === "Monitoring" ? window.captureMonitorBridge.stop() : window.captureMonitorBridge.start() }
                    Label { text: window.captureMonitorBridge ? (window.captureMonitorBridge.status === "Error" ? "Capture error — retry monitoring" : "Capture: " + window.captureMonitorBridge.status) : "Capture unavailable"; objectName: "captureStatus"; Accessible.name: "Capture monitor status: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text + ". " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready."); ToolTip.text: "Current capture monitor status and recovery state" }
                ColumnLayout {
                    objectName: "monitoringStatusGroup"
                    Layout.fillWidth: true
                    Accessible.name: "Live monitoring status and guidance"
                    Accessible.value: window.liveBridge.status
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Current output monitoring state and the next action needed to produce live estimates. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")
                    ToolTip.text: "Live monitoring state and next action; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Label { text: "Live output monitoring: " + window.liveBridge.status; font.pixelSize: 18; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Live output monitoring: " + text; Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready"); Accessible.role: Accessible.StaticText; Accessible.description: text + ". " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready."); ToolTip.text: "Current live output monitoring state; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready") }
                    Label { text: window.liveBridge.status === "Stopped" ? "Start monitoring from the DJ controls when an output source is available." : "Live estimates update below as output audio is captured."; objectName: "monitoringGuidance"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Monitoring guidance"; Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready"); Accessible.role: Accessible.StaticText; Accessible.description: text + " " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready."); ToolTip.text: "Monitoring guidance and next action; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready") }
                }
                RowLayout {
                    objectName: "visualizationModeControls"
                    Accessible.name: "Visualization mode controls, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Accessible.value: visualizationCanvas.viewMode + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Choose waveform or frequency-domain FFT visualization; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                    ToolTip.text: "Visualization mode controls; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending" : "overlay is ready") + "; current mode is announced when available"
                    Button {
                        id: waveformView
                        activeFocusOnTab: true
                        text: "Waveform"; objectName: "waveformView"; checkable: true; checked: visualizationCanvas.viewMode === "waveform"; Accessible.value: (checked ? "selected" : "not selected") + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready"); Accessible.role: Accessible.RadioButton
                        Accessible.name: "Waveform view, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready"); Accessible.description: (checked ? "Waveform view is selected; show the selectable time-domain waveform." : "Show the selectable time-domain waveform.") + (window.overlayDisplayRecoveryPending ? " Overlay recovery is pending." : " Overlay is ready.")
                        ToolTip.text: "Show the time-domain waveform (Ctrl+1); " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                        onClicked: visualizationCanvas.viewMode = "waveform"
                        Keys.onRightPressed: { visualizationCanvas.viewMode = "spectrum"; spectrumView.forceActiveFocus(); event.accepted = true }
                        Keys.onLeftPressed: { visualizationCanvas.viewMode = "waveform"; event.accepted = true }
                        Keys.onEscapePressed: { visualizationCanvas.forceActiveFocus(); event.accepted = true }
                        background: Rectangle {
                            color: waveformView.checked ? Qt.darker(window.themeAccentColors[window.selectedThemeIndex], 1.8) : window.themeBackgrounds[window.selectedThemeIndex]
                            border.width: waveformView.activeFocus ? 3 : 1
                            border.color: waveformView.activeFocus ? window.themeAccentColors[window.selectedThemeIndex] : window.themeForegrounds[window.selectedThemeIndex]
                            radius: 6
                        }
                    }
                    Button {
                        id: spectrumView
                        activeFocusOnTab: true
                        text: "FFT"; objectName: "spectrumView"; checkable: true; checked: visualizationCanvas.viewMode === "spectrum"; Accessible.value: (checked ? "selected" : "not selected") + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready"); Accessible.role: Accessible.RadioButton
                        Accessible.name: "FFT view, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready"); Accessible.description: (checked ? "FFT view is selected; show the selectable frequency-domain FFT." : "Show the selectable frequency-domain FFT.") + (window.overlayDisplayRecoveryPending ? " Overlay recovery is pending." : " Overlay is ready.")
                        ToolTip.text: "Show the frequency-domain FFT (Ctrl+2); " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                        onClicked: visualizationCanvas.viewMode = "spectrum"
                        Keys.onLeftPressed: { visualizationCanvas.viewMode = "waveform"; waveformView.forceActiveFocus(); event.accepted = true }
                        Keys.onRightPressed: { visualizationCanvas.viewMode = "spectrum"; event.accepted = true }
                        Keys.onEscapePressed: { visualizationCanvas.forceActiveFocus(); event.accepted = true }
                        background: Rectangle {
                            color: spectrumView.checked ? Qt.darker(window.themeAccentColors[window.selectedThemeIndex], 1.8) : window.themeBackgrounds[window.selectedThemeIndex]
                            border.width: spectrumView.activeFocus ? 3 : 1
                            border.color: spectrumView.activeFocus ? window.themeAccentColors[window.selectedThemeIndex] : window.themeForegrounds[window.selectedThemeIndex]
                            radius: 6
                        }
                    }
                    Label {
                        text: visualizationCanvas.viewMode === "waveform" ? "Time domain" : "Frequency domain"
                        objectName: "visualizationModeStatus"
                        color: window.themeForegrounds[window.selectedThemeIndex]
                        font.pixelSize: window.compactHeader ? 12 : 14
                    Accessible.name: "Visualization mode, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: visualizationCanvas.viewMode
                        Accessible.role: Accessible.StaticText
                        Accessible.description: text + ". Use Left and Right arrows to switch views, or Ctrl+1 and Ctrl+2."
                    }
                }
                Canvas {
                    id: visualizationCanvas; objectName: "visualizationCanvas"
                    property string viewMode: "waveform"
                    property real selectedPosition: -1
                    property int selectedSpectrumBin: -1
                    property int renderedRevision: -1
                    property int acceptedPlaybackEpoch: -1
                    property string acceptedPlaybackSource: ""
                    property var renderedWaveform: []
                    property var renderedSpectrum: []
                    readonly property int spectrumPeakIndex: {
                        let peak = -1, value = 0
                        for (let index = 0; index < renderedSpectrum.length; ++index) {
                            const magnitude = Number(renderedSpectrum[index])
                            if (isFinite(magnitude) && magnitude > value) { value = magnitude; peak = index }
                        }
                        return peak
                    }
                    readonly property real spectrumPeakMagnitude: spectrumPeakIndex >= 0 ? Math.max(0, Number(renderedSpectrum[spectrumPeakIndex])) : 0
                    readonly property string acceptedResultIdentity: window.acceptedAnalysisIdentity
                    readonly property bool reducedMotion: window.themePreferencesBridge !== null && window.themePreferencesBridge.reduceMotion
                    readonly property bool playbackSnapshotAligned: presentationReady
                        && window.playbackBridge !== null
                        && window.playbackBridge.state !== "Stopped"
                        && window.playbackBridge.epoch >= 0
                        && window.playbackMatchesSelection
                    activeFocusOnTab: true
                    Keys.onLeftPressed: {
                        if (viewMode === "spectrum") {
                            selectedSpectrumBin = Math.max(0, (selectedSpectrumBin < 0 ? 0 : selectedSpectrumBin) - 1)
                            requestPaint(); event.accepted = true; return
                        }
                        const step = (event.modifiers & Qt.ShiftModifier) ? 0.05 : 0.01
                        selectedPosition = Math.max(0, (selectedPosition < 0 ? 0 : selectedPosition) - step)
                        requestPaint()
                        event.accepted = true
                    }
                    Keys.onRightPressed: {
                        if (viewMode === "spectrum") {
                            selectedSpectrumBin = Math.min(Math.max(0, renderedSpectrum.length - 1), (selectedSpectrumBin < 0 ? 0 : selectedSpectrumBin) + 1)
                            requestPaint(); event.accepted = true; return
                        }
                        const step = (event.modifiers & Qt.ShiftModifier) ? 0.05 : 0.01
                        selectedPosition = Math.min(1, (selectedPosition < 0 ? 0 : selectedPosition) + step)
                        requestPaint()
                        event.accepted = true
                    }
                    Keys.onHomePressed: { if (viewMode === "spectrum") selectedSpectrumBin = 0; else selectedPosition = 0; requestPaint(); event.accepted = true }
                    Keys.onEndPressed: { if (viewMode === "spectrum") selectedSpectrumBin = Math.max(0, renderedSpectrum.length - 1); else selectedPosition = 1; requestPaint(); event.accepted = true }
                    Keys.onPageUpPressed: { if (viewMode === "spectrum") selectedSpectrumBin = Math.max(0, (selectedSpectrumBin < 0 ? 0 : selectedSpectrumBin) - 10); else selectedPosition = Math.max(0, (selectedPosition < 0 ? 0 : selectedPosition) - 0.1); requestPaint(); event.accepted = true }
                    Keys.onPageDownPressed: { if (viewMode === "spectrum") selectedSpectrumBin = Math.min(Math.max(0, renderedSpectrum.length - 1), (selectedSpectrumBin < 0 ? 0 : selectedSpectrumBin) + 10); else selectedPosition = Math.min(1, (selectedPosition < 0 ? 0 : selectedPosition) + 0.1); requestPaint(); event.accepted = true }
                    Keys.onReturnPressed: { if (selectedPosition >= 0) clearVisualizationSelection.forceActiveFocus(); event.accepted = true }
                    Keys.onEnterPressed: { if (selectedPosition >= 0) clearVisualizationSelection.forceActiveFocus(); event.accepted = true }
                    Keys.onSpacePressed: { if (selectedPosition >= 0) clearVisualizationSelection.forceActiveFocus(); event.accepted = true }
                    Keys.onEscapePressed: { window.clearVisualizationPositionAndFocus(); event.accepted = true }
                    Keys.onPressed: {
                        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Left) {
                            selectedPosition = 0
                            requestPaint()
                            event.accepted = true
                        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Right) {
                            selectedPosition = 1
                            requestPaint()
                            event.accepted = true
                        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Up) {
                            viewMode = "waveform"
                            waveformView.forceActiveFocus()
                            event.accepted = true
                        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Down) {
                            viewMode = "spectrum"
                            spectrumView.forceActiveFocus()
                            event.accepted = true
                        }
                    }
                    readonly property bool presentationReady: typeof visualizationPresentation !== "undefined"
                        && window.libraryAnalysisResultReady
                        && visualizationPresentation.generation >= 0
                        && visualizationPresentation.revision >= 0
                    readonly property bool modeHasData: presentationReady
                        && (viewMode === "waveform"
                            ? renderedWaveform.length > 0
                            : renderedSpectrum.length > 0)
                    Layout.fillWidth: true; Layout.preferredHeight: window.compactHeader ? 96 : 120; Layout.minimumHeight: window.compactHeader ? 72 : 96
                    Accessible.name: "Live " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT spectrum")
                    Accessible.value: visualizationCanvas.viewMode + ", revision " + visualizationCanvas.renderedRevision + ", " + (visualizationCanvas.selectedPosition < 0 ? "no position selected" : "position " + Math.round(visualizationCanvas.selectedPosition * 100) + "%") + (visualizationCanvas.viewMode === "spectrum" && visualizationCanvas.selectedSpectrumBin >= 0 ? ", selected bin " + visualizationCanvas.selectedSpectrumBin : "") + (visualizationCanvas.viewMode === "spectrum" && visualizationCanvas.spectrumPeakIndex >= 0 ? ", peak bin " + visualizationCanvas.spectrumPeakIndex : "")
                    Accessible.role: Accessible.Graphic
                    Accessible.description: (visualizationCanvas.viewMode === "waveform"
                        ? "Bounded time-domain waveform from the current output capture."
                        : "Bounded frequency-domain FFT magnitudes from the current output capture.")
                        + " Click or scroll to select and scrub; double-click or right-click to clear; hold Shift or Ctrl while scrolling for larger steps. Use Left/Right for fine movement, Shift+Left/Shift+Right for five-percent movement, Page Up/Page Down for larger movement, Ctrl+Left/Ctrl+Right for boundary jumps, Ctrl+Up/Ctrl+Down to change mode, Home/End for boundaries, Enter or Space to reach clear, and Escape to clear. "
                        + (visualizationCanvas.selectedPosition < 0 ? "No position selected." : "Selected position " + Math.round(visualizationCanvas.selectedPosition * 100) + " percent.")
                    ToolTip.text: "Click or scroll to select a position; double-click or right-click to clear; hold Shift or Ctrl while scrolling for larger steps. Use Left/Right/Shift+Left/Shift+Right/Page Up/Page Down/Ctrl+Left/Ctrl+Right/Ctrl+Up/Ctrl+Down/Home/End to navigate, Enter or Space for clear, or Escape to clear"
                    onPaint: {
                        var ctx = getContext("2d"); ctx.reset();
                        ctx.fillStyle = window.themeBackgrounds[window.selectedThemeIndex]; ctx.fillRect(0, 0, width, height)
                        ctx.strokeStyle = window.themeForegrounds[window.selectedThemeIndex]
                        ctx.globalAlpha = 0.18; ctx.lineWidth = 1; ctx.beginPath()
                        const gridDivisions = window.compactHeader ? 2 : 4
                        for (var grid = 1; grid < gridDivisions; ++grid) {
                            var gridY = grid * height / gridDivisions
                            ctx.moveTo(0, gridY); ctx.lineTo(width, gridY)
                        }
                        ctx.stroke(); ctx.globalAlpha = 1.0
                        if (!presentationReady)
                            return
                        var wave = renderedWaveform
                        var spec = renderedSpectrum
                        if (viewMode === "waveform") {
                            ctx.strokeStyle = window.themeAccentColors[window.selectedThemeIndex]; ctx.beginPath()
                            for (var i = 0; i < wave.length; ++i) {
                                if (!wave[i] || wave[i].length < 2 || !isFinite(wave[i][0]) || !isFinite(wave[i][1]))
                                    continue
                                var x = i * width / Math.max(1, wave.length - 1)
                                var lo = Math.max(-1, Math.min(1, Number(wave[i][0])))
                                var hi = Math.max(-1, Math.min(1, Number(wave[i][1])))
                                ctx.moveTo(x, height * (0.5 - hi * 0.4)); ctx.lineTo(x, height * (0.5 - lo * 0.4))
                            }
                            ctx.stroke()
                        } else {
                            ctx.fillStyle = window.themeAccentColors[window.selectedThemeIndex]; var bar = width / Math.max(1, spec.length)
                            for (var k = 0; k < spec.length; ++k) {
                                if (!isFinite(spec[k]))
                                    continue
                                const magnitude = Math.max(0, spec[k])
                                ctx.fillRect(k * bar, height, Math.max(1, bar - 1), -Math.min(height * 0.45, magnitude * height * 2))
                            }
                            if (selectedSpectrumBin >= 0 && selectedSpectrumBin < spec.length) {
                                ctx.strokeStyle = window.themeForegrounds[window.selectedThemeIndex]; ctx.lineWidth = 2; ctx.beginPath()
                                ctx.moveTo((selectedSpectrumBin + 0.5) * width / spec.length, 0)
                                ctx.lineTo((selectedSpectrumBin + 0.5) * width / spec.length, height); ctx.stroke()
                            }
                        }
                        if (selectedPosition >= 0) {
                            ctx.strokeStyle = window.themeForegrounds[window.selectedThemeIndex]; ctx.lineWidth = window.compactHeader ? 1 : 2; ctx.beginPath()
                            ctx.moveTo(selectedPosition * width, 0); ctx.lineTo(selectedPosition * width, height); ctx.stroke()
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            if (mouse.button === Qt.RightButton) {
                                window.clearVisualizationPositionAndFocus()
                                mouse.accepted = true
                            }
                        }
                        onClicked: {
                            if (!visualizationCanvas.presentationReady)
                                return
                            if (visualizationCanvas.viewMode === "spectrum") {
                                visualizationCanvas.selectedSpectrumBin = Math.max(0, Math.min(visualizationCanvas.renderedSpectrum.length - 1, Math.floor(mouse.x * visualizationCanvas.renderedSpectrum.length / visualizationCanvas.width)))
                                visualizationCanvas.forceActiveFocus(); visualizationCanvas.requestPaint(); return
                            }
                            visualizationCanvas.selectedPosition = Math.max(0, Math.min(1, mouse.x / visualizationCanvas.width))
                            visualizationCanvas.forceActiveFocus()
                            visualizationCanvas.requestPaint()
                        }
                        onDoubleClicked: {
                            window.clearVisualizationPositionAndFocus()
                            mouse.accepted = true
                        }
                        onWheel: {
                            if (!visualizationCanvas.presentationReady) {
                                wheel.accepted = true
                                return
                            }
                            if (visualizationCanvas.viewMode === "spectrum") {
                                const binStep = (wheel.modifiers & Qt.ControlModifier) ? 10 : ((wheel.modifiers & Qt.ShiftModifier) ? 5 : 1)
                                visualizationCanvas.selectedSpectrumBin = Math.max(0, Math.min(visualizationCanvas.renderedSpectrum.length - 1, (visualizationCanvas.selectedSpectrumBin < 0 ? 0 : visualizationCanvas.selectedSpectrumBin) + Math.sign(wheel.angleDelta.y) * binStep))
                                visualizationCanvas.forceActiveFocus(); visualizationCanvas.requestPaint(); wheel.accepted = true; return
                            }
                            const stepDivisor = (wheel.modifiers & Qt.ControlModifier) ? 120 : ((wheel.modifiers & Qt.ShiftModifier) ? 240 : 1200)
                            const delta = wheel.angleDelta.y / stepDivisor
                            visualizationCanvas.selectedPosition = Math.max(0, Math.min(1, (visualizationCanvas.selectedPosition < 0 ? 0 : visualizationCanvas.selectedPosition) + delta))
                            visualizationCanvas.forceActiveFocus()
                            visualizationCanvas.requestPaint()
                            wheel.accepted = true
                        }
                    }
                    Rectangle {
                        objectName: "visualizationFocusRing"
                        anchors.fill: parent
                        color: "transparent"
                        border.width: window.compactHeader ? 2 : 3
                        border.color: window.themeAccentColors[window.selectedThemeIndex]
                        visible: visualizationCanvas.activeFocus
                        z: 2
                    }
                    Connections { target: typeof visualizationPresentation !== "undefined" ? visualizationPresentation : null; function onChanged() { visualizationCanvas.selectedSpectrumBin = -1; if (visualizationPresentation.revision >= 0) { window.visualizationSelectionResetReason = ""; visualizationCanvas.renderedWaveform = visualizationPresentation.waveform.slice(); visualizationCanvas.renderedSpectrum = visualizationPresentation.spectrum.slice(); visualizationCanvas.acceptedPlaybackEpoch = window.playbackBridge ? window.playbackBridge.epoch : -1; visualizationCanvas.acceptedPlaybackSource = window.playbackBridge ? window.playbackBridge.source.toString() : "" } else { visualizationCanvas.renderedWaveform = []; visualizationCanvas.renderedSpectrum = []; visualizationCanvas.acceptedPlaybackEpoch = -1; visualizationCanvas.acceptedPlaybackSource = "" } visualizationCanvas.renderedRevision = visualizationPresentation.revision >= 0 ? visualizationPresentation.revision : -1; visualizationCanvas.requestPaint() } }
                }
                Label {
                    objectName: "visualizationInteractionHint"
                    text: {
                        const modeLabel = visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT"
                        if (window.liveBridge.status === "Error")
                            return "Capture error; retry monitoring before selecting a " + modeLabel + " position."
                        if (visualizationCanvas.selectedPosition < 0)
                            return "Select a " + modeLabel + " position with the pointer or keyboard."
                        return modeLabel.charAt(0).toUpperCase() + modeLabel.slice(1) + " position " + Math.round(visualizationCanvas.selectedPosition * 100) + "% selected; Enter, Space, right-click, or double-click can clear it."
                    }
                    Accessible.name: "Visualization interaction hint, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Status
                    Accessible.description: text
                    ToolTip.text: "Current visualization selection and recovery hint; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    color: window.liveBridge.status === "Error" ? "#fca5a5" : window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: window.compactHeader ? 36 : 28
                    wrapMode: Text.WordWrap
                }
                Button {
                    objectName: "visualizationRetryCapture"
                    visible: window.liveBridge.status === "Error" && window.captureMonitorBridge && window.captureMonitorBridge.start
                    enabled: visible
                    text: "Retry monitoring"
                    Accessible.name: "Retry visualization capture"
                    Accessible.value: enabled ? "available" : "unavailable"
                    Accessible.role: Accessible.Button
                    Accessible.description: "Retry output monitoring to restore live waveform and FFT data."
                    ToolTip.text: "Retry output monitoring for visualization data"
                    contentItem: Text { text: parent.text; color: window.themeBackgrounds[window.selectedThemeIndex]; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: window.themeAccentColors[window.selectedThemeIndex]; radius: 5 }
                    Layout.minimumWidth: window.compactHeader ? 112 : 136
                    onClicked: {
                        window.overlayCaptureEvidenceStale = true
                        window.announceCaptureRecoveryState("Visualization capture retry requested; previous overlay evidence is stale until fresh monitoring data arrives.")
                        window.captureMonitorBridge.start()
                        visualizationCanvas.forceActiveFocus()
                    }
                }
                Button {
                    objectName: "visualizationClearSelection"
                    enabled: visualizationCanvas.selectedPosition >= 0
                    text: enabled ? (window.compactHeader ? "Clear" : "Clear selection") : "No selection"
                    Accessible.name: "Clear visualization selection, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Accessible.value: enabled ? "available" : "unavailable"
                    Accessible.role: Accessible.Button
                    Accessible.description: enabled
                        ? "Clear the selected waveform or FFT position; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                        : "There is no selected waveform or FFT position to clear; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                    ToolTip.text: (enabled ? "Clear the selected visualization position" : "No visualization position is selected") + "; capture: " + window.liveBridge.status + "; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    contentItem: Text { text: parent.text; color: window.themeBackgrounds[window.selectedThemeIndex]; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: parent.enabled ? 1 : 0.65 }
                    background: Rectangle { color: window.themeForegrounds[window.selectedThemeIndex]; radius: 5; opacity: parent.enabled ? 1 : 0.35 }
                    Layout.preferredWidth: window.compactHeader ? 120 : 180
                    onClicked: window.clearVisualizationPositionAndFocus()
                }
                Label {
                    objectName: "visualizationSelectionResetFeedback"
                    visible: window.visualizationSelectionResetReason !== ""
                    text: window.visualizationSelectionResetReason
                    Accessible.name: "Visualization selection reset status, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: text
                    ToolTip.text: "Why the visualization selection was cleared"
                    color: window.themeAccentColors[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.preferredHeight: window.compactHeader ? 36 : 28
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationRetryFeedback"
                    visible: window.liveBridge.status === "Starting" || window.liveBridge.status === "Reacquiring"
                    text: window.liveBridge.status === "Reacquiring" ? "Reacquiring capture; visualization will resume when output is available." : "Starting capture; visualization will populate when output is available."
                    Accessible.name: "Visualization capture recovery status, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Current visualization capture recovery transition"
                    color: window.themeAccentColors[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                ColumnLayout {
                    objectName: "visualizationStatusGroup"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.alignment: Qt.AlignTop
                    spacing: window.compactHeader ? 4 : 8
                    Accessible.name: "Visualization data and capture status, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Accessible.value: (window.liveBridge.status === "Error" ? "capture error" : (window.liveBridge.status === "Starting" || window.liveBridge.status === "Reacquiring" ? window.liveBridge.status.toLowerCase() : (visualizationCanvas.modeHasData ? "ready" : "waiting"))) + "; selection " + (visualizationCanvas.selectedPosition >= 0 ? "clear available" : "clear unavailable") + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Current waveform or FFT availability, generation, and capture state; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                    ToolTip.text: "Visualization data, generation, and capture status: " + (window.liveBridge.status === "Error" ? "capture error" : (visualizationCanvas.modeHasData ? "ready" : "waiting")) + "; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                Label {
                    objectName: "visualizationEmptyState"
                    visible: !visualizationCanvas.modeHasData
                    text: window.liveBridge.status === "Error"
                        ? "Capture error; retry monitoring to populate the " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT") + "."
                        : window.liveBridge.status === "Stopped"
                        ? "Start monitoring to populate the " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT") + "."
                        : "Waiting for captured audio data to populate the " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT") + "."
                    color: window.liveBridge.status === "Error" ? "#fca5a5" : window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Accessible.name: "Visualization empty state, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text + " " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")
                    ToolTip.text: "Visualization data availability and next action; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                }
                Label {
                    objectName: "visualizationLoadState"
                    text: typeof visualizationPresentation === "undefined"
                        ? "Unavailable"
                        : (visualizationCanvas.modeHasData ? (visualizationCanvas.reducedMotion ? "Ready · static snapshot" : "Ready") : (window.liveBridge.status === "Tracking" ? "Waiting for capture data" : "Idle"))
                    Accessible.name: "Visualization load state, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending" : "; recovery ready")
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Current waveform/FFT presentation load state; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationAnalysisHandoff"
                    text: typeof visualizationPresentation === "undefined" || !visualizationCanvas.presentationReady
                        ? (window.visualizationSelectionResetReason !== "" ? "Waiting for replacement analysis result; " + window.visualizationSelectionResetReason : "Waiting for an accepted analysis result")
                        : "Accepted analysis generation " + visualizationPresentation.generation + " · revision " + visualizationPresentation.revision
                    Accessible.name: "Visualization analysis handoff, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Status
                    Accessible.description: "Status of the generation-gated analysis result consumed by waveform and FFT rendering."
                    ToolTip.text: "Accepted analysis generation and revision consumed by the visualization; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationAnalysisQueueHandoff"
                    text: window.analysisJobBridge === null
                        ? "Analysis queue unavailable"
                        : (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running"
                           ? "Analysis handoff queued · " + window.analysisJobBridge.status
                           : (window.analysisJobBridge.result !== "" ? "Analysis handoff accepted · result ready" : "Analysis handoff " + window.analysisJobBridge.status))
                    Accessible.name: "Visualization analysis queue handoff: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: "Queue and cached-result status adjacent to the generation-gated visualization handoff."
                    ToolTip.text: "Cached, queued, and accepted analysis handoff status"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "selectedAnalysisActionStatus"
                    visible: window.selectedName !== ""
                    text: window.analysisJobBridge === null
                        ? "Selected track · analysis unavailable"
                        : (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running"
                           ? "Selected track · analysis in progress"
                           : (window.libraryAnalysisResultReady ? "Selected track · analysis ready" : "Selected track · analysis needs retry"))
                    Accessible.name: "Selected track analysis action status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: "Action-oriented status for the selected Library track analysis."
                    ToolTip.text: "Selected Library track analysis action status"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationMotionState"
                    text: visualizationCanvas.reducedMotion
                        ? "Reduced motion · static accepted snapshot"
                        : "Full motion · static accepted snapshot"
                    Accessible.name: "Visualization motion state: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: "Reduced motion changes presentation animation only; accepted waveform and FFT snapshots remain available for selection and seek."
                    ToolTip.text: "Motion preference for visualization presentation"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Button {
                    objectName: "selectedAnalysisRetry"
                    visible: window.selectedName !== "" && window.analysisJobBridge !== null && window.analysisJobBridge.canRetry
                    enabled: visible
                    text: "Retry selected analysis"
                    Accessible.name: "Retry selected track analysis"
                    Accessible.value: enabled ? "available" : "unavailable"
                    Accessible.role: Accessible.Button
                    Accessible.description: "Retry analysis for the selected Library track and replace stale visualization data when accepted."
                    ToolTip.text: "Retry analysis for the selected Library track"
                    onClicked: window.retryVisualizationAnalysis()
                }
                Label {
                    objectName: "visualizationPlaybackFeedback"
                    text: visualizationCanvas.selectedPosition < 0
                        ? (visualizationCanvas.reducedMotion ? "Playback seek ready after selecting a static snapshot position" : "Playback seek ready after selecting a position")
                        : (window.playbackBridge && window.playbackBridge.state !== "Stopped"
                           ? "Selected " + Math.round(visualizationCanvas.selectedPosition * 100) + "% · seek uses accepted snapshot"
                           : "Selected " + Math.round(visualizationCanvas.selectedPosition * 100) + "% · start playback to seek")
                    Accessible.name: "Visualization playback feedback, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.StaticText
                    Accessible.description: "Playback and seek readiness for the accepted waveform or FFT snapshot; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                    ToolTip.text: "Playback seek feedback for the accepted visualization snapshot; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationSpectrumReadout"
                    visible: visualizationCanvas.viewMode === "spectrum"
                    text: visualizationCanvas.selectedSpectrumBin < 0
                        ? "FFT bin: none selected"
                        : "FFT bin " + visualizationCanvas.selectedSpectrumBin + " · magnitude " + Number(visualizationCanvas.renderedSpectrum[visualizationCanvas.selectedSpectrumBin]).toFixed(3)
                    Accessible.name: "FFT selection readout, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: "Selected FFT bin and its accepted spectrum magnitude; wheel and keyboard selection update this readout."
                    ToolTip.text: "Selected FFT bin and accepted magnitude"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationRecoveryAnnouncement"
                    text: window.visualizationRecoveryAnnouncement
                    visible: text !== ""
                    Accessible.name: "Visualization recovery announcement, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Status
                    Accessible.description: "Live announcement for keyboard or pointer playback recovery actions; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                    ToolTip.text: "Latest visualization playback recovery action"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationDeviceRecoveryState"
                    text: window.endpointSelectionBridge !== null && window.endpointSelectionBridge.endpoints.length === 0
                        ? "No output device detected · connect a device and refresh endpoints"
                        : (window.captureMonitorBridge === null
                           ? "Capture permission/device bridge unavailable"
                           : (window.captureMonitorBridge.status === "Error"
                              ? "Capture retry failed · check permission and selected device"
                              : (window.captureMonitorBridge.status === "Reacquiring" ? "Capture reacquiring after a device transition" : "Capture device ready")))
                    Accessible.name: "Visualization device recovery state, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Status
                    Accessible.description: "Visible no-device, permission, retry-failure, and reacquisition guidance for visualization capture. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")
                    ToolTip.text: "Capture device and permission recovery state; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    color: window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "#fca5a5" : window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationPlaybackEpochFeedback"
                    text: visualizationCanvas.playbackSnapshotAligned
                        ? "Playback aligned · epoch " + window.playbackBridge.epoch + " · accepted snapshot seek available"
                        : (window.visualizationSelectionResetReason !== ""
                           ? "Playback snapshot cleared · " + window.visualizationSelectionResetReason
                           : "Playback source/epoch is not aligned with the accepted snapshot")
                    Accessible.name: "Visualization playback epoch feedback, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text
                    Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Status
                    Accessible.description: "Live playback source and epoch alignment for the accepted visualization snapshot; " + (window.overlayDisplayRecoveryPending ? "overlay recovery is pending." : "overlay is ready.")
                    ToolTip.text: "Playback source and epoch alignment for visualization seek; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationAcceptedIdentity"
                    text: visualizationCanvas.acceptedResultIdentity === ""
                        ? "No accepted result identity"
                        : "Accepted result identity: " + visualizationCanvas.acceptedResultIdentity
                    Accessible.name: "Visualization accepted result identity: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: "The source, generation, and revision that selection and seek controls currently target."
                    ToolTip.text: "Source identity and accepted analysis revision targeted by selection controls"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationStatus"
                    text: typeof visualizationPresentation === "undefined"
                          ? "Visualization unavailable"
                          : (visualizationCanvas.renderedWaveform.length === 0 && visualizationCanvas.renderedSpectrum.length === 0
                             ? "Waiting for captured audio data"
                             : "Waveform samples: " + visualizationCanvas.renderedWaveform.length + " · Spectrum bins: " + visualizationCanvas.renderedSpectrum.length)
                    Accessible.name: "Visualization data status"
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: text
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationDataStatus"
                    text: visualizationCanvas.renderedWaveform.length + " waveform samples · " + visualizationCanvas.renderedSpectrum.length + " spectrum bins"
                    Accessible.name: "Visualization data status: " + text; Accessible.role: Accessible.StaticText; Accessible.description: text
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.preferredHeight: window.compactHeader ? 36 : 28
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationSelectedDataStatus"
                    text: visualizationCanvas.viewMode === "waveform"
                        ? (visualizationCanvas.renderedWaveform.length + " samples in selected waveform view")
                        : (visualizationCanvas.renderedSpectrum.length + " bins in selected FFT view")
                    Accessible.name: "Selected visualization data status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Data available in the selected waveform or FFT view"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    objectName: "visualizationSelectionControls"
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: window.compactHeader ? 4 : 8
                    Accessible.name: "Visualization position selection"
                    Accessible.value: visualizationCanvas.selectedPosition < 0 ? "none" : Math.round(visualizationCanvas.selectedPosition * 100) + "% selected"
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Review or clear the selected waveform or FFT position."
                    ToolTip.text: "Visualization position selection and reset"
                Label {
                    objectName: "visualizationSelectionStatus"
                    text: visualizationCanvas.selectedPosition < 0 ? "No visualization position selected" : "Selected visualization position " + Math.round(visualizationCanvas.selectedPosition * 100) + "%"
                    Accessible.name: "Visualization selection status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: "Click the canvas or use Left/Right/Home/End to select a presentation position. " + text
                    ToolTip.text: "Current selected waveform/FFT position"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Slider {
                    id: visualizationPositionSlider
                    objectName: "visualizationPositionSlider"
                    from: 0; to: 1
                    value: visualizationCanvas.selectedPosition < 0 ? 0 : visualizationCanvas.selectedPosition
                    enabled: visualizationCanvas.presentationReady && visualizationCanvas.viewMode === "waveform"
                    activeFocusOnTab: true
                    Accessible.name: "Visualization playhead position"
                    Accessible.value: enabled ? Math.round(value * 100) + "%" : "unavailable"
                    Accessible.role: Accessible.Slider
                    Accessible.description: "Choose a time-aligned waveform position before seeking playback."
                    ToolTip.text: "Choose the waveform playhead position"
                    onMoved: { visualizationCanvas.selectedPosition = value; visualizationCanvas.requestPaint() }
                    Layout.fillWidth: true
                }
                Button {
                    objectName: "clearVisualizationSelection"
                    activeFocusOnTab: true
                    text: "Clear position"
                    ToolTip.text: enabled ? "Clear the selected waveform or FFT position; Escape, Enter, Space, right-click, or double-click also clear" : "No visualization position selected"
                    enabled: visualizationCanvas.selectedPosition >= 0
                    Accessible.name: "Clear visualization position"
                    Accessible.value: enabled ? "available" : "no position selected"
                    Accessible.role: Accessible.Button
                    Accessible.description: "Remove the selected waveform or FFT position. Escape, Enter, Space, right-click, and double-click provide equivalent recovery paths."
                    contentItem: Text { text: parent.text; color: window.themeBackgrounds[window.selectedThemeIndex]; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: parent.enabled ? 1 : 0.65 }
                    background: Rectangle { color: window.themeForegrounds[window.selectedThemeIndex]; radius: 5; opacity: parent.enabled ? 1 : 0.35 }
                    Layout.minimumWidth: window.compactHeader ? 104 : 128
                    onClicked: {
                        window.clearVisualizationPositionAndFocus()
                    }
                }
                Button {
                    objectName: "seekVisualizationSelection"
                    activeFocusOnTab: true
                    text: "Seek to position"
                    enabled: visualizationCanvas.selectedPosition >= 0
                        && visualizationCanvas.acceptedResultIdentity !== ""
                        && visualizationCanvas.playbackSnapshotAligned
                        && typeof visualizationPresentation !== "undefined"
                        && visualizationCanvas.renderedWaveform.length > 0
                    Accessible.name: "Seek playback to visualization position"
                    Accessible.value: enabled ? "available" : "unavailable"
                    Accessible.role: Accessible.Button
                    Accessible.description: enabled
                        ? "Seek playback to the selected waveform position using the available waveform sample span."
                        : "Select a waveform position and start playback before seeking."
                    ToolTip.text: enabled ? "Seek playback to the selected waveform position" : "Seeking requires an active playback source and waveform data"
                    onClicked: window.seekPlaybackToVisualizationPosition(visualizationCanvas.selectedPosition, visualizationCanvas.renderedWaveform.length)
                }
                Button {
                    objectName: "reopenSelectedPlayback"
                    activeFocusOnTab: true
                    text: "Reopen selected"
                    enabled: window.playbackBridge !== null && window.selectedAudioUrl.toString() !== ""
                    Accessible.name: "Reopen selected Library track"
                    Accessible.value: enabled ? "available" : "select a Library track"
                    Accessible.role: Accessible.Button
                    Accessible.description: enabled
                        ? "Start or replace playback with the currently selected Library track at frame zero."
                        : "Select a Library track before reopening playback."
                    ToolTip.text: enabled ? "Reopen the selected Library track from the beginning" : "Select a Library track first"
                    onClicked: { window.reopenSelectedPlayback(); reopenSelectedPlayback.forceActiveFocus() }
                }
                Button {
                    objectName: "resumeVisualizationPlayback"
                    activeFocusOnTab: true
                    text: "Resume playback"
                    enabled: window.playbackBridge !== null && window.playbackBridge.state === "Paused"
                    Accessible.name: "Resume playback for visualization"
                    Accessible.value: enabled ? "available" : "unavailable"
                    Accessible.role: Accessible.Button
                    Accessible.description: enabled ? "Resume playback while retaining the accepted visualization snapshot." : "Playback must be paused to resume it."
                    ToolTip.text: enabled ? "Resume playback without clearing the accepted snapshot" : "Resume is available when playback is paused"
                    onClicked: window.togglePlaybackPause()
                }
                Button {
                    objectName: "stopVisualizationPlayback"
                    activeFocusOnTab: true
                    text: "Stop playback"
                    enabled: window.playbackBridge !== null && window.playbackBridge.state !== "Stopped"
                    Accessible.name: "Stop playback and clear visualization"
                    Accessible.value: enabled ? "available" : "unavailable"
                    Accessible.role: Accessible.Button
                    Accessible.description: enabled ? "Stop playback and clear the accepted visualization snapshot." : "Playback is already stopped."
                    ToolTip.text: enabled ? "Stop playback and clear the accepted visualization snapshot" : "Playback is already stopped"
                    onClicked: window.stopPlaybackWithRecovery("Playback stopped; accepted snapshot cleared.")
                }
                }
                Label {
                    objectName: "visualizationRevisionStatus"
                    text: typeof visualizationPresentation === "undefined"
                        ? "No visualization generation is active"
                        : "Visualization generation " + visualizationPresentation.generation + " · revision " + visualizationPresentation.revision
                    Accessible.name: "Visualization generation status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Current accepted visualization generation and revision"
                    color: window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "visualizationCaptureStatus"
                    text: window.liveBridge.status === "Stopped" ? "Capture is stopped; start monitoring to populate the visualization." : "Capture is active; visualization updates from the current output stream."
                    Accessible.name: "Visualization capture status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: text
                    ToolTip.text: "Current capture state feeding waveform and FFT data"
                    color: window.liveBridge.status === "Error" ? "#fca5a5" : window.themeForegrounds[window.selectedThemeIndex]
                    font.pixelSize: window.compactHeader ? 12 : 14
                    Layout.fillWidth: true; Layout.minimumWidth: 0; wrapMode: Text.WordWrap
                }
                }
                }
                ColumnLayout {
                    objectName: "djEvidenceGroup"
                    Layout.fillWidth: true
                    Accessible.name: "DJ monitoring evidence"
                    Accessible.value: window.liveBridge.status
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Current DJ monitoring state, estimates, and evidence-quality guidance."
                    ToolTip.text: "DJ monitoring state, estimates, and evidence quality"
                    Label {
                        objectName: "djStatus"; text: "Status: " + window.liveBridge.status
                        Accessible.name: "DJ monitoring status: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current DJ monitoring status"
                    }
                    Label {
                        objectName: "djEstimates"
                        text: "BPM: " + window.liveBridge.tempo + "    Key: " + window.liveBridge.key + "    Chord: " + window.liveBridge.chord
                        Accessible.name: "DJ estimates"; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current BPM, key, and chord estimates"
                    }
                    Label {
                     objectName: "djEvidenceStatus"
                     text: {
                         const state = window.liveBridge.status
                         if (state === "Tracking") return "Live evidence is provisional and source-attributed to the selected output mix. Overlapping decks or notifications may make key/chord labels ambiguous."
                         if (state === "Reacquiring") return "Reacquiring the selected output source; previous BPM, key, and chord evidence has been cleared."
                         if (state === "Silent") return "No active output evidence; BPM, key, and chord labels are unavailable until audio resumes."
                         if (state === "Error") return "Output capture error; live evidence is unavailable. Check the selected endpoint and retry monitoring."
                         return "Live evidence is unavailable until output monitoring starts."
                     }
                     Accessible.name: "DJ evidence status: " + text
                     Accessible.value: text
                     Accessible.role: Accessible.StaticText
                     Accessible.description: text
                     ToolTip.text: "Live evidence quality and recovery status"
                     wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                }
                RowLayout {
                    objectName: "overlayActions"
                    visible: window.overlayBridge !== null
                    Accessible.name: "Overlay actions"
                    Accessible.value: window.overlayBridge && window.overlayBridge.available
                        ? (window.overlayHiddenByUser ? "available; hidden by user" : "available") + (window.overlayDisplayRecoveryPending ? "; recovery pending" : "") + "; settings retained; revision " + window.overlaySettingsRevision + "; Ctrl+Alt+Shift+B focuses beat pulse status"
                        : "unavailable; control-window fallback active; settings retained; Ctrl+Alt+Shift+B focuses beat pulse status"
                    Accessible.role: Accessible.Grouping
                    Accessible.description: window.overlayLaunchFallbackActive
                        ? "Show, unlock, or hide the DJ overlay window; native overlay launch fallback is active, the control window remains available, and overlay settings are retained. Ctrl+Alt+Shift+B focuses beat pulse status."
                        : window.overlayDisplayRecoveryPending
                        ? "Show, unlock, or hide the DJ overlay window; recovery is pending, the control-window fallback remains available, and persisted settings are retained. Ctrl+Alt+Shift+B focuses beat pulse status."
                        : "Show, unlock, or hide the DJ overlay window. Ctrl+Alt+Shift+B focuses beat pulse status."
                    ToolTip.text: window.overlayBridge && window.overlayBridge.available
                        ? (window.overlayHiddenByUser ? "Overlay actions are available; hidden by user" : (window.overlayDisplayRecoveryPending ? "Overlay actions are available; recovery is pending" : "Overlay window actions are available")) + "; Ctrl+Alt+Shift+B focuses beat pulse status"
                        : "Overlay window actions are unavailable; control-window fallback is active; overlay settings are retained; Ctrl+Alt+Shift+B focuses beat pulse status"
                    Button { text: "Show overlay"; objectName: "showOverlay"; Accessible.name: "Show DJ overlay"; Accessible.role: Accessible.Button; Accessible.value: enabled ? (window.overlayDisplayRecoveryPending ? "available; recovery pending" : "available") : (window.overlayDisplaySelectionRequired ? "choose a display first" : "unavailable; control-window fallback active"); Accessible.description: enabled ? "Show the DJ overlay window" : (window.overlayDisplaySelectionRequired ? "Choose an available display before showing the overlay" : "Native overlay unavailable; keep using the control-window fallback"); ToolTip.text: enabled ? "Show the DJ overlay and hide the control window" : (window.overlayDisplaySelectionRequired ? "Choose a display before showing the overlay" : "Overlay unavailable; control-window fallback is active"); enabled: window.overlayBridge && window.overlayBridge.available && !window.overlayDisplaySelectionRequired && window.overlayBridge.screenNames.length > 0; onClicked: window.handleOverlayShowButtonAction() }
                    Button { text: window.overlayHiddenByUser ? "Restore hidden overlay" : "Restore overlay"; objectName: "restoreOverlay"; Accessible.name: "Restore DJ overlay"; Accessible.role: Accessible.Button; Accessible.value: enabled ? (window.overlayHiddenByUser ? "hidden by user" : (window.overlayDisplayRecoveryPending ? "available; recovery pending" : "available")) : (window.overlayDisplaySelectionRequired ? "choose a display first" : "unavailable; control-window fallback active"); Accessible.description: enabled ? (window.overlayHiddenByUser ? "Restore the overlay hidden by the user using its persisted display, position, size, and opacity." : "Restore the DJ overlay using its persisted display, position, size, and opacity") : (window.overlayDisplaySelectionRequired ? "Choose a display before restoring the overlay" : "Native overlay unavailable; control-window fallback remains available"); ToolTip.text: enabled ? (window.overlayHiddenByUser ? "Restore the overlay hidden by the user with persisted settings" : "Restore the overlay with persisted settings") : (window.overlayDisplaySelectionRequired ? "Choose a display before restoring the overlay" : "Overlay unavailable; control-window fallback is active"); enabled: window.overlayBridge && window.overlayBridge.available && !window.overlayDisplaySelectionRequired && window.overlayBridge.screenNames.length > 0; onClicked: window.handleOverlayRestoreButtonAction() }
                    Button { text: "Reapply saved settings"; objectName: "reapplyOverlaySettings"; Accessible.name: "Reapply saved overlay settings"; Accessible.role: Accessible.Button; Accessible.value: enabled ? (window.overlayDisplayRecoveryPending ? "available; recovery pending" : "available") : (window.overlayDisplaySelectionRequired ? "choose a display first" : "unavailable; control-window fallback active"); Accessible.description: enabled ? "Reapply the saved display, position, size, and opacity before restoring the DJ overlay" : (window.overlayDisplaySelectionRequired ? "Choose an available display before reapplying saved settings" : "Native overlay unavailable; control-window fallback remains available"); ToolTip.text: enabled ? "Reapply saved overlay display, geometry, and opacity" : (window.overlayDisplaySelectionRequired ? "Choose a display before reapplying settings" : "Overlay unavailable; control-window fallback is active"); enabled: window.overlayBridge && window.overlayBridge.available && !window.overlayDisplaySelectionRequired && window.overlayBridge.screenNames.length > 0; onClicked: window.handleOverlayReapplyButtonAction() }
                    Button { id: unlockOverlay; text: "Unlock overlay"; objectName: "unlockOverlay"; Accessible.name: "Unlock DJ overlay"; Accessible.role: Accessible.Button; Accessible.value: enabled ? (window.overlayCaptureEvidenceStale ? "available; live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "available; recovery pending" : "available") : "unavailable"; Accessible.description: enabled ? "Restore keyboard and pointer access to the overlay" + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : "") : "Overlay is not in click-through mode or is unavailable"; ToolTip.text: enabled ? "Restore keyboard and pointer access to the overlay" : "Overlay is not in click-through mode"; enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.clickThrough; onClicked: window.handleOverlayUnlockAction() }
                    Button { text: "Hide overlay"; objectName: "hideOverlay"; Accessible.name: "Hide DJ overlay"; Accessible.role: Accessible.Button; Accessible.value: enabled ? (window.overlayHiddenByUser ? "already hidden" : "available") : "unavailable"; Accessible.description: enabled ? "Hide the DJ overlay window and restore the control window" : "Native overlay unavailable; control-window fallback is active"; ToolTip.text: "Hide the DJ overlay and restore the control window"; enabled: window.overlayBridge && window.overlayBridge.available; onClicked: window.handleOverlayHideAction() }
                    Button { text: "Quit Beatglass"; objectName: "quitBeatglass"; Accessible.name: "Quit Beatglass"; Accessible.role: Accessible.Button; Accessible.value: "available; overlay settings retained"; Accessible.description: "Hide the DJ overlay and quit Beatglass while retaining persisted settings"; ToolTip.text: "Quit Beatglass and retain overlay settings"; onClicked: { window.announceCaptureRecoveryState("Beatglass quitting; overlay display, geometry, size, opacity, and interaction settings are retained."); window.quitApplication() } }
                }
                Label {
                    objectName: "overlayShortcutHelp"
                    text: "Keyboard: Esc unlocks · Ctrl+Alt+U focuses unlock · Ctrl+Alt+I toggles click-through · Ctrl+Alt+H hides/restores · Ctrl+Alt+J focuses hide · Ctrl+Alt+W focuses show · Ctrl+Alt+Shift+W focuses restore · Ctrl+Alt+D focuses display · Ctrl+Alt+Y retries overlay · Ctrl+Alt+Shift+Y focuses retry · Ctrl+Alt+O cycles size · Ctrl+Alt+Z focuses size · Ctrl+Alt+Shift+O resets opacity · Ctrl+Alt+L focuses opacity · Ctrl+Alt+Shift+R reapplies saved settings · Ctrl+Alt+Shift+P focuses reapply · Ctrl+Alt+Shift+S reviews retained settings · Ctrl+Alt+Shift+M reviews overlay summary · Ctrl+Alt+Shift+B focuses beat pulse status"
                    Accessible.name: "Overlay keyboard shortcuts"
                    Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : "") + (window.overlayBridge !== null ? "; retained-settings review shortcut available" : "; retained-settings review unavailable")
                    Accessible.role: Accessible.Status
                    Accessible.description: "Keyboard shortcuts for unlocking, hiding, restoring, reapplying retained settings, reviewing the overlay summary, and focusing beat pulse status." + (window.overlayCaptureEvidenceStale ? " Live evidence is stale until fresh capture arrives." : "")
                    ToolTip.text: text
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    objectName: "overlayRoutingControls"
                    Accessible.name: "Overlay interaction and opacity controls"
                    Accessible.value: window.overlayBridge
                        ? (window.overlayBridge.clickThrough ? "click-through" : "interactive") + ", " + Math.round(window.overlayBridge.opacity * 100) + "% opacity, settings revision " + window.overlaySettingsRevision + (window.overlayCaptureEvidenceStale ? ", live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? ", recovery pending" : "")
                        : "unavailable; control-window fallback active"
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Configure pointer click-through and overlay opacity." + (window.overlayCaptureEvidenceStale ? " Live evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? " Recovery is pending." : "")
                    ToolTip.text: "Overlay interaction mode and opacity; current value is announced when available"
                    CheckBox {
                        id: clickThroughToggle
                        text: "Click through"; objectName: "clickThrough"
                        activeFocusOnTab: true
                        Accessible.name: checked ? "Disable click-through" : "Enable click-through"
                        Accessible.value: (checked ? "enabled" : "disabled") + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; recovery pending" : "")
                        Accessible.role: Accessible.CheckBox
                        Accessible.description: "Allow pointer input to pass through the overlay window; currently " + (checked ? "enabled" : "disabled") + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : "")
                        ToolTip.text: "Toggle pointer click-through for the overlay" + (window.overlayCaptureEvidenceStale ? "; live evidence is stale until fresh capture" : "")
                        enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.screenNames.length > 0
                        onClicked: window.handleOverlayClickThroughButtonAction(checked)
                        Connections {
                            target: window.overlayBridge
                            function onClickThroughChanged() { clickThroughToggle.checked = window.overlayBridge.clickThrough }
                        }
                    }
                    Label { objectName: "overlayInteractionMode"; text: window.overlayBridge ? (window.overlayBridge.clickThrough ? "Input mode: click-through" : "Input mode: interactive") : "Input mode unavailable"; Accessible.name: "Overlay interaction mode: " + text + (window.overlayCaptureEvidenceStale ? ", live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? ", recovery pending" : ", ready"); Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; recovery pending; settings retained" : ""); Accessible.role: Accessible.StaticText; Accessible.description: text + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? ". Recovery is pending; overlay settings are retained." : ""); ToolTip.text: "Current overlay pointer interaction mode" }
                    Label {
                        objectName: "overlayKeyboardRecoveryStatus"
                        visible: window.overlayBridge !== null
                        text: visible
                            ? (window.overlayBridge.clickThrough ? "Keyboard recovery: press Escape to unlock overlay (Ctrl+Alt+U focuses Unlock)" : "Keyboard recovery: overlay controls are interactive")
                            : "Keyboard recovery unavailable"
                        Accessible.name: "Overlay keyboard recovery status"
                        Accessible.value: text + "; Ctrl+Alt+Shift+B focuses beat pulse status" + (window.captureMonitorBridge && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring") ? "; live evidence stale" : "")
                        Accessible.role: Accessible.Status
                        Accessible.description: visible && window.overlayBridge.clickThrough
                            ? text + ". Ctrl+Alt+U also focuses Unlock overlay from the control window. Ctrl+Alt+Shift+B focuses beat pulse status."
                            : visible ? text + ". Click-through can be enabled or disabled from the overlay controls." : text
                        ToolTip.text: visible ? Accessible.description : "Overlay keyboard recovery"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Label { objectName: "overlayUnlockFailureStatus"; visible: window.overlayLaunchFallbackActive && window.overlayBridge !== null && window.overlayBridge.clickThrough; text: "Unlock failed · control window fallback active"; Accessible.name: "Overlay unlock failure status"; Accessible.role: Accessible.Status; Accessible.value: text; Accessible.description: "Native overlay unlock failed; the control window is active so Unlock overlay can be retried."; ToolTip.text: "Unlock failed; retry from the control window"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Label {
                        objectName: "overlayEndpointStatus"
                        visible: window.overlayBridge !== null
                        text: visible && window.endpointSelectionBridge && window.endpointSelectionBridge.selectedEndpoint !== ""
                            ? "Overlay endpoint: " + window.endpointSelectionBridge.selectedEndpoint
                            : "Overlay endpoint: none selected"
                        Accessible.name: "Overlay capture endpoint status, " + (window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")
                        Accessible.value: text + (window.captureMonitorBridge && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring") ? "; live evidence stale" : "")
                        Accessible.role: Accessible.Status
                        Accessible.description: visible && window.endpointSelectionBridge && window.endpointSelectionBridge.selectedEndpoint !== ""
                            ? text + ". The overlay monitors the selected system output endpoint; it does not identify individual decks." + (window.overlayDisplayRecoveryPending ? " The selected endpoint remains retained while recovery is pending." : "") + (window.captureMonitorBridge && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring") ? " Live overlay evidence is stale until fresh capture arrives." : "")
                            : text + ". Choose an output endpoint before starting monitoring." + (window.overlayDisplayRecoveryPending ? " The last endpoint selection remains retained while recovery is pending." : "")
                        ToolTip.text: visible ? Accessible.description : "Overlay capture endpoint"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Button {
                        objectName: "overlayEndpointRefreshAction"
                        visible: window.overlayBridge !== null
                        text: "Refresh overlay endpoint"
                        enabled: visible && window.endpointSelectionBridge !== null
                        Accessible.name: text
                        Accessible.value: enabled ? (window.overlayDisplayRecoveryPending ? "refresh available; recovery pending" : "refresh available; ready") + (window.captureMonitorBridge && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring") ? "; live evidence stale" : "") : "refresh unavailable"
                        Accessible.role: Accessible.Button
                        Accessible.description: enabled ? "Refresh available output endpoints before starting or retrying overlay monitoring. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.") + (window.captureMonitorBridge && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring") ? " Live overlay evidence is stale until fresh capture arrives." : "") : "Output endpoint refresh is unavailable."
                        ToolTip.text: Accessible.description + " Current settings revision: " + window.overlaySettingsRevision + "."
                        onClicked: {
                            window.endpointSelectionBridge.refresh()
                            window.announceCaptureRecoveryState("Overlay endpoint refresh requested; choose an output endpoint before retrying monitoring.")
                            endpointSelector.forceActiveFocus()
                        }
                    }
                    Label {
                        objectName: "overlayLiveMonitoringState"
                        activeFocusOnTab: true
                        visible: window.overlayBridge !== null
                        text: visible ? ("Overlay monitoring: " + window.liveBridge.status
                            + (window.liveBridge.status === "Tracking" ? " · live estimates updating" : " · waiting for accepted output")) : "Overlay monitoring unavailable"
                        Accessible.name: "Overlay live monitoring state, " + (window.overlayCaptureEvidenceStale ? "evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")
                        Accessible.value: visible ? text + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : "") : ""
                        Accessible.role: Accessible.Status
                        Accessible.description: visible && window.liveBridge.status === "Tracking"
                            ? text + ". Estimates are attributed to the monitored combined output." + (window.overlayCaptureEvidenceStale ? " Previously displayed evidence is stale until fresh capture arrives." : "") + (window.overlayDisplayRecoveryPending ? " The selected endpoint remains retained while recovery is pending." : "")
                            : visible ? text + ". The overlay remains available while capture recovers; the selected endpoint remains retained." + (window.overlayCaptureEvidenceStale ? " Live overlay evidence is stale until fresh capture arrives." : "") : text
                        ToolTip.text: visible ? Accessible.description : "Overlay monitoring state"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        objectName: "overlayCaptureRecoveryStatus"
                        visible: window.overlayBridge !== null && window.captureMonitorBridge !== null
                            && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring")
                        text: window.captureMonitorBridge && window.captureMonitorBridge.status === "Reacquiring"
                            ? "Monitoring paused · reacquiring selected output"
                            : "Monitoring paused · selected output unavailable"
                        Accessible.name: "Overlay capture recovery status"
                        Accessible.value: text + "; no estimates"
                        Accessible.role: Accessible.Status
                        Accessible.description: text + ". No new estimates are published while the output is unavailable; retry capture after reconnecting or choosing another output."
                        ToolTip.text: Accessible.description
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Button {
                        objectName: "overlayMonitoringAction"
                        visible: window.overlayBridge !== null
                        text: visible && window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" ? "Stop overlay monitoring" : "Start overlay monitoring"
                        enabled: visible && window.captureMonitorBridge !== null
                            && window.captureMonitorBridge.status !== "Starting"
                            && window.captureMonitorBridge.status !== "Reacquiring"
                        Accessible.name: text
                        Accessible.value: (window.captureMonitorBridge ? window.captureMonitorBridge.status : "unavailable")
                            + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; recovery pending" : "; ready")
                        Accessible.role: Accessible.Button
                        Accessible.description: enabled
                            ? (window.captureMonitorBridge.status === "Monitoring" ? "Stop live output monitoring while keeping overlay settings available." : "Start live output monitoring for the overlay's combined-output estimates.") + (window.overlayDisplayRecoveryPending ? " Overlay recovery is pending." : " Overlay is ready.") + (window.overlayCaptureEvidenceStale ? " Live overlay evidence is stale until fresh capture arrives." : "")
                            : "Monitoring is transitioning; live overlay evidence is stale until the current capture state produces a fresh accepted snapshot."
                        ToolTip.text: Accessible.description + " Current settings revision: " + window.overlaySettingsRevision + "."
                        onClicked: {
                            if (!window.captureMonitorBridge) return
                            const status = window.captureMonitorBridge.status
                            if (status === "Monitoring") {
                                window.captureMonitorBridge.stop()
                                window.overlayCaptureEvidenceStale = true
                                window.announceCaptureRecoveryState("Overlay monitoring stopped; overlay settings remain available.")
                            } else if (status === "Starting" || status === "Reacquiring") {
                                window.announceCaptureRecoveryState("Overlay monitoring is already transitioning; no duplicate capture request was sent.")
                            } else {
                                window.overlayCaptureEvidenceStale = true
                                window.captureMonitorBridge.start()
                                window.announceCaptureRecoveryState("Overlay monitoring requested; waiting for accepted output data.")
                            }
                            overlayLiveMonitoringState.forceActiveFocus()
                        }
                    }
                    Button {
                        objectName: "overlayCaptureRecoveryAction"
                        visible: window.overlayBridge !== null && window.captureMonitorBridge !== null && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring")
                        text: "Retry overlay capture"
                        enabled: visible && window.captureMonitorBridge.start !== undefined
                        Accessible.name: text
                        Accessible.value: enabled ? (window.overlayCaptureEvidenceStale ? "retry available; evidence stale until fresh capture" : (window.captureMonitorBridge.status === "Reacquiring" ? "retry available; reacquiring" : "retry available; capture status error")) : "retry unavailable"
                        Accessible.role: Accessible.Button
                        Accessible.description: enabled
                            ? "Retry the selected output capture for the overlay and wait for a fresh accepted snapshot; " + (window.overlayCaptureEvidenceStale ? "current evidence is stale; " : "") + "settings revision " + window.overlaySettingsRevision + "."
                            : "Overlay capture retry is unavailable; settings revision " + window.overlaySettingsRevision + "."
                        ToolTip.text: Accessible.description
                        onClicked: {
                            window.overlayCaptureEvidenceStale = true
                            window.captureMonitorBridge.start()
                            window.announceCaptureRecoveryState("Overlay capture retry requested; waiting for a fresh accepted snapshot.")
                            overlayLiveMonitoringState.forceActiveFocus()
                        }
                    }
                    Button {
                        objectName: "chooseOverlayOutput"
                        visible: window.overlayBridge !== null && window.captureMonitorBridge !== null
                            && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring")
                        text: "Choose output"
                        enabled: visible && window.endpointSelectionBridge !== null
                        Accessible.name: "Choose overlay output"
                        Accessible.value: enabled ? "available; capture recovery" : "unavailable"
                        Accessible.role: Accessible.Button
                        Accessible.description: enabled
                            ? "Choose another system output for overlay monitoring before retrying capture; current estimates remain unavailable until fresh output is accepted."
                            : "Output selection is unavailable while overlay capture recovers."
                        ToolTip.text: Accessible.description
                        onClicked: {
                            window.endpointSelectionBridge.refresh()
                            window.announceCaptureRecoveryState("Choose an output endpoint before retrying overlay capture.")
                            endpointSelector.forceActiveFocus()
                        }
                    }
                    Label {
                        objectName: "overlayEstimateAttribution"
                        visible: window.overlayBridge !== null
                        text: visible ? "Estimate source: selected system output · combined mix" : "Estimate source unavailable"
                        Accessible.name: "Overlay estimate attribution, " + (window.overlayCaptureEvidenceStale ? "evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")
                        Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; recovery pending; source retained" : "; source ready")
                        Accessible.role: Accessible.StaticText
                        Accessible.description: visible
                            ? text + ". Track and deck metadata are unavailable unless a separately supported source provides them." + (window.overlayCaptureEvidenceStale ? " Previously displayed estimates are stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? " The selected combined-output source is retained while recovery is pending." : "")
                            : text
                        ToolTip.text: visible ? Accessible.description : "Overlay estimate source"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        objectName: "overlayTrackMetadataUnavailable"
                        visible: window.overlayBridge !== null
                        text: "Track info unavailable"
                        Accessible.name: "Overlay track information unavailable"
                        Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : "")
                        Accessible.role: Accessible.Status
                        Accessible.description: "The combined system output does not identify individual tracks or decks. Beatglass will not attribute metadata without a separately supported source."
                        ToolTip.text: Accessible.description
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        objectName: "overlaySettingsRetentionStatus"
                        activeFocusOnTab: true
                        visible: window.overlayBridge !== null
                        text: visible
                            ? (window.overlayDisplayRecoveryPending
                                ? "Overlay settings retained while recovery is pending"
                                : "Overlay settings retained: display, geometry, opacity, and interaction mode")
                            : "Overlay settings retention unavailable"
                        Accessible.name: "Overlay settings retention, " + (window.overlayCaptureEvidenceStale ? "evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending" : "ready") + ", revision " + window.overlaySettingsRevision
                        Accessible.value: text + "; settings revision " + window.overlaySettingsRevision
                        Accessible.role: Accessible.Status
                        Accessible.description: visible
                            ? text + ". Persisted settings remain available to restore the native overlay or use the control-window fallback."
                            : text
                        ToolTip.text: visible ? Accessible.description : "Overlay settings retention"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Button {
                        objectName: "overlaySettingsRetentionReviewAction"
                        visible: window.overlayBridge !== null
                        text: window.overlayDisplayRecoveryPending ? "Review recovery settings" : "Review retained settings"
                        activeFocusOnTab: true
                        enabled: visible
                        Accessible.name: text
                        Accessible.value: (window.overlayCaptureEvidenceStale ? "evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending" : "settings retained") + "; settings revision " + window.overlaySettingsRevision
                        Accessible.role: Accessible.Button
                        Accessible.description: window.overlayDisplayRecoveryPending
                            ? "Focus the overlay settings-retention status to review persisted settings while recovery is pending; current settings revision " + window.overlaySettingsRevision + (window.overlayCaptureEvidenceStale ? ". Previously displayed evidence is stale until fresh capture arrives." : ".")
                            : "Focus the overlay settings-retention status to review persisted display, geometry, opacity, and interaction settings; current settings revision " + window.overlaySettingsRevision + "."
                        ToolTip.text: Accessible.description + " Current settings revision: " + window.overlaySettingsRevision + "."
                        onClicked: {
                            overlaySettingsRetentionStatus.forceActiveFocus()
                            window.announceCaptureRecoveryState(window.overlayDisplayRecoveryPending
                                ? "Reviewing retained overlay settings while recovery is pending; settings revision " + window.overlaySettingsRevision + "."
                                : "Reviewing retained overlay settings; native overlay recovery is available at settings revision " + window.overlaySettingsRevision + ".")
                        }
                    }
                    Label {
                        objectName: "overlayDataReadiness"
                        visible: window.overlayBridge !== null
                        text: visible
                            ? (visualizationCanvas.modeHasData ? "Overlay estimates: accepted snapshot ready" : "Overlay estimates: waiting for accepted snapshot")
                            : "Overlay estimates unavailable"
                        Accessible.name: "Overlay estimate readiness, " + (window.overlayCaptureEvidenceStale ? "evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")
                        Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending; snapshot retained" : "; recovery ready")
                        Accessible.role: Accessible.Status
                        Accessible.description: visible && visualizationCanvas.modeHasData
                            ? text + ". The overlay is showing data from the current accepted visualization snapshot." + (window.overlayCaptureEvidenceStale ? " The accepted snapshot is stale until fresh capture arrives." : "") + (window.overlayDisplayRecoveryPending ? " The accepted snapshot remains retained while recovery is pending." : "")
                            : visible ? text + ". The overlay will update after monitoring accepts a replacement snapshot." + (window.overlayDisplayRecoveryPending ? " The last accepted snapshot remains retained while recovery is pending." : "") : text
                        ToolTip.text: visible ? Accessible.description : "Overlay estimate readiness"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Label { text: "Opacity"; Accessible.name: "Overlay opacity control"; Accessible.role: Accessible.Heading; ToolTip.text: "Adjust the overlay window opacity" }
                    Slider { id: overlayOpacityControl; objectName: "overlayOpacity"; Accessible.name: "Overlay opacity"; Accessible.role: Accessible.Slider; Accessible.value: Math.round(value * 100) + "%" + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; recovery pending" : ""); Accessible.description: "Adjust the overlay window opacity; current value " + Math.round(value * 100) + "%" + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? ". Recovery is pending." : ""); ToolTip.text: "Adjust overlay opacity from 20% to 100%"; from: 0.2; to: 1; value: window.overlayBridge ? window.overlayBridge.opacity : 0.9; enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.screenNames.length > 0; onMoved: { if (window.overlayBridge) { const applied = window.overlayBridge.setOpacity(value); if (!applied) { value = window.overlayBridge.opacity; window.focusOverlayOpacityEditRecovery("DJ overlay opacity could not be changed; persisted opacity remains active."); return } window.overlayOpacityEditInFlight = true; window.overlayPendingOpacity = value; window.overlaySettingsRevision += 1 } } onPressedChanged: if (!pressed) window.focusOverlayOpacityEditRecovery("DJ overlay opacity edit complete; opacity control remains focused for keyboard adjustment.") }
                    Connections {
                        target: window.overlayBridge
                        function onOpacityChanged() { if (window.overlayBridge && (!window.overlayOpacityEditInFlight || Math.abs(Number(window.overlayBridge.opacity) - window.overlayPendingOpacity) < 0.001) ) overlayOpacityControl.value = window.overlayBridge.opacity }
                    }
                    Label { objectName: "overlayOpacityValue"; text: window.overlayBridge ? "Current opacity: " + Math.round(window.overlayBridge.opacity * 100) + "%" : "Opacity unavailable"; Accessible.name: "Current overlay opacity: " + text; Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : ""); Accessible.role: Accessible.StaticText; Accessible.description: text + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : ""); ToolTip.text: "Current overlay opacity value" }
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    objectName: "overlayDisplayControls"
                    Accessible.name: "Overlay display and size controls"
                    Accessible.value: (window.overlayBridge && window.overlayBridge.screenIndex >= 0 && window.overlayBridge.screenIndex < window.overlayBridge.screenNames.length ? window.overlayBridge.screenNames[window.overlayBridge.screenIndex] : "unavailable") + (window.overlayDisplayRecoveryPending ? "; recovery pending; live evidence stale" : "") + (window.overlayHiddenByUser ? "; hidden by user" : "")
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Choose the overlay destination display and window size." + (window.overlayHiddenByUser ? " The overlay is hidden by user request." : "") + (window.overlayCaptureEvidenceStale ? " Live overlay evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? " Recovery is pending and live overlay evidence is stale until a display is confirmed." : "")
                    ToolTip.text: "Overlay destination display and window size; current display is announced when available" + (window.overlayCaptureEvidenceStale ? "; live evidence is stale until fresh capture" : "")
                    Label { text: "Display"; Accessible.name: "Overlay display control"; Accessible.role: Accessible.Heading; ToolTip.text: "Choose the overlay destination display" }
                    ComboBox {
                        objectName: "overlayDisplay"
                        activeFocusOnTab: true
                    Accessible.name: "Overlay display"; Accessible.value: (currentIndex >= 0 && count > currentIndex ? currentText : "unavailable") + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; recovery pending" : ""); Accessible.role: Accessible.ComboBox; Accessible.description: "Choose the display where the overlay is shown; current display " + (currentIndex >= 0 && count > currentIndex ? currentText : "unavailable") + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? ". Recovery is pending." : ""); ToolTip.text: "Choose the display where the overlay is shown"
                        enabled: window.overlayBridge && window.overlayBridge.available && count > 0
                        model: window.overlayBridge ? window.overlayBridge.screenNames : []
                        currentIndex: window.overlayBridge && window.overlayBridge.screenIndex >= 0 ? window.overlayBridge.screenIndex : 0
                        onActivated: {
                            const requestedIndex = currentIndex
                            if (window.persistOverlayDisplaySelection(requestedIndex, currentText))
                                window.focusOverlayDisplaySelectionRecovery("Overlay display selected: " + currentText + ". Display selection is ready for recovery.")
                            else {
                                currentIndex = window.overlayBridge && window.overlayBridge.screenIndex >= 0 ? window.overlayBridge.screenIndex : 0
                                window.focusOverlayDisplaySelectionRecovery("Overlay display selection could not be applied; persisted display remains active.")
                            }
                        }
                        Layout.fillWidth: true
                    }
                    Button {
                        objectName: "forgetUnavailableOverlayDisplay"
                        visible: window.overlayDisplaySelectionRequired && window.overlayPreferredDisplayName !== ""
                        text: "Forget saved display"
                        activeFocusOnTab: true
                        Accessible.name: "Forget saved overlay display"
                        Accessible.role: Accessible.Button
                        Accessible.description: "Clear the unavailable saved display so another display can be selected" + (window.overlayCaptureEvidenceStale ? ". Live overlay evidence is stale until fresh capture arrives." : "")
                        ToolTip.text: "Clear the unavailable saved display and choose a replacement" + (window.overlayCaptureEvidenceStale ? "; live evidence is stale until fresh capture" : "")
                        onClicked: {
                            window.forgetUnavailableOverlayDisplay()
                            window.announceCaptureRecoveryState("Saved overlay display cleared; choose a replacement display.")
                            overlayDisplay.forceActiveFocus()
                        }
                    }
                    Label { objectName: "overlayDisplayStatus"; text: window.overlayBridge && window.overlayBridge.screenIndex >= 0 && window.overlayBridge.screenIndex < window.overlayBridge.screenNames.length ? "Current display: " + window.overlayBridge.screenNames[window.overlayBridge.screenIndex] : "Current display unavailable"; Accessible.name: "Current overlay display: " + text; Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending; live evidence stale" : "") + (window.overlayHiddenByUser ? "; hidden by user" : ""); Accessible.role: Accessible.StaticText; Accessible.description: window.overlayDisplayRecoveryPending ? text + ". The saved display is being recovered; live overlay evidence is stale until the overlay returns." : (window.overlayHiddenByUser ? text + ". The overlay is hidden by user request; use Restore overlay when ready." : text + ". The overlay destination is ready."); ToolTip.text: "Current overlay display status" }
                    Label {
                        objectName: "overlayVisibilityStatus"
                        visible: window.overlayBridge !== null
                        text: visible ? (window.overlayHiddenByUser ? "Overlay visibility: hidden by user" : "Overlay visibility: available") : "Overlay visibility unavailable"
                        Accessible.name: "Overlay visibility status"
                        Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : "")
                        Accessible.role: Accessible.Status
                        Accessible.description: visible && window.overlayHiddenByUser
                            ? text + ". Use Restore overlay to return the persisted overlay without losing monitoring or invalidating live evidence."
                            : visible ? text + ". Hide overlay preserves monitoring and live evidence, then restores the control window." + (window.overlayCaptureEvidenceStale ? " Live evidence is stale until fresh capture arrives." : "") : text
                        ToolTip.text: visible ? Accessible.description : "Overlay visibility status"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        objectName: "overlayRecoveryActionStatus"
                        visible: window.overlayBridge !== null
                        text: visible
                            ? (window.overlayDisplayRecoveryPending ? "Overlay recovery: waiting for a compatible display · live evidence stale" : (window.overlayHiddenByUser ? "Overlay recovery: restore is available" : "Overlay recovery: ready"))
                            : "Overlay recovery unavailable"
                        Accessible.name: "Overlay recovery action status" + (window.overlayCaptureEvidenceStale ? ", evidence stale until fresh capture" : "")
                        Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; evidence stale until fresh capture" : "")
                        Accessible.role: Accessible.Status
                        Accessible.description: visible && window.overlayDisplayRecoveryPending
                            ? text + ". Keep the control window available; live overlay evidence is stale until you choose or reconnect a display before restoring."
                            : visible && window.overlayHiddenByUser ? text + ". Restore overlay uses the persisted display, size, and opacity." : text
                        ToolTip.text: visible ? Accessible.description : "Overlay recovery status"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        objectName: "overlayDisplayEmptyState"
                        visible: window.overlayBridge && window.overlayBridge.screenNames.length === 0
                        text: "No displays are available for the overlay."
                        Accessible.name: "Overlay display empty state, live evidence stale until fresh capture"
                        Accessible.value: text + "; live evidence stale until fresh capture"
                        Accessible.role: Accessible.Status
                        Accessible.description: text + " Live overlay evidence is stale while no display is available; monitoring continues in the control window. Connect a display before choosing an overlay destination."
                        ToolTip.text: "Connect a display before choosing an overlay destination"
                    }
                    Label {
                        objectName: "overlayMissingDisplayRecovery"
                        visible: window.overlayDisplaySelectionRequired && window.overlayPreferredDisplayName !== ""
                        text: "Saved display unavailable: " + window.overlayPreferredDisplayName + ". Connect it or choose another display."
                        Accessible.name: "Overlay saved display recovery, live evidence stale until fresh capture: " + text
                        Accessible.value: text + "; live overlay evidence stale"
                        Accessible.role: Accessible.Status
                        Accessible.description: "The native overlay bridge saved a display that is not currently enumerated. Live overlay evidence is stale while the saved display is missing; monitoring continues in the control window."
                        ToolTip.text: "Reconnect the saved display or choose another available display"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label { text: "Size"; Accessible.name: "Overlay size control"; Accessible.role: Accessible.Heading; ToolTip.text: "Choose the overlay window size" }
                    ComboBox {
                        objectName: "overlaySize"
                    Accessible.name: "Overlay size"; Accessible.value: currentText + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : ""); Accessible.role: Accessible.ComboBox; Accessible.description: "Choose the overlay window size; current size " + currentText + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : ""); ToolTip.text: "Choose Compact, Medium, or Large overlay size"
                        model: ["Compact", "Medium", "Large"]; currentIndex: window.overlayPreferredSizeIndex
                        enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.screenNames.length > 0
                        onActivated: if (window.overlayBridge) {
                            const sizes = [[250, 110], [310, 130], [430, 180]]
                            const requestedIndex = currentIndex
                            const resized = window.overlayBridge.resizeOverlay(
                                sizes[requestedIndex][0], sizes[requestedIndex][1])
                            if (resized) {
                                window.overlayPreferredSizeIndex = requestedIndex
                                window.overlaySettingsRevision += 1
                                window.focusOverlaySizeRecovery("DJ overlay size set to " + currentText + ".")
                            } else {
                                window.syncOverlaySettingsStateFromBridge()
                                currentIndex = window.overlayPreferredSizeIndex
                                window.focusOverlaySizeRecovery("DJ overlay size could not be changed; persisted size remains active.")
                            }
                        }
                    }
                }
                Label { visible: window.overlayBridge !== null && !window.overlayBridge.available; text: "Overlay unavailable on this display."; objectName: "overlayUnavailable"; Accessible.name: "Overlay availability: " + text; Accessible.value: text + "; live overlay evidence stale"; Accessible.role: Accessible.Status; Accessible.description: text + " Live overlay evidence is stale; monitoring continues in the control window while you choose another display or retry."; ToolTip.text: "Native overlay is unavailable on the selected display"; wrapMode: Text.WordWrap }
                Label {
                    objectName: "overlayFallbackState"
                    visible: window.overlayBridge !== null && (window.overlayLaunchFallbackActive || !window.overlayBridge.available)
                    text: window.overlayBridge.available
                        ? "Control window remains available as the fallback while the overlay restores. Review persisted settings, then use Restore overlay."
                        : "Control window remains available as the tray/taskbar fallback until the overlay display recovers. Use Retry overlay after reconnecting a display."
                    Accessible.name: "Overlay fallback state"
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: text
                    ToolTip.text: "Tray/taskbar fallback keeps controls available while the overlay is unavailable"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Button {
                    objectName: "overlayFallbackReviewAction"
                    visible: window.overlayBridge !== null && (window.overlayLaunchFallbackActive || window.overlayDisplayRecoveryPending)
                    text: window.overlayBridge && window.overlayBridge.available ? "Review recovery" : "Review recovery when display returns"
                    activeFocusOnTab: true
                    Accessible.name: text
                    Accessible.value: window.overlayDisplayRecoveryPending ? "recovery pending" : "fallback active"
                    Accessible.role: Accessible.Button
                    Accessible.description: "Focus the persisted overlay settings summary and review the next recovery action."
                    ToolTip.text: Accessible.description
                    onClicked: {
                        overlaySettingsSummary.forceActiveFocus()
                        window.announceCaptureRecoveryState("Overlay recovery workflow focused; review persisted display, size, opacity, and recovery state.")
                    }
                }
                Label {
                    objectName: "overlayRecoveryStep"
                    visible: window.overlayBridge !== null && window.overlayDisplayRecoveryPending
                    text: window.overlayBridge.available
                        ? "Recovery steps: 1. Review settings  →  2. Restore overlay"
                        : "Recovery steps: 1. Reconnect a display  →  2. Retry overlay"
                    Accessible.name: "Overlay recovery steps"
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: text
                    ToolTip.text: text
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "overlayGeometryStatus"
                    text: window.overlayBridge && window.overlayBridge.geometry && window.overlayBridge.geometry.width > 0
                        ? "Overlay position/size: " + window.overlayBridge.geometry.x + "," + window.overlayBridge.geometry.y + " · " + window.overlayBridge.geometry.width + "×" + window.overlayBridge.geometry.height
                        : "Overlay geometry unavailable; control-window fallback remains active."
                    Accessible.name: "Overlay geometry status: " + text + (window.overlayCaptureEvidenceStale ? " (live evidence stale until fresh capture)" : window.overlayDisplayRecoveryPending ? " (recovery pending)" : "")
                    Accessible.value: text + "; geometry revision " + window.overlayGeometryRevision + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "; recovery pending" : "") + (window.overlayHiddenByUser ? "; hidden by user" : "")
                    Accessible.role: Accessible.Status
                    Accessible.description: window.overlayBridge && window.overlayBridge.geometry && window.overlayBridge.geometry.width > 0
                        ? "Persisted overlay position and size: " + text + (window.overlayDisplayRecoveryPending ? ". Recovery is pending; settings are retained." : "") + (window.overlayHiddenByUser ? " The overlay is hidden by user request." : ".")
                        : "Persisted overlay position and size unavailable; control-window fallback remains active and settings are retained."
                    ToolTip.text: window.overlayDisplayRecoveryPending
                        ? "Current persisted overlay position and size; recovery is pending"
                        : "Current persisted overlay position and size"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "overlayRestoreState"
                    text: window.overlayBridge && window.overlayBridge.displayName !== ""
                        ? "Persisted restore: " + window.overlayBridge.displayName + " · " + ["Compact", "Medium", "Large"][window.overlayPreferredSizeIndex] + " · " + (window.overlayBridge.geometry ? window.overlayBridge.geometry.x + "," + window.overlayBridge.geometry.y + " · " + window.overlayBridge.geometry.width + "×" + window.overlayBridge.geometry.height : "geometry unavailable") + " · " + Math.round(window.overlayBridge.opacity * 100) + "% opacity" + (window.overlayDisplayRecoveryPending ? " · recovery pending" : "")
                        : "Persisted restore state unavailable; control-window fallback remains active."
                    Accessible.name: "Overlay persisted restore state: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: "Restored overlay display, size, and opacity, including whether recovery is pending, or the fallback state when native restore data is unavailable."
                    ToolTip.text: "Persisted display, size, opacity, and recovery state from the native overlay bridge"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "overlaySettingsSummary"
                        activeFocusOnTab: true
                    text: "Overlay settings: "
                        + (window.overlayBridge && window.overlayBridge.displayName !== "" ? window.overlayBridge.displayName : "display unavailable")
                        + " · " + ["Compact", "Medium", "Large"][window.overlayPreferredSizeIndex]
                        + " · " + (window.overlayBridge ? Math.round(window.overlayBridge.opacity * 100) : 90) + "% opacity"
                        + (window.overlayHiddenByUser ? " · hidden by user" : "")
                        + (window.overlayLaunchFallbackActive ? " · launch fallback active" : "")
                        + " · settings revision " + window.overlaySettingsRevision
                        + (window.overlayDisplayRecoveryPending ? " · recovery pending · settings retained" : (window.overlayLaunchFallbackActive ? " · settings retained" : ""))
                    Accessible.name: "Overlay settings summary, " + (window.overlayCaptureEvidenceStale ? "live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")
                    Accessible.value: text + (window.overlayCaptureEvidenceStale ? "; live evidence stale until fresh capture" : "")
                    Accessible.role: Accessible.Status
                    Accessible.description: "Current overlay display, size, opacity, and settings revision. "
                        + (window.overlayCaptureEvidenceStale
                            ? "Live overlay evidence is stale until fresh capture arrives; persisted settings remain available."
                            : window.overlayDisplayRecoveryPending
                            ? "Recovery is pending; persisted settings are retained while the display is restored."
                            : (window.overlayLaunchFallbackActive
                                ? "The overlay is using its launch fallback while persisted settings remain available."
                                : "The overlay is ready with persisted settings applied."))
                    ToolTip.text: window.overlayCaptureEvidenceStale
                        ? "Overlay live evidence is stale until fresh capture; persisted display settings are retained"
                        : window.overlayDisplayRecoveryPending
                        ? "Overlay recovery pending; persisted display settings are retained"
                        : (window.overlayLaunchFallbackActive
                            ? "Overlay launch fallback active; persisted display settings are available"
                            : "Current overlay display, size, opacity, and recovery state")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                    Button {
                        objectName: "overlaySettingsSummaryReviewAction"
                        visible: window.overlayBridge !== null
                        text: window.overlayLaunchFallbackActive ? "Review recovery" : "Review overlay summary"
                        activeFocusOnTab: true
                        Accessible.name: text + ", " + (window.overlayCaptureEvidenceStale ? "live evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")
                        Accessible.value: (window.overlayCaptureEvidenceStale ? "live overlay evidence stale until fresh capture" : window.overlayDisplayRecoveryPending ? "recovery pending; live overlay evidence stale" : "available") + "; settings revision " + window.overlaySettingsRevision
                        Accessible.role: Accessible.Button
                        Accessible.description: "Focus the consolidated overlay settings summary for display, size, opacity, and recovery state; "
                            + (window.overlayCaptureEvidenceStale ? "live overlay evidence is stale until fresh capture; " : window.overlayDisplayRecoveryPending ? "recovery is pending and live overlay evidence is stale; " : "recovery is ready; ")
                            + "current settings revision " + window.overlaySettingsRevision + "."
                        ToolTip.text: Accessible.description
                        onClicked: {
                            overlaySettingsSummary.forceActiveFocus()
                            window.announceCaptureRecoveryState((window.overlayLaunchFallbackActive ? "Overlay recovery reviewed" : "Overlay summary reviewed") + "; settings revision " + window.overlaySettingsRevision + (window.overlayCaptureEvidenceStale ? ". Live evidence is stale until fresh capture arrives." : window.overlayDisplayRecoveryPending ? ", recovery pending." : "."))
                        }
                    }
                Label {
                    objectName: "overlayPersistedRecoveryDiagnostic"
                    visible: window.overlayBridge !== null && !window.overlayBridge.available
                        && typeof storageRecovery !== "undefined"
                        && storageRecovery.analysisAdmissionResult !== undefined
                        && storageRecovery.analysisAdmissionResult !== ""
                        && !window.overlayPersistedDiagnosticDismissed
                        && (storageRecovery.analysisStatus === "Analysis failed" || storageRecovery.analysisStatus === "Analysis cancelled")
                    text: visible ? "Persisted analysis admission result (not current visualization): " + storageRecovery.analysisAdmissionResult : ""
                    Accessible.name: "Overlay persisted recovery diagnostic (historical, live evidence stale until fresh capture): " + text
                    Accessible.value: text + "; live evidence stale until fresh capture"
                    Accessible.role: Accessible.Status
                    Accessible.description: "Persisted bridge feedback is historical and must not be treated as the current visualization result. Live evidence is stale until fresh capture arrives."
                    ToolTip.text: "Historical persisted analysis admission feedback"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Label { id: overlaySafeModeStatus; objectName: "overlaySafeModeStatus"; visible: window.overlayRecoveryAttemptCount >= window.overlayRecoveryAttemptLimit; text: "Safe mode active · monitoring continues without the native overlay"; Accessible.name: "Overlay safe mode status, live evidence stale until fresh capture"; Accessible.role: Accessible.Status; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "Native overlay retries are paused after the limit; live overlay evidence is stale until the overlay is restored and fresh capture arrives; clear safe mode to try again."; ToolTip.text: "Native overlay recovery is paused; monitoring remains available"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Label { id: overlaySafeModeShortcutHint; objectName: "overlaySafeModeShortcutHint"; visible: overlaySafeModeStatus.visible; text: "Shortcut: Ctrl+Alt+Shift+C clears safe mode"; Accessible.name: "Overlay safe mode shortcut hint"; Accessible.role: Accessible.StaticText; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "Keyboard shortcut for clearing native overlay safe mode; live evidence is stale until fresh capture arrives"; ToolTip.text: "Clear safe mode from the keyboard"; Layout.fillWidth: true }
                Label { id: overlayRecoveryAttemptStatus; objectName: "overlayRecoveryAttemptStatus"; visible: window.overlayRecoveryAttemptCount > 0 && window.overlayRecoveryAttemptCount < window.overlayRecoveryAttemptLimit; text: "Recovery attempts: " + window.overlayRecoveryAttemptCount + "/" + window.overlayRecoveryAttemptLimit + " · live overlay evidence stale"; Accessible.name: "Overlay recovery attempt status, live evidence stale until fresh capture"; Accessible.role: Accessible.Status; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "Bounded native overlay recovery attempt progress; live overlay evidence is stale until the overlay is restored and fresh capture arrives."; ToolTip.text: "Native overlay recovery attempt progress; overlay evidence is stale"; Layout.fillWidth: true }
                Label { id: overlayRecoveryPausedStatus; objectName: "overlayRecoveryPausedStatus"; visible: window.overlayRecoveryPausedByUser; text: "Recovery paused · monitoring continues · live overlay evidence stale"; Accessible.name: "Overlay recovery paused status, live evidence stale until fresh capture"; Accessible.role: Accessible.Status; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "Native overlay recovery is paused by user choice; live overlay evidence is stale until recovery succeeds and fresh capture arrives; start another retry when ready."; ToolTip.text: "Recovery is paused by user choice; monitoring remains available; overlay evidence is stale until fresh capture"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Button { id: resumeOverlayRecovery; objectName: "resumeOverlayRecovery"; visible: window.overlayRecoveryPausedByUser; text: "Resume recovery"; activeFocusOnTab: true; enabled: window.overlayBridge && window.overlayBridge.screenNames.length > 0 && !window.overlayDisplaySelectionRequired; Accessible.name: "Resume overlay recovery"; Accessible.role: Accessible.Button; Accessible.description: "Resume native overlay recovery with a new bounded attempt while keeping monitoring available; live evidence is stale until fresh capture arrives"; ToolTip.text: "Resume native overlay recovery; live evidence is stale until fresh capture"; onClicked: window.handleOverlayResumeRecoveryAction() }
                Label { id: overlayResumeRecoveryShortcutHint; objectName: "overlayResumeRecoveryShortcutHint"; visible: window.overlayRecoveryPausedByUser; text: "Shortcut: Ctrl+Alt+Shift+R resumes recovery"; Accessible.name: "Overlay resume recovery shortcut hint"; Accessible.role: Accessible.StaticText; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "Keyboard shortcut for resuming native overlay recovery; live evidence is stale until fresh capture arrives"; ToolTip.text: "Resume recovery from the keyboard; live evidence is stale until fresh capture"; Layout.fillWidth: true }
                Label { id: overlayPausedRecoveryPageShortcutHint; objectName: "overlayPausedRecoveryPageShortcutHint"; visible: window.overlayRecoveryPausedByUser; text: "Shortcut: Ctrl+Alt+Shift+Pause opens recovery"; Accessible.name: "Overlay paused recovery page shortcut hint"; Accessible.role: Accessible.StaticText; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "Keyboard shortcut for opening the paused overlay recovery page; live evidence is stale until fresh capture arrives"; ToolTip.text: "Open recovery from the keyboard"; Layout.fillWidth: true }
                Label { id: overlayRecoveryPausedDisplayStatus; objectName: "overlayRecoveryPausedDisplayStatus"; visible: window.overlayRecoveryPausedByUser && window.overlayDisplayRecoveryPending; text: "Display recovery pending · resume remains manual"; Accessible.name: "Overlay paused display recovery status, live evidence stale until fresh capture"; Accessible.role: Accessible.Status; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "A display is being recovered while native overlay retries remain paused by user choice; live evidence is stale until fresh capture arrives."; ToolTip.text: "Display recovery is pending; resume recovery when ready; live evidence is stale until fresh capture"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Label { id: overlayStopRecoveryShortcutHint; objectName: "overlayStopRecoveryShortcutHint"; visible: overlayRecoveryAttemptStatus.visible; text: "Shortcut: Ctrl+Alt+Shift+X stops recovery"; Accessible.name: "Overlay stop recovery shortcut hint"; Accessible.role: Accessible.StaticText; Accessible.value: text + "; live evidence stale until fresh capture"; Accessible.description: "Keyboard shortcut for stopping native overlay recovery; live evidence is stale until fresh capture arrives"; ToolTip.text: "Stop recovery from the keyboard"; Layout.fillWidth: true }
                Button { id: stopOverlayRecovery; visible: window.overlayRecoveryAttemptCount > 0 && window.overlayRecoveryAttemptCount < window.overlayRecoveryAttemptLimit; text: "Stop recovery"; objectName: "stopOverlayRecovery"; activeFocusOnTab: true; enabled: true; Accessible.name: "Stop overlay recovery"; Accessible.role: Accessible.Button; Accessible.description: "Pause native overlay recovery attempts while keeping monitoring available; live overlay evidence remains stale until recovery succeeds"; ToolTip.text: "Stop retrying and keep monitoring; overlay evidence remains stale"; onClicked: window.handleOverlayStopRecoveryAction() }
                Button { id: retryOverlay; visible: window.overlayBridge !== null && !window.overlayBridge.available; text: "Retry overlay"; objectName: "retryOverlay"; activeFocusOnTab: true; enabled: window.overlayBridge && window.overlayBridge.screenNames.length > 0 && !window.overlayDisplaySelectionRequired && window.overlayRecoveryAttemptCount < window.overlayRecoveryAttemptLimit; Accessible.name: "Retry DJ overlay"; Accessible.role: Accessible.Button; Accessible.value: enabled ? "available; attempt " + (window.overlayRecoveryAttemptCount + 1) : (window.overlayRecoveryAttemptCount >= window.overlayRecoveryAttemptLimit ? "safe mode after " + window.overlayRecoveryAttemptLimit + " attempts" : (window.overlayDisplaySelectionRequired ? "select a display first" : "waiting for a display")); Accessible.description: enabled ? "Retry showing the native DJ overlay on the selected display; attempt " + (window.overlayRecoveryAttemptCount + 1) : (window.overlayRecoveryAttemptCount >= window.overlayRecoveryAttemptLimit ? "Native overlay retry limit reached; reconnect a display or use settings to leave safe mode" : (window.overlayDisplaySelectionRequired ? "Select an output display before retrying the native DJ overlay" : "Connect or restore a display before retrying the native DJ overlay")); ToolTip.text: enabled ? "Retry the native DJ overlay on the selected display" : (window.overlayRecoveryAttemptCount >= window.overlayRecoveryAttemptLimit ? "Retry limit reached; overlay remains in safe mode" : (window.overlayDisplaySelectionRequired ? "Select a display before retrying the overlay" : "Waiting for a display before retrying the overlay")); onClicked: window.handleOverlayRetryButtonAction() }
                Button { id: clearOverlaySafeMode; visible: window.overlayRecoveryAttemptCount >= window.overlayRecoveryAttemptLimit; text: "Clear safe mode"; objectName: "clearOverlaySafeMode"; activeFocusOnTab: true; enabled: true; Accessible.name: "Clear overlay safe mode"; Accessible.role: Accessible.Button; Accessible.description: "Clear the native overlay retry limit and make Retry overlay available again"; ToolTip.text: "Clear safe mode and allow another overlay retry"; onClicked: window.handleOverlayClearSafeModeAction() }
                Item { Layout.fillHeight: true }
            }
        }
        Item {
            objectName: "settingsPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 14
                Label { text: "Settings"; font.pixelSize: 28; font.bold: true; color: window.themeForegrounds[window.selectedThemeIndex]; Accessible.name: "Settings"; Accessible.role: Accessible.Heading; ToolTip.text: "Audio capture, appearance, and overlay settings" }
                GroupBox {
                    title: "Audio capture"
                    Accessible.name: "Audio capture settings"
                    Accessible.value: (window.endpointSelectionBridge ? window.endpointSelectionBridge.endpoints.length : 0) + " output endpoints" + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Choose the output endpoint and device used for live monitoring. " + (window.overlayDisplayRecoveryPending ? "Overlay recovery is pending." : "Overlay is ready.")
                    ToolTip.text: "Configure live audio capture sources; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent; spacing: 8
                        Label { text: "Output device"; font.bold: true; Accessible.name: "Output device settings, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready"); Accessible.role: Accessible.Heading; ToolTip.text: "Choose the system output device used for monitoring; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready") }
                            RowLayout {
                                ComboBox {
                                    objectName: "endpointSelector"
                                    activeFocusOnTab: true
                                    model: window.endpointSelectionBridge ? window.endpointSelectionBridge.endpoints : []
                                    textRole: "name"
                                    enabled: count > 0; Layout.fillWidth: true
                                    Accessible.name: "Output capture endpoint"
                                    Accessible.value: currentText + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready") + (window.captureMonitorBridge && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring") ? "; live evidence stale" : "")
                                    Accessible.role: Accessible.ComboBox
                                    Accessible.description: (currentText !== "" ? "Current capture endpoint: " + currentText + ". Choose the output endpoint used for live capture." : "No capture endpoint selected. Choose the output endpoint used for live capture.") + (window.overlayDisplayRecoveryPending ? " Overlay recovery is pending." : " Overlay is ready.") + (window.captureMonitorBridge && (window.captureMonitorBridge.status === "Error" || window.captureMonitorBridge.status === "Reacquiring") ? " Live overlay evidence is stale until fresh capture arrives." : "")
                                    ToolTip.text: "Choose the output endpoint used for live capture; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")
                                    onActivated: { window.overlayCaptureEvidenceStale = true; if (window.endpointSelectionBridge) window.endpointSelectionBridge.select(currentValue.id); if (window.captureMonitorBridge && window.captureMonitorBridge.start) window.captureMonitorBridge.start(); window.announceVisualizationRecovery("Output endpoint selected; monitoring restarted and prior evidence is stale until the new endpoint provides a fresh accepted snapshot."); endpointSelector.forceActiveFocus() }
                                }
                                Button { text: "Refresh"; objectName: "refreshEndpoints"; activeFocusOnTab: true; enabled: window.endpointSelectionBridge !== null; Accessible.name: "Refresh capture endpoints"; Accessible.role: Accessible.Button; Accessible.description: "Refresh the available output capture endpoints." + (window.overlayDisplayRecoveryPending ? " Overlay recovery is pending." : " Overlay is ready."); ToolTip.text: "Refresh available output capture endpoints; " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready"); onClicked: { window.endpointSelectionBridge.refresh(); window.announceVisualizationRecovery("Output endpoint refresh requested; focus remains on endpoint recovery controls."); endpointSelector.forceActiveFocus() } }
                            }
                            Label {
                                objectName: "endpointEmptyState"
                                visible: window.endpointSelectionBridge && window.endpointSelectionBridge.endpoints.length === 0
                                text: "No output endpoints detected. Refresh after connecting an output device."
                                Accessible.name: "Output endpoint empty state"
                                Accessible.value: text
                                Accessible.role: Accessible.Status
                                Accessible.description: text
                                ToolTip.text: "No output endpoint is available; refresh after connecting a device"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label { text: window.endpointSelectionBridge ? window.endpointSelectionBridge.status : "Output selection unavailable"; objectName: "endpointStatus"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Output endpoint status: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current output endpoint discovery status" }
                            Label {
                                objectName: "endpointUnavailableState"
                                visible: window.endpointSelectionBridge === null
                                text: "Output endpoint discovery is unavailable in this build."
                                Accessible.name: "Output endpoint discovery availability"
                                Accessible.value: text
                                Accessible.role: Accessible.Status
                                Accessible.description: text
                                ToolTip.text: "Output endpoint discovery is unavailable in this build"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label {
                                objectName: "endpointRecoveryGuidance"
                                visible: window.endpointSelectionBridge && window.endpointSelectionBridge.status.indexOf("Error") >= 0
                                text: "Output endpoint recovery is available: connect an output device and refresh the endpoint list."
                                Accessible.name: "Output endpoint recovery guidance"
                                Accessible.value: text
                                Accessible.role: Accessible.Status
                                Accessible.description: text
                                ToolTip.text: "Connect an output device and refresh endpoint discovery"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label {
                                objectName: "endpointRecoveryAnnouncement"
                                text: window.endpointSelectionBridge && window.endpointSelectionBridge.status.indexOf("Error") >= 0
                                    ? "Endpoint retry failed; check device permission and connect an output device before refreshing."
                                    : ""
                                visible: text !== ""
                                Accessible.name: "Endpoint recovery announcement: " + text
                                Accessible.value: text
                                Accessible.role: Accessible.Status
                                Accessible.description: "Live output-device permission and retry status."
                                ToolTip.text: "Output endpoint retry and permission status"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label { text: window.endpointSelectionBridge && window.endpointSelectionBridge.selectedEndpoint !== "" ? "Selected endpoint: " + window.endpointSelectionBridge.selectedEndpoint : "No output endpoint selected"; objectName: "selectedEndpointStatus"; Accessible.name: "Selected output endpoint: " + text; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Current selected output endpoint"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        ComboBox {
                            id: outputDevice
                            objectName: "outputDevice"
                            activeFocusOnTab: true
                            model: ["Default system output"]
                            currentIndex: 0
                            enabled: true
                            Accessible.name: "Output device"
                            Accessible.value: currentText
                            Accessible.role: Accessible.ComboBox
                            Accessible.description: "Current output device: " + currentText + ". Monitoring follows the operating system default output device."
                            ToolTip.text: "Current system output device used for monitoring"
                            Layout.fillWidth: true
                        }
                        Label {
                            objectName: "permissionGuidance"
                            text: Qt.platform.os === "osx"
                                  ? "On macOS 14.2 or later, allow Beatglass in System Settings → Privacy & Security → Screen & System Audio Recording, then relaunch Beatglass before monitoring."
                                  : "System audio capture permission is not available yet; monitoring will remain stopped until a device is detected."
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                            Accessible.name: "Audio capture permission guidance"
                            Accessible.value: text
                            Accessible.role: Accessible.Status
                            Accessible.description: text
                            ToolTip.text: "Audio capture permission and recovery guidance"
                        }
                    }
                }
                GroupBox {
                    title: "Appearance"
                    Accessible.name: "Appearance settings"
                    Accessible.value: window.selectedTheme + ", " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced motion" : "full motion")
                    Accessible.role: Accessible.Grouping
                    Accessible.description: "Choose the color theme and motion preference for the desktop. Current settings: " + window.selectedTheme + ", " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced motion" : "full motion") + "."
                    ToolTip.text: "Configure theme and motion preferences; current: " + window.selectedTheme + ", " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced motion" : "full motion")
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent; spacing: 8
                        Label { text: "Choose a color theme or enable reduced motion from the toolbar."; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Appearance guidance"; Accessible.value: text; Accessible.role: Accessible.Status; Accessible.description: text; ToolTip.text: "Theme and reduced-motion guidance" }
                        Label {
                            objectName: "appearanceState"
                            text: "Theme: " + window.selectedTheme + " · Motion: " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced" : "full")
                            Accessible.name: "Current appearance settings: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.Status
                            Accessible.description: text
                            ToolTip.text: "Current theme: " + window.selectedTheme + "; motion: " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced" : "full")
                            Layout.fillWidth: true
                        }
                    }
                }
                Label {
                    objectName: "settingsStatus"
                    text: "Settings are saved for this desktop profile. User audio files are never modified."
                    Accessible.name: "Settings status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.Status
                    Accessible.description: "Settings persistence status and user-audio safety guarantee."
                    ToolTip.text: "Settings persistence and source-file safety"
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
    Connections {
        target: window.metadataBridge
        function onStatusChanged(text) { window.metadataStatus = text }
    }
    Timer {
        id: metadataRetryCooldown
        interval: 750
        repeat: false
        onTriggered: window.metadataRetryPending = false
    }

    FolderDialog {
        id: folderPicker
        title: "Choose a folder to browse"
        currentFolder: window.browseFolder
        onAccepted: {
            window.browseFolder = selectedFolder
            window.catalogDiagnostic = ""
            search.text = ""
            window.recursiveCatalogActive = false
            window.recursiveCatalogRows = []
            window.resetLibraryBrowserSelection("Library folder selected; choose an audio file to inspect or analyze.")
        }
        onRejected: window.handleLibraryFolderChooserRejected()
    }
}

























