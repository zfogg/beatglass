import QtQuick
import QtTest
import "../../src/desktop/qml"

TestCase {
    name: "Navigation"
    when: windowShown
    width: 900
    height: 620
    QtObject {
        id: fakeMetadata
        property url lastFile
        signal statusChanged(string status)
        function inspect(url) { lastFile = url }
    }
    QtObject {
        id: fakeLive
        property string status: "Stopped"
        property string tempo: "—"
        property string key: "—"
        property string chord: "—"
        signal changed()
        onStatusChanged: changed()
        onTempoChanged: changed()
        onKeyChanged: changed()
        onChordChanged: changed()
    }
    QtObject {
        id: fakeCapture
        property string status: "Monitoring"
        property int starts: 0
        property int stops: 0
        signal changed()
        onStatusChanged: changed()
        function stop() { stops++; status = "Stopped" }
        function start() { starts++; status = "Monitoring" }
    }
    QtObject {
        id: fakePlayback
        property string state: "Stopped"
        property url source: ""
        property int epoch: 1
        property int currentFrame: 0
        property int opens: 0
        function openSource(url, nextEpoch) { source = url; epoch = nextEpoch; state = "Playing"; opens++; return true }
        function seek(frame, nextEpoch) { currentFrame = frame; epoch = nextEpoch; return true }
        function pause() { state = "Paused" }
        function resume() { state = "Playing" }
    }
    QtObject {
        id: fakeAnalysis
        property string status: "Queued"
        property int frames: 0
        property bool canCancel: false
        property bool canRetry: false
        property int batchAcceptedCount: 0
        property int batchRejectedCount: 0
        property int batchCapacity: 0
        property int cancels: 0
        property int retries: 0
        property int batchCancels: 0
        property int batchRetries: 0
        function cancel() { cancels++ ; status = "Cancelled"; canCancel = false; canRetry = true }
        function retry() { retries++; status = "Queued"; canRetry = false; canCancel = true }
        function cancelBatch() { batchCancels++; status = "Cancelled" }
        function retryBatch() { batchRetries++; status = "Queued" }
    }
    QtObject {
        id: fakeThemePreferences
        property int themeIndex: 0
        property bool reduceMotion: false
    }
    QtObject {
        id: fakeOverlay
        property bool available: true
        property bool clickThrough: false
        property real opacity: 0.9
        property int attaches: 0
        property int shows: 0
        property int hides: 0
        property var screenNames: ["Display"]
        signal screensChanged()
        onScreenNamesChanged: screensChanged()
        property string displayName: "Display"
        property int screenIndex: -1
        property int selectedScreen: -1
        property int requestedWidth: 0
        property int requestedHeight: 0
        property var geometry: ({ x: 120, y: 80, width: 430, height: 240 })
        property bool refuseClickThrough: false
        property bool refuseHide: false
        property bool refuseUnlock: false
        property bool refuseShow: false
        function attach(overlay, control) { attaches++; return true }
        function showOverlay() { shows++; return !refuseShow }
        function hideOverlay() { hides++; if (refuseHide) return false; clickThrough = false; return true }
        function setClickThrough(value) { if ((value && refuseClickThrough) || (!value && refuseUnlock)) return false; clickThrough = value; return true }
        function setOpacity(value) { opacity = value }
        function moveToScreen(index) { selectedScreen = index; screenIndex = index; displayName = screenNames[index]; return true }
        function resizeOverlay(width, height) { requestedWidth = width; requestedHeight = height; geometry = ({ x: geometry.x, y: geometry.y, width: width, height: height }); return true }
    }
    Component { id: shell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive } }
    Component { id: shellWithPlayback; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackBridge: fakePlayback } }
    Component { id: shellWithAnalysis; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; analysisJobBridge: fakeAnalysis } }
    Component { id: shellWithOverlay; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; overlayBridge: fakeOverlay; captureMonitorBridge: fakeCapture } }
    Component { id: shellWithOverlayWithoutCapture; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; overlayBridge: fakeOverlay } }
    Component { id: shellWithOverlayPreferences; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; overlayBridge: fakeOverlay; captureMonitorBridge: fakeCapture; themePreferencesBridge: fakeThemePreferences } }
    Component { id: shellWithPreferences; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; themePreferencesBridge: fakeThemePreferences } }
    function test_overlayControlsUseBridge() {
        fakeOverlay.attaches = 0; fakeOverlay.shows = 0; fakeOverlay.hides = 0; fakeOverlay.clickThrough = false; fakeOverlay.refuseClickThrough = false; fakeOverlay.selectedScreen = 0; fakeOverlay.screenIndex = 0; fakeOverlay.displayName = "Display"; fakeOverlay.requestedWidth = 0
        let app = shellWithOverlay.createObject(null)
        verify(app !== null); compare(fakeOverlay.attaches, 1)
        findChild(app, "navigation").currentIndex = 1
        let show = findChild(app, "showOverlay")
        let toggle = findChild(app, "unlockOverlay")
        let unlock = findChild(app, "unlockOverlay")
        let hide = findChild(app, "hideOverlay")
        tryVerify(() => show.visible && show.enabled)
        show.clicked(); compare(fakeOverlay.shows, 1); tryCompare(app, "visible", false)
        verify(app.handleOverlayClickThroughButtonAction(true)); verify(fakeOverlay.clickThrough); verify(unlock.enabled)
        verify(unlock.Accessible.description.indexOf("Allow pointer input to pass through the overlay window; currently enabled") === 0)
        verify(app.handleOverlayClickThroughButtonAction(false)); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        fakeOverlay.refuseClickThrough = true
        toggle.checked = true; toggle.clicked(); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        let display = findChild(app, "overlayDisplay")
        let size = findChild(app, "overlaySize")
        verify(toggle.Accessible.description.indexOf("Allow pointer input to pass through the overlay window; currently disabled") === 0)
        verify(!toggle.ToolTip || toggle.ToolTip.text.indexOf("Toggle pointer click-through for the overlay") === 0)
        verify(findChild(app, "overlayOpacity") !== null)
        verify(display !== null)
        verify(size !== null)
        verify(display !== null); verify(size !== null)
        fakeOverlay.opacity = 0.6; tryCompare(findChild(app, "overlayOpacity"), "value", 0.6)
        verify(findChild(app, "overlayOpacity") !== null)
        hide.clicked(); compare(fakeOverlay.hides, 1); tryCompare(app, "visible", true)
        app.destroy()
    }
    function test_captureReacquiringFocusesLiveMonitoringState() {
        let app = shellWithOverlay.createObject(null)
        fakeCapture.status = "Reacquiring"
        verify(app.overlayCaptureEvidenceStale)
        let monitoring = findChild(app, "overlayLiveMonitoringState")
        tryVerify(() => monitoring.activeFocus)
        verify(monitoring.Accessible.description.indexOf("evidence is stale until fresh capture") >= 0)
        tryVerify(() => app.overlayRecoveryAnnouncement.indexOf("reacquiring") >= 0)
        fakeCapture.status = "Monitoring"
        app.destroy()
        wait(0)
    }
    function test_stoppingOverlayMonitoringMarksEvidenceStale() {
        let app = shellWithOverlay.createObject(null)
        let action = findChild(app, "overlayMonitoringAction")
        fakeCapture.status = "Monitoring"
        action.clicked()
        verify(app.overlayCaptureEvidenceStale)
        verify(action.Accessible.value.indexOf("evidence stale until fresh capture") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("monitoring stopped") >= 0)
        app.destroy()
    }
    function test_overlayWithoutCaptureBridgeStartsWithStaleEvidence() {
        let app = shellWithOverlayWithoutCapture.createObject(null)
        verify(app.overlayCaptureEvidenceStale)
        verify(app.overlayRecoveryAnnouncement.indexOf("capture is unavailable") >= 0)
        app.destroy()
    }
    function test_captureStartingMarksOverlayEvidenceStale() {
        let app = shellWithOverlay.createObject(null)
        fakeCapture.status = "Starting"
        verify(app.overlayCaptureEvidenceStale)
        tryVerify(() => findChild(app, "overlayLiveMonitoringState").activeFocus)
        tryVerify(() => app.overlayRecoveryAnnouncement.indexOf("capture starting") >= 0)
        fakeCapture.status = "Monitoring"
        app.destroy()
        wait(0)
    }
    function test_monitoringActionRejectsStaleTransitionActivation() {
        let app = shellWithOverlay.createObject(null)
        let action = findChild(app, "overlayMonitoringAction")
        fakeCapture.starts = 0
        fakeCapture.stops = 0
        fakeCapture.status = "Reacquiring"
        action.clicked()
        compare(fakeCapture.starts, 0)
        compare(fakeCapture.stops, 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("no duplicate capture request") >= 0)
        fakeCapture.status = "Monitoring"
        app.destroy()
    }
    function test_liveAnalysisTransitionInvalidatesOverlayEvidence() {
        let app = shellWithOverlay.createObject(null)
        fakeCapture.status = "Monitoring"
        fakeLive.status = "Starting"
        verify(app.overlayCaptureEvidenceStale)
        verify(app.overlayRecoveryAnnouncement.indexOf("starting") >= 0)
        fakeLive.status = "Tracking"
        verify(!app.overlayCaptureEvidenceStale)
        verify(app.overlayRecoveryAnnouncement.indexOf("fresh accepted evidence") >= 0)
        fakeCapture.status = "Monitoring"
        fakeLive.status = "Stopped"
        app.destroy()
    }
    function test_visualizationRetryMarksOverlayEvidenceStale() {
        let app = shellWithOverlay.createObject(null)
        fakeLive.status = "Error"
        fakeCapture.starts = 0
        let retry = findChild(app, "visualizationRetryCapture")
        verify(retry.visible)
        retry.clicked()
        verify(app.overlayCaptureEvidenceStale)
        compare(fakeCapture.starts, 1)
        verify(app.overlayRecoveryAnnouncement.indexOf("Visualization capture retry requested") >= 0)
        fakeCapture.status = "Monitoring"
        fakeLive.status = "Tracking"
        app.destroy()
    }
    function test_resumeRecoveryRejectsStaleActivation() {
        let app = shellWithOverlay.createObject(null)
        app.overlayRecoveryPausedByUser = false
        verify(!app.handleOverlayResumeRecoveryAction())
        verify(app.overlayRecoveryAnnouncement.indexOf("no longer paused") >= 0)
        app.destroy()
    }
    function test_hideRejectsAlreadyHiddenActivation() {
        let app = shellWithOverlay.createObject(null)
        fakeOverlay.hides = 0
        app.overlayHiddenByUser = true
        verify(!app.handleOverlayHideAction())
        compare(fakeOverlay.hides, 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("already hidden") >= 0)
        app.destroy()
        app.overlayHiddenByUser = false
    }
    function test_opacityResetRejectsDuplicateInFlightActivation() {
        let app = shellWithOverlay.createObject(null)
        app.overlayOpacityEditInFlight = true
        let before = fakeOverlay.opacity
        verify(!app.handleOverlayOpacityResetAction(0.9))
        compare(fakeOverlay.opacity, before)
        verify(app.overlayRecoveryAnnouncement.indexOf("already pending") >= 0)
        app.destroy()
    }
    function test_geometryAnnouncementRejectsMalformedSnapshot() {
        let app = shellWithOverlay.createObject(null)
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: NaN })
        verify(!app.announceOverlayGeometryState())
        verify(app.overlayDisplayRecoveryPending)
        verify(app.overlayDisplaySelectionRequired)
        verify(app.overlayRecoveryAnnouncement.indexOf("geometry changed") >= 0)
        app.destroy()
        fakeOverlay.geometry = ({ x: 120, y: 80, width: 430, height: 240 })
    }
    function test_overlayAvailabilityFailureRestoresControls() {
        fakeOverlay.hides = 0; fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        verify(app !== null)
        let show = findChild(app, "showOverlay")
        show.clicked(); tryCompare(app, "visible", false)
        fakeOverlay.available = false
        tryCompare(fakeOverlay, "hides", 1)
        tryCompare(app, "visible", true)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_closingShellHidesOverlay() {
        fakeOverlay.hides = 0
        let app = shellWithOverlay.createObject(null)
        verify(app !== null)
        app.close()
        compare(fakeOverlay.hides, 1)
        app.destroy()
    }
    function test_captureErrorOffersRecoveryCopy() {
        fakeCapture.status = "Error"
        let app = shellWithOverlay.createObject(null)
        verify(app !== null)
        compare(findChild(app, "overlayCaptureStatus").text, "Capture error — retry monitoring")
        findChild(app, "navigation").currentIndex = 1
        compare(findChild(app, "captureStatus").text, "Capture error — retry monitoring")
        app.destroy()
        fakeCapture.status = "Monitoring"
    }
    function test_captureStartingDisablesDuplicateRequests() {
        fakeCapture.status = "Starting"
        let app = shellWithOverlay.createObject(null)
        verify(app !== null)
        let toggle = findChild(app, "toggleMonitoring")
        compare(toggle.text, "Starting…")
        verify(!toggle.enabled)
        app.destroy()
        fakeCapture.status = "Monitoring"
    }
    function test_captureReacquiringDisablesDuplicateRequests() {
        fakeCapture.status = "Reacquiring"
        let app = shellWithOverlay.createObject(null)
        verify(app !== null)
        let toggle = findChild(app, "toggleMonitoring")
        compare(toggle.text, "Reacquiring…")
        verify(!toggle.enabled)
        app.destroy()
        fakeCapture.status = "Monitoring"
    }
    function test_destinationsAndMetadata() {
        let app = shell.createObject(null)
        verify(app !== null)
        let tabs = findChild(app, "navigation")
        let library = findChild(app, "libraryPage")
        let dj = findChild(app, "djPage")
        verify(library.visible)
        verify(!dj.visible)
        tabs.currentIndex = 1
        tryCompare(dj, "visible", true)
        verify(!library.visible)
        fakeMetadata.statusChanged("WAV | 44100 Hz")
        tabs.currentIndex = 0
        tryCompare(library, "visible", true)
        compare(findChild(app, "metadataStatus").text, "WAV | 44100 Hz")
        app.destroy()
    }

    function test_catalogStatusDescribesBrowseWorkflow() {
        let app = shell.createObject(null)
        let status = findChild(app, "catalogStatus")
        verify(status !== null)
        compare(status.Accessible.name, "Library catalog status")
        compare(status.text, "Choose a folder to browse local audio files.")
        app.destroy()
    }

    function test_playbackSeekControlsArePresent() {
        let app = shell.createObject(null)
        verify(findChild(app, "seekBackPlayback") !== null)
        verify(findChild(app, "seekForwardPlayback") !== null)
        compare(findChild(app, "playbackPosition").Accessible.name, "Playback position")
        compare(findChild(app, "playbackState").Accessible.name, "Playback state")
        compare(findChild(app, "playbackSource").Accessible.name, "Playback source")
        compare(findChild(app, "nativePlaybackError").ToolTip.text, "Native playback error details")
        compare(findChild(app, "playbackSeekLimitation").ToolTip.text, "Playback seek currently uses source audio frames")
        compare(findChild(app, "playbackSelectionStatus").ToolTip.text, "Whether playback matches the selected library source")
        compare(findChild(app, "playbackPosition").ToolTip.text, "Current source frame and playback epoch")
        compare(findChild(app, "playbackSource").ToolTip.text, "Current playback source")
        compare(findChild(app, "playbackState").ToolTip.text, "Current native playback state")
        compare(findChild(app, "reopenSelectedPlayback").ToolTip.text, "Replace playback with the selected audio source")
        compare(findChild(app, "retryNativePlayback").ToolTip.text, "Retry native playback after the output error")
        compare(findChild(app, "playbackSampleRate").ToolTip.text, "Choose the native playback sample rate in hertz")
        compare(findChild(app, "playbackChannels").ToolTip.text, "Choose the native playback channel count")
        compare(findChild(app, "startNativePlayback").Accessible.description, "Native playback is unavailable in this build")
        compare(findChild(app, "startNativePlayback").ToolTip.text, "Start native playback using the selected format")
        compare(findChild(app, "playbackDevice").ToolTip.text, "Choose the output device used for native playback")
        compare(findChild(app, "openPlayback").Accessible.description, "Select an audio source before opening playback")
        compare(findChild(app, "seekForwardPlayback").Accessible.description, "Seek playback forward five seconds")
        compare(findChild(app, "seekBackPlayback").ToolTip.text, "Seek backward 5 seconds")
        compare(findChild(app, "seekForwardPlayback").ToolTip.text, "Seek forward 5 seconds")
        compare(findChild(app, "pausePlayback").ToolTip.text, "Pause or resume playback")
        compare(findChild(app, "stopPlayback").ToolTip.text, "Stop playback and clear the current source")
        compare(findChild(app, "seekFrame").Accessible.name, "Seek target frame")
        compare(findChild(app, "seekToFrame").Accessible.description, "Seek playback to the entered audio frame")
        compare(findChild(app, "seekToFrame").ToolTip.text, "Seek playback to the entered source frame")
        compare(findChild(app, "seekFrame").ToolTip.text, "Enter a source frame, then choose Seek")
        app.destroy()
    }
    function test_playbackPauseShortcutTracksPlaybackState() {
        fakePlayback.state = "Playing"
        let app = shellWithPlayback.createObject(null)
        let shortcut = findChild(app, "playbackPauseShortcut")
        verify(shortcut !== null)
        compare(shortcut.sequence, "Ctrl+Alt+Space")
        verify(shortcut.enabled)
        fakePlayback.state = "Stopped"
        verify(!shortcut.enabled)
        fakePlayback.state = "Paused"
        verify(shortcut.enabled)
        app.destroy()
    }
    function test_playbackDeviceSelectionControlsArePresent() {
        verify(findChild(app, "playbackDevice") !== null)
        compare(findChild(app, "playbackDevice").Accessible.name, "Playback output device")
        compare(findChild(app, "refreshPlaybackDevices").Accessible.description, "Refresh native playback output devices")
        compare(findChild(app, "refreshPlaybackDevices").ToolTip.text, "Refresh the available playback output devices")
    }
    function test_nativePlaybackStartControlsArePresent() {
        verify(findChild(app, "startNativePlayback") !== null)
        compare(findChild(app, "playbackSampleRate").Accessible.name, "Playback sample rate")
        compare(findChild(app, "playbackChannels").Accessible.name, "Playback channels")
        verify(findChild(app, "nativePlaybackState") !== null)
        verify(findChild(app, "nativePlaybackError") !== null)
        verify(findChild(app, "retryNativePlayback") !== null)
    }

    function test_frameSeekUsesPlaybackBridge() {
        fakePlayback.state = "Playing"; fakePlayback.currentFrame = 0; fakePlayback.epoch = 1
        let app = shellWithPlayback.createObject(null)
        let frame = findChild(app, "seekFrame")
        frame.value = 12345
        findChild(app, "seekToFrame").clicked()
        compare(fakePlayback.currentFrame, 12345)
        compare(fakePlayback.epoch, 2)
        app.destroy()
    }

    function test_playbackSourceAndPositionAreVisible() {
        let app = shell.createObject(null)
        verify(findChild(app, "playbackSource") !== null)
        verify(findChild(app, "playbackPosition") !== null)
        app.destroy()
    }

    function test_playbackStateAndSourceAreAccessible() {
        let app = shell.createObject(null)
        compare(findChild(app, "playbackState").Accessible.name, "Playback state")
        compare(findChild(app, "playbackSource").Accessible.name, "Playback source")
        let unavailable = findChild(app, "playbackUnavailable")
        verify(unavailable.visible)
        compare(unavailable.Accessible.name, "Playback availability")
        app.destroy()
    }

    function test_seekPresentationDocumentsFrameBasedLimitation() {
        let app = shell.createObject(null)
        let limitation = findChild(app, "playbackSeekLimitation")
        verify(limitation !== null)
        verify(!limitation.visible)
        compare(limitation.Accessible.name, "Playback seek limitation")
        app.destroy()
    }

    function test_selectedSourceMismatchOffersRecovery() {
        fakePlayback.source = ""; fakePlayback.state = "Stopped"; fakePlayback.epoch = 1; fakePlayback.opens = 0
        let app = shellWithPlayback.createObject(null)
        app.selectedAudioUrl = Qt.resolvedUrl("../fixtures/generated/tone.flac")
        let recovery = findChild(app, "reopenSelectedPlayback")
        verify(recovery.visible)
        compare(findChild(app, "playbackSelectionStatus").text, "")
        recovery.clicked()
        compare(fakePlayback.opens, 1)
        compare(fakePlayback.source, app.selectedAudioUrl)
        compare(findChild(app, "playbackSelectionStatus").text, "Playing selected source")
        verify(!recovery.visible)
        app.destroy()
    }

    function test_djMonitoringExplainsCurrentState() {
        let app = shell.createObject(null)
        let dj = findChild(app, "djPage")
        verify(dj !== null)
        verify(dj.visible === false || findChild(app, "djStatus") !== null)
        app.destroy()
    }

    function test_captureMonitorControlsAreBound() {
        let app = shell.createObject(null)
        let toggle = findChild(app, "toggleMonitoring")
        verify(toggle !== null)
        compare(toggle.Accessible.description, "Start monitoring, or retry after a capture error.")
        verify(findChild(app, "captureStatus") !== null)
        compare(findChild(app, "captureStatus").Accessible.name, "Capture monitor status")
        app.destroy()
    }

    function test_endpointSelectionControlsAreBound() {
        let app = shell.createObject(null)
        let selector = findChild(app, "endpointSelector")
        verify(selector !== null)
        verify(selector.Accessible.description.indexOf("Choose the output endpoint used for live capture.") >= 0)
        verify(!selector.ToolTip || selector.ToolTip.text.indexOf("Choose the output endpoint used for live capture") === 0)
        verify(findChild(app, "refreshEndpoints").Accessible.description.indexOf("Refresh the available output capture endpoints.") === 0)
        verify(findChild(app, "refreshEndpoints") !== null)
        verify(findChild(app, "endpointRecoveryAnnouncement") !== null)
        verify(findChild(app, "outputDevice") !== null)
        let empty = findChild(app, "endpointEmptyState")
        verify(empty !== null)
        verify(!empty.visible)
        compare(empty.Accessible.name, "Output endpoint empty state")
        verify(empty !== null)
        let recovery = findChild(app, "endpointRecoveryGuidance")
        verify(recovery !== null)
        verify(!recovery.visible)
        compare(recovery.Accessible.name, "Output endpoint recovery guidance")
        verify(recovery !== null)
        let unavailable = findChild(app, "endpointUnavailableState")
        verify(unavailable !== null)
        compare(unavailable.Accessible.name, "Output endpoint discovery availability")
        verify(unavailable !== null)
        app.destroy()
    }

    function test_endpointSelectorUsesEndpointNames() {
        let app = shell.createObject(null)
        let selector = findChild(app, "endpointSelector")
        verify(selector !== null)
        compare(selector.textRole, "name")
        app.destroy()
    }

    function test_overlayReportsCaptureRecoveryStatus() {
        let app = shell.createObject(null)
        let status = findChild(app, "overlayCaptureStatus")
        verify(status !== null)
        compare(status.Accessible.name, "Overlay capture status")
        compare(status.ToolTip.text, "Current output capture status")
        let retry = findChild(app, "overlayRetryCapture")
        verify(retry !== null)
        verify(!retry.visible)
        compare(retry.ToolTip.text, "Retry monitoring the selected output source")
        app.destroy()
    }
    function test_captureLossMarksOverlayEvidenceStaleUntilMonitoring() {
        let app = shellWithOverlay.createObject(null)
        fakeCapture.status = "Error"
        let tempo = findChild(app, "overlayTempo")
        let captureStatus = findChild(app, "overlayCaptureStatus")
        let liveGroup = findChild(app, "overlayLiveEvidenceGroup")
        let attribution = findChild(app, "overlayEstimateAttribution")
        verify(findChild(app, "overlayCaptureRecoveryAction").activeFocus)
        verify(app.overlayCaptureEvidenceStale)
        verify(tempo.Accessible.value.indexOf("stale until fresh capture") >= 0)
        verify(captureStatus.Accessible.value.indexOf("evidence stale until fresh capture") >= 0)
        verify(liveGroup.Accessible.value.indexOf("evidence stale until fresh capture") >= 0)
        verify(attribution.Accessible.value.indexOf("evidence stale until fresh capture") >= 0)
        let retry = findChild(app, "overlayCaptureRecoveryAction")
        verify(retry.Accessible.value.indexOf("evidence stale until fresh capture") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("fresh snapshot") >= 0)
        fakeCapture.status = "Monitoring"
        verify(!app.overlayCaptureEvidenceStale)
        verify(app.overlayRecoveryAnnouncement.indexOf("fresh accepted snapshot") >= 0)
        verify(findChild(app, "overlayLiveMonitoringState").activeFocus)
        app.destroy()
    }
    function test_overlayUnavailableStateIsAccessible() {
        let app = shell.createObject(null)
        let state = findChild(app, "overlayUnavailable")
        verify(state !== null)
        compare(state.Accessible.name, "Overlay availability")
        compare(state.ToolTip.text, "Native overlay is unavailable on the selected display")
        app.destroy()
    }
    function test_overlayUnavailableOffersRetry() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        verify(retry !== null)
        verify(retry.visible)
        compare(retry.Accessible.description, "Retry showing the native DJ overlay")
        compare(retry.ToolTip.text, "Retry showing the native DJ overlay on the selected display")
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlayRetryTracksRecoveryAttemptAndResetsWhenAvailable() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        compare(app.overlayRecoveryAttemptCount, 0)
        retry.clicked()
        compare(app.overlayRecoveryAttemptCount, 1)
        fakeOverlay.available = true
        tryCompare(app, "overlayRecoveryAttemptCount", 0)
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlayRetryStopsAfterBoundedAttempts() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        compare(app.overlayRecoveryAttemptLimit, 3)
        retry.clicked(); retry.clicked(); retry.clicked()
        compare(app.overlayRecoveryAttemptCount, 3)
        verify(!retry.enabled)
        compare(retry.Accessible.value, "safe mode after 3 attempts")
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlaySafeModeCanBeClearedForAnotherRecoveryAttempt() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        let clear = findChild(app, "clearOverlaySafeMode")
        retry.clicked(); retry.clicked(); retry.clicked()
        verify(clear.visible)
        compare(clear.Accessible.description, "Clear the native overlay retry limit and make Retry overlay available again")
        clear.clicked()
        compare(app.overlayRecoveryAttemptCount, 0)
        verify(retry.enabled)
        compare(findChild(app, "navigation").currentIndex, 1)
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlaySafeModeStatusExplainsMonitoringFallback() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        retry.clicked(); retry.clicked(); retry.clicked()
        let status = findChild(app, "overlaySafeModeStatus")
        verify(status.visible)
        compare(status.text, "Safe mode active · monitoring continues without the native overlay")
        compare(status.Accessible.description, "Native overlay retries are paused after the limit; clear safe mode to try again.")
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlaySafeModeClearShortcutRestoresRetryFocus() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        retry.clicked(); retry.clicked(); retry.clicked()
        let shortcut = findChild(app, "overlayClearSafeModeShortcut")
        compare(shortcut.sequence, "Ctrl+Alt+Shift+C")
        verify(shortcut.enabled)
        shortcut.activated()
        compare(app.overlayRecoveryAttemptCount, 0)
        verify(retry.enabled)
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlaySafeModeShowsKeyboardShortcutHint() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        retry.clicked(); retry.clicked(); retry.clicked()
        let hint = findChild(app, "overlaySafeModeShortcutHint")
        verify(hint.visible)
        compare(hint.text, "Shortcut: Ctrl+Alt+Shift+C clears safe mode")
        compare(hint.Accessible.description, "Keyboard shortcut for clearing native overlay safe mode")
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlayRecoveryAttemptStatusShowsBoundedProgress() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        let status = findChild(app, "overlayRecoveryAttemptStatus")
        verify(!status.visible)
        retry.clicked()
        verify(status.visible)
        compare(status.text, "Recovery attempts: 1/3")
        compare(status.Accessible.description, "Bounded native overlay recovery attempt progress")
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlayRecoveryCanBeStoppedBeforeSafeMode() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        retry.clicked()
        let stop = findChild(app, "stopOverlayRecovery")
        verify(stop.visible)
        compare(stop.Accessible.description, "Pause native overlay recovery attempts while keeping monitoring available")
        stop.clicked()
        compare(app.overlayRecoveryAttemptCount, 0)
        verify(!stop.visible)
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlayStopRecoveryShortcutPausesActiveCycle() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        retry.clicked()
        let shortcut = findChild(app, "overlayStopRecoveryShortcut")
        compare(shortcut.sequence, "Ctrl+Alt+Shift+X")
        verify(shortcut.enabled)
        shortcut.activated()
        compare(app.overlayRecoveryAttemptCount, 0)
        verify(!shortcut.enabled)
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlayStopRecoveryShowsKeyboardShortcutHint() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        findChild(app, "retryOverlay").clicked()
        let hint = findChild(app, "overlayStopRecoveryShortcutHint")
        verify(hint.visible)
        compare(hint.text, "Shortcut: Ctrl+Alt+Shift+X stops recovery")
        compare(hint.Accessible.description, "Keyboard shortcut for stopping native overlay recovery")
        app.destroy()
        fakeOverlay.available = true
    }

    function test_overlayDisplayEmptyStateIsAccessible() {
        fakeOverlay.screenNames = []
        let app = shellWithOverlay.createObject(null)
        let empty = findChild(app, "overlayDisplayEmptyState")
        verify(empty !== null)
        verify(empty.visible)
        compare(empty.text, "No displays are available for the overlay.")
        verify(!findChild(app, "overlaySize").enabled)
        verify(!findChild(app, "clickThrough").enabled)
        verify(!findChild(app, "overlayOpacity").enabled)
        compare(empty.Accessible.name, "Overlay display empty state")
        compare(empty.ToolTip.text, "Connect a display before choosing an overlay destination")
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
    }

    function test_visualizationCanvasIsAccessible() {
        let app = shell.createObject(null)
        let canvas = findChild(app, "visualizationCanvas")
        verify(canvas !== null)
        compare(canvas.Accessible.name, "Live waveform and spectrum")
        compare(findChild(app, "visualizationStatus").Accessible.name, "Visualization data status")
        compare(findChild(app, "visualizationStatus").text, "Visualization unavailable")
        let focusRing = findChild(app, "visualizationFocusRing")
        verify(focusRing !== null)
        verify(canvas.Accessible.description.length > 0)
        verify(!focusRing.visible)
        canvas.forceActiveFocus()
        tryVerify(() => focusRing.visible)
        compare(focusRing.border.width, 3)
        let selection = findChild(app, "visualizationSelectionStatus")
        verify(selection !== null)
        compare(selection.text, "No visualization position selected")
        verify(selection.Accessible.description.indexOf("Left/Right/Home/End") >= 0)
        verify(selection.Accessible.description.length > 0)
        mouseClick(canvas, canvas.width * 0.25, canvas.height * 0.5)
        compare(selection.text, "Selected visualization position 25%")
        let clearSelection = findChild(app, "clearVisualizationSelection")
        verify(clearSelection !== null); verify(clearSelection.enabled)
        verify(clearSelection.Accessible.description.length > 0)
        clearSelection.clicked()
        compare(selection.text, "No visualization position selected")
        verify(!clearSelection.enabled)
        mouseClick(canvas, canvas.width * 0.25, canvas.height * 0.5)
        canvas.forceActiveFocus()
        keyClick(Qt.Key_Right)
        compare(selection.text, "Selected visualization position 26%")
        keyClick(Qt.Key_Home)
        compare(selection.text, "Selected visualization position 0%")
        keyClick(Qt.Key_End)
        compare(selection.text, "Selected visualization position 100%")
        let loadState = findChild(app, "visualizationLoadState")
        verify(loadState !== null)
        compare(loadState.text, "Unavailable")
        compare(loadState.Accessible.name, "Visualization load state")
        verify(loadState.Accessible.description.length > 0)
        let revision = findChild(app, "visualizationRevisionStatus")
        verify(revision !== null)
        compare(revision.text, "No visualization generation is active")
        verify(revision.Accessible.description.length > 0)
        let empty = findChild(app, "visualizationEmptyState")
        verify(empty !== null)
        verify(empty.visible)
        compare(findChild(app, "visualizationCanvas").modeHasData, false)
        compare(empty.text, "Start monitoring to populate the waveform.")
        findChild(app, "spectrumView").clicked()
        compare(empty.text, "Start monitoring to populate the FFT.")
        compare(findChild(app, "visualizationCanvas").Accessible.description,
                "Bounded frequency-domain FFT magnitudes from the current output capture.")
        app.destroy()
    }

    function test_visualizationDataStatusIsAccessible() {
        let app = shell.createObject(null)
        let status = findChild(app, "visualizationDataStatus")
        verify(status !== null)
        compare(status.Accessible.name, "Visualization data status")
        compare(findChild(app, "visualizationCaptureStatus").Accessible.name, "Visualization capture status")
        compare(findChild(app, "visualizationCaptureStatus").ToolTip.text, "Current capture state feeding waveform and FFT data")
        verify(findChild(app, "visualizationCaptureStatus").text.indexOf("Capture is stopped") === 0)
        let selected = findChild(app, "visualizationSelectedDataStatus")
        verify(selected !== null)
        compare(selected.text, "0 samples in selected waveform view")
        compare(selected.Accessible.name, "Selected visualization data status")
        compare(selected.ToolTip.text, "Data available in the selected waveform or FFT view")
        app.destroy()
    }

    function test_visualizationModeFocusRemainsVisible() {
        let app = shell.createObject(null)
        let waveform = findChild(app, "waveformView")
        let spectrum = findChild(app, "spectrumView")
        verify(waveform !== null)
        verify(spectrum !== null)
        verify(waveform.activeFocusOnTab)
        verify(spectrum.activeFocusOnTab)
        compare(waveform.background.border.width, 1)
        waveform.forceActiveFocus()
        tryCompare(waveform.background, "border.width", 3)
        compare(waveform.background.border.color, "#60a5fa")
        spectrum.forceActiveFocus()
        tryCompare(spectrum.background, "border.width", 3)
        compare(spectrum.background.border.color, "#c084fc")
        app.destroy()
    }

    function test_visualizationEmptyStateExplainsCaptureError() {
        fakeLive.status = "Error"
        let app = shell.createObject(null)
        let empty = findChild(app, "visualizationEmptyState")
        verify(empty !== null)
        verify(empty.visible)
        compare(empty.text, "Capture error; retry monitoring to populate the waveform.")
        fakeLive.status = "Stopped"
        app.destroy()
    }

    function test_visualizationCaptureStateClearIsSafeWithoutPresentationBridge() {
        let app = shell.createObject(null)
        verify(app.clearVisualizationForCaptureState() === undefined)
        fakeLive.status = "Error"
        fakeLive.status = "Stopped"
        fakeLive.status = "Reacquiring"
        fakeLive.status = "Starting"
        verify(findChild(app, "visualizationEmptyState").visible)
        app.destroy()
    }

    function test_visualizationModesHaveKeyboardShortcuts() {
        let app = shell.createObject(null)
        let canvas = findChild(app, "visualizationCanvas")
        let waveform = findChild(app, "visualizationWaveformShortcut")
        let spectrum = findChild(app, "visualizationSpectrumShortcut")
        verify(canvas !== null); verify(waveform !== null); verify(spectrum !== null)
        compare(waveform.sequence, "Ctrl+1")
        compare(spectrum.sequence, "Ctrl+2")
        compare(waveform.context, Qt.ApplicationShortcut)
        compare(spectrum.context, Qt.ApplicationShortcut)
        compare(waveform.ToolTip.text, "Show the time-domain waveform (Ctrl+1)")
        compare(spectrum.ToolTip.text, "Show the frequency-domain FFT (Ctrl+2)")
        canvas.viewMode = "spectrum"
        waveform.activated()
        compare(canvas.viewMode, "waveform")
        spectrum.activated()
        compare(canvas.viewMode, "spectrum")
        app.destroy()
    }

    function test_playbackSourceAndEpochChangesClearVisualization() {
        let app = shellWithPlayback.createObject(null)
        let empty = findChild(app, "visualizationEmptyState")
        verify(empty !== null)
        findChild(app, "visualizationCanvas").selectedPosition = 0.5
        fakePlayback.source = Qt.resolvedUrl("../fixtures/tone.flac")
        fakePlayback.epoch = 2
        fakePlayback.state = "Stopped"
        verify(empty.visible)
        compare(findChild(app, "visualizationCanvas").selectedPosition, -1)
        app.destroy()
    }

    function test_playbackFrameMappingClampsAndFloorsPositions() {
        let app = shell.createObject(null)
        compare(app.visualizationPositionToFrame(0, 44100), 0)
        compare(app.visualizationPositionToFrame(0.5, 44100), 22050)
        compare(app.visualizationPositionToFrame(1, 44100), 44100)
        compare(app.visualizationPositionToFrame(-1, 44100), 0)
        compare(app.visualizationPositionToFrame(2, 44100), 44100)
        compare(app.visualizationPositionToFrame(Number.NaN, 44100), -1)
        compare(app.visualizationPositionToFrame(0.5, -1), -1)
        app.destroy()
    }

    function test_visualizationPositionCanSeekValidatedPlayback() {
        let app = shellWithPlayback.createObject(null)
        fakePlayback.state = "Playing"
        fakePlayback.epoch = 3
        verify(app.seekPlaybackToVisualizationPosition(0.5, 44100))
        compare(fakePlayback.currentFrame, 22050)
        compare(fakePlayback.epoch, 4)
        fakePlayback.state = "Stopped"
        verify(!app.seekPlaybackToVisualizationPosition(0.5, 44100))
        app.destroy()
    }

    function test_playbackFrameCanSynchronizeVisualizationSelection() {
        let app = shell.createObject(null)
        verify(app.syncVisualizationSelectionFromPlayback(22050, 44100))
        compare(findChild(app, "visualizationSelectionStatus").text, "Selected visualization position 50%")
        verify(!app.syncVisualizationSelectionFromPlayback(1, 0))
        app.destroy()
    }

    function test_overlayBeatPulseHonorsReducedMotion() {
        let app = shell.createObject(null)
        let pulse = findChild(app, "beatPulse")
        verify(pulse !== null)
        compare(pulse.repeat, true)
        app.destroy()
    }

    function test_overlayAvailabilityLossRestoresKeyboardFocusToRetry() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Display"]
        let app = shellWithOverlayPreferences.createObject(null)
        findChild(app, "navigation").currentIndex = 1
        fakeOverlay.available = false
        let retry = findChild(app, "retryOverlay")
        tryVerify(() => retry.visible)
        tryVerify(() => retry.activeFocus)
        compare(retry.Accessible.name, "Retry DJ overlay")
        compare(retry.ToolTip.text, retry.Accessible.description)
        fakeOverlay.available = true
        app.destroy()
    }

    function test_reducedMotionStopsAndClearsOverlayPulse() {
        fakeThemePreferences.reduceMotion = false
        fakeLive.status = "Tracking"
        let app = shellWithOverlayPreferences.createObject(null)
        let pulse = findChild(app, "beatPulse")
        let fade = findChild(app, "pulseFade")
        let surface = findChild(app, "overlaySurface")
        verify(pulse !== null); verify(fade !== null); verify(surface !== null)
        fakeThemePreferences.reduceMotion = true
        tryCompare(pulse, "running", false)
        compare(fade.running, false)
        compare(surface.pulseOpacity, 0.0)
        fakeLive.status = "Stopped"
        app.destroy()
    }

    function test_libraryDetailsAvailabilityIsActionable() {
        let app = shell.createObject(null)
        let details = findChild(app, "detailsAvailability")
        verify(details !== null)
        compare(details.text, "Select an audio file to inspect its read-only details.")
        app.destroy()
    }
    function test_selectedAnalysisCanReturnToEmptyState() {
        let app = shell.createObject(null)
        app.selectedName = "stale.wav"
        app.selectedAudioUrl = Qt.resolvedUrl("../fixtures/generated/tone.flac")
        app.metadataStatus = "WAV | 44100 Hz"
        let clear = findChild(app, "clearSelectedAudio")
        verify(clear.enabled)
        verify(clear.activeFocusOnTab)
        clear.clicked()
        compare(app.selectedName, "")
        compare(app.selectedAudioUrl.toString(), "")
        compare(findChild(app, "detailsAvailability").text, "Select an audio file to inspect its read-only details.")
        compare(clear.Accessible.description, "Clear the selected audio source and return analysis details to the empty state.")
        app.destroy()
    }
    function test_metadataErrorOffersRetryRecovery() {
        let app = shell.createObject(null)
        app.selectedName = "broken.wav"
        app.selectedAudioUrl = Qt.resolvedUrl("../fixtures/generated/tone.flac")
        app.metadataStatus = "Error: unreadable audio"
        let retry = findChild(app, "retryMetadata")
        verify(retry.visible)
        verify(retry.activeFocusOnTab)
        let guidance = findChild(app, "metadataErrorGuidance")
        verify(guidance.visible)
        compare(guidance.Accessible.name, "Metadata error recovery guidance")
        compare(retry.Accessible.description, "Retry reading metadata for the selected audio source after an error.")
        retry.clicked()
        compare(fakeMetadata.lastFile, app.selectedAudioUrl)
        app.destroy()
    }
    function test_sourceWorkflowStatusIsAccessible() {
        let app = shell.createObject(null)
        compare(findChild(app, "sourceWorkflowStatus").Accessible.name, "Selected source workflow status")
        app.destroy()
    }
    function test_libraryBrowserIsAccessible() {
        let app = shell.createObject(null)
        compare(findChild(app, "browser").Accessible.name, "Folder contents")
        compare(findChild(app, "browserEmptyState").Accessible.name, "Library browser empty state")
        compare(findChild(app, "chooseFolder").Accessible.description, "Choose a local folder to browse")
        compare(findChild(app, "sortOrder").Accessible.description, "Toggle ascending or descending file-name order")
        app.destroy()
    }
    function test_libraryEmptyStateDistinguishesFolderErrors() {
        let app = shell.createObject(null)
        let empty = findChild(app, "browserEmptyState")
        let message = findChild(app, "browserEmptyMessage")
        app.browseFolder = Qt.resolvedUrl("../fixtures/does-not-exist")
        tryVerify(() => empty.visible)
        verify(message.text.indexOf("could not be read") >= 0 || message.text.indexOf("No matching") >= 0)
        app.browseFolder = ""
        tryCompare(message, "text", "Choose a folder to browse.")
        app.destroy()
    }
    function test_librarySearchExplainsFiltering() {
        let app = shell.createObject(null)
        compare(findChild(app, "search").Accessible.description, "Filter the current folder by audio or folder name")
        app.destroy()
    }
    function test_analysisControlsAreAccessible() {
        let app = shell.createObject(null)
        compare(findChild(app, "analysisProgress").Accessible.name, "Analysis progress")
        compare(findChild(app, "analysisUncertainty").Accessible.name, "Analysis uncertainty")
        verify(findChild(app, "analysisUncertainty").text.indexOf("Track key: Unknown") === 0)
        compare(findChild(app, "analysisFreshness").Accessible.name, "Analysis freshness")
        verify(findChild(app, "analysisFreshness").text.indexOf("waiting for source-linked analysis evidence") >= 0)
        compare(findChild(app, "analysisProgress").text, "Queued · select an audio source to begin analysis.")
        verify(!findChild(app, "analysisWaitingIndicator").visible)
        let retry = findChild(app, "retryAnalysis")
        verify(!retry.enabled)
        verify(retry.activeFocusOnTab)
        compare(retry.Accessible.description, "Retry is unavailable until the analysis job lifecycle bridge exposes cancellation and retry operations.")
        compare(retry.ToolTip.text, retry.Accessible.description)
        verify(findChild(app, "cancelAnalysis").activeFocusOnTab)
        verify(findChild(app, "retryAnalysisBatch").activeFocusOnTab)
        verify(findChild(app, "cancelAnalysisBatch").activeFocusOnTab)
        app.destroy()
    }
    function test_analysisWaitingIndicatorUsesRealFrameState() {
        let app = shell.createObject(null)
        app.selectedName = "waiting.wav"
        let indicator = findChild(app, "analysisWaitingIndicator")
        verify(indicator.visible)
        verify(indicator.running)
        compare(indicator.Accessible.name, "Analysis waiting indicator")
        app.destroy()
    }
    function test_analysisLifecycleControlsUseBridge() {
        fakeAnalysis.status = "Running"; fakeAnalysis.frames = 42; fakeAnalysis.canCancel = true; fakeAnalysis.canRetry = true; fakeAnalysis.cancels = 0; fakeAnalysis.retries = 0
        let app = shellWithAnalysis.createObject(null)
        app.selectedName = "source.wav"
        let progress = findChild(app, "analysisProgress")
        compare(progress.text, "Running · observed 42 frames.")
        let cancel = findChild(app, "cancelAnalysis")
        verify(cancel.visible)
        verify(cancel.enabled)
        cancel.clicked()
        compare(fakeAnalysis.cancels, 1)
        let retry = findChild(app, "retryAnalysis")
        verify(retry.enabled)
        retry.clicked()
        compare(fakeAnalysis.retries, 1)
        app.destroy()
    }
    function test_batchQueueStatusShowsCapacityAndRejectedInputs() {
        fakeAnalysis.batchAcceptedCount = 3
        fakeAnalysis.batchRejectedCount = 2
        fakeAnalysis.batchCapacity = 4
        let app = shellWithAnalysis.createObject(null)
        let status = findChild(app, "analysisBatchStatus")
        verify(status.visible)
        compare(status.text, "Batch queue · accepted 3 · rejected 2 · capacity 4")
        compare(status.Accessible.name, "Batch analysis queue status")
        app.destroy()
    }
    function test_batchRecoveryControlsUseBridge() {
        fakeAnalysis.batchAcceptedCount = 2; fakeAnalysis.batchRejectedCount = 0; fakeAnalysis.batchCapacity = 2
        fakeAnalysis.status = "Running"; fakeAnalysis.batchCancels = 0; fakeAnalysis.batchRetries = 0
        let app = shellWithAnalysis.createObject(null)
        let cancel = findChild(app, "cancelAnalysisBatch")
        verify(cancel.visible); verify(cancel.enabled); cancel.clicked(); compare(fakeAnalysis.batchCancels, 1)
        compare(cancel.ToolTip.text, cancel.Accessible.description)
        fakeAnalysis.status = "Failed"
        let retry = findChild(app, "retryAnalysisBatch")
        verify(retry.visible); verify(retry.enabled); compare(retry.ToolTip.text, retry.Accessible.description); retry.clicked(); compare(fakeAnalysis.batchRetries, 1)
        app.destroy()
    }
    function test_settingsExposeUnavailableAudioStateAndGuidance() {
        let app = shell.createObject(null)
        let tabs = findChild(app, "navigation")
        tabs.currentIndex = 2
        let settings = findChild(app, "settingsPage")
        tryCompare(settings, "visible", true)
        let device = findChild(app, "outputDevice")
        verify(device !== null)
        compare(device.count, 1)
        compare(device.currentText, "No output device detected")
        verify(!device.enabled)
        compare(device.ToolTip.text, "Current system output device used for monitoring")
        let guidance = findChild(app, "permissionGuidance")
        verify(guidance.text.length > 20)
         if (Qt.platform.os === "osx") verify(guidance.text.indexOf("14.2") >= 0)
        compare(findChild(app, "settingsStatus").text, "Settings are saved for this desktop profile. User audio files are never modified.")
        compare(findChild(app, "settingsStatus").ToolTip.text, "Settings persistence and source-file safety")
        compare(findChild(app, "resetAppearance").ToolTip.text, "Restore Light theme and full motion")
        verify(findChild(app, "resetAppearance").activeFocusOnTab)
        let appearance = findChild(app, "appearanceState")
        verify(appearance !== null)
        compare(appearance.ToolTip.text, "Current theme and motion settings")
        compare(appearance.text, "Theme: Light · Motion: full")
        fakeThemePreferences.reduceMotion = true
        tryCompare(appearance, "text", "Theme: Light · Motion: reduced")
        app.destroy()
    }
    function test_resetAppearanceRestoresDefaultsAndFocus() {
        fakeThemePreferences.themeIndex = 4
        fakeThemePreferences.reduceMotion = true
        let app = shellWithPreferences.createObject(null)
        app.selectedThemeIndex = 4
        app.selectedTheme = "Blue"
        let reset = findChild(app, "resetAppearance")
        reset.clicked()
        compare(app.selectedThemeIndex, 0)
        compare(app.selectedTheme, "Light")
        compare(fakeThemePreferences.reduceMotion, false)
        verify(reset.activeFocus)
        compare(findChild(app, "themePreferenceAnnouncement").text, "Appearance reset to Light theme with full motion.")
        app.destroy()
        fakeThemePreferences.themeIndex = 0
    }

    function test_themePreviewDescriptionIncludesMotionMode() {
        fakeThemePreferences.reduceMotion = true
        let app = shellWithPreferences.createObject(null)
        let preview = findChild(app, "themePreview4")
        verify(preview.Accessible.description.indexOf("Motion is reduced.") >= 0)
        fakeThemePreferences.reduceMotion = false
        verify(preview.Accessible.description.indexOf("Motion is full.") >= 0)
        app.destroy()
    }

    function test_themePalettesMatchDesignSystem() {
        let app = shell.createObject(null)
        compare(app.themeBackgrounds, ["#f2f4ef", "#10151c", "#000000", "#251f3e", "#102c43", "#332a1b"])
        compare(app.themeForegrounds, ["#142c2c", "#f1f6f7", "#ffffff", "#f9f5ff", "#effaff", "#fff9e8"])
        compare(app.themeAccentColors, ["#126f60", "#8ee5cc", "#8cffe2", "#d4a8ff", "#75d9ff", "#ffd45e"])
        compare(findChild(app, "reduceMotion").Accessible.description, "Reduce animation and transition effects in the desktop presentation; currently disabled")
        app.destroy()
    }

    function test_themeSelectorOffersAllSixThemes() {
        let app = shell.createObject(null)
        let selector = findChild(app, "themeSelector")
        verify(selector !== null)
        compare(selector.count, 6)
        compare(selector.Accessible.name, "Color theme")
        compare(selector.Accessible.description, "Choose one of six color themes for the Beatglass desktop; current theme Light")
        compare(selector.currentText, "Light")
        selector.currentIndex = 4; selector.activated(4)
        compare(app.selectedTheme, "Blue")
        compare(app.selectedThemeIndex, 4)
        selector.currentIndex = 2; selector.activated(2)
        compare(app.selectedTheme, "High contrast")
        compare(app.selectedThemeIndex, 2)
        app.destroy()
    }

    function test_themePreviewsExposeAccessibleChoices() {
        let app = shell.createObject(null)
        for (let i = 0; i < 6; ++i) {
            let preview = findChild(app, "themePreview" + i)
            verify(preview !== null)
            verify(preview.Accessible.name.length > 0)
            verify(preview.Accessible.description.indexOf("Select " + app.themeNames[i] + " theme;") === 0)
            verify(preview.checkable)
            verify(preview.activeFocusOnTab)
            compare(preview.Accessible.role, Accessible.RadioButton)
        }
        app.destroy()
    }

    function test_themePreviewSelectionIsExposedAsCheckedState() {
        let app = shell.createObject(null)
        let light = findChild(app, "themePreview0")
        let blue = findChild(app, "themePreview4")
        verify(light.checked)
        verify(!blue.checked)
        blue.clicked()
        compare(app.selectedThemeIndex, 4)
        verify(!light.checked)
        verify(blue.checked)
        verify(blue.Accessible.description.indexOf("selected") >= 0)
        app.destroy()
    }

    function test_djStatusAndEstimatesAreAccessible() {
        let app = shell.createObject(null)
        let status = findChild(app, "djStatus")
        let estimates = findChild(app, "djEstimates")
        let evidence = findChild(app, "djEvidenceStatus")
        let guidance = findChild(app, "monitoringGuidance")
        verify(status !== null); verify(estimates !== null)
        verify(evidence !== null)
        verify(guidance !== null)
        compare(status.Accessible.name, "DJ monitoring status")
        compare(estimates.Accessible.name, "DJ estimates")
        compare(evidence.Accessible.name, "DJ evidence status")
        compare(status.ToolTip.text, "Current DJ monitoring status")
        compare(estimates.ToolTip.text, "Current BPM, key, and chord estimates")
        compare(evidence.ToolTip.text, "Live evidence quality and recovery status")
        compare(guidance.ToolTip.text, "Monitoring guidance and next action")
        fakeLive.status = "Reacquiring"
        tryCompare(evidence, "text", "Reacquiring the selected output source; previous BPM, key, and chord evidence has been cleared.")
        fakeLive.status = "Tracking"
        verify(evidence.text.indexOf("provisional") >= 0)
        fakeLive.status = "Stopped"
        app.destroy()
    }

    function test_overlayControlsDescribeKeyboardActions() {
        let app = shell.createObject(null)
        compare(findChild(app, "showOverlay").Accessible.description, "Show the DJ overlay window")
        compare(findChild(app, "showOverlay").ToolTip.text, "Show the DJ overlay and hide the control window")
        compare(findChild(app, "unlockOverlay").Accessible.description, "Restore keyboard and pointer access to the overlay")
        compare(findChild(app, "unlockOverlay").ToolTip.text, "Restore keyboard and pointer access to the overlay")
        compare(findChild(app, "hideOverlay").Accessible.description, "Hide the DJ overlay window")
        compare(findChild(app, "hideOverlay").ToolTip.text, "Hide the DJ overlay and restore the control window")
        let overlayHide = findChild(app, "overlayWindowHide")
        verify(overlayHide !== null)
        compare(overlayHide.Accessible.description, "Hide the overlay window and restore the control window")
        app.destroy()
    }

    function test_accessibilityRolesForPreferenceSwitches() {
        let app = shell.createObject(null)
        let motion = findChild(app, "reduceMotion")
        let clickThrough = findChild(app, "clickThrough")
        compare(motion.Accessible.role, Accessible.CheckBox)
        compare(clickThrough.Accessible.role, Accessible.CheckBox)
        compare(clickThrough.Accessible.name, "Click through")
        app.destroy()
    }

    function test_escapeShortcutProvidesOverlayRecovery() {
        let app = shell.createObject(null)
        let shortcut = findChild(app, "overlayEscapeShortcut")
        verify(shortcut !== null)
        compare(shortcut.sequence, "Escape")
        compare(shortcut.context, Qt.ApplicationShortcut)
        let hint = findChild(app, "overlayEscapeHint")
        verify(hint !== null)
        compare(hint.Accessible.name, "Overlay recovery hint")
        compare(hint.text, "Press Escape to unlock overlay controls")
        app.destroy()
    }
    function test_themePreviewsSelectTheirPalette() {
        let app = shell.createObject(null)
        let previews = findChild(app, "themePreviews")
        verify(previews !== null)
        compare(previews.children.length, 6)
        let blue = findChild(app, "themePreview4")
        verify(blue !== null)
        compare(blue.color, "#10243d")
        mouseClick(blue, blue.width / 2, blue.height / 2)
        compare(app.selectedTheme, "Blue")
        compare(app.selectedThemeIndex, 4)
        let yellow = findChild(app, "themePreview5")
        verify(yellow !== null)
        yellow.forceActiveFocus()
        keyClick(Qt.Key_Return)
        compare(app.selectedTheme, "Yellow")
        compare(app.selectedThemeIndex, 5)
        app.destroy()
    }
    function test_themePreviewStripHidesInCompactHeader() {
        let app = shell.createObject(null)
        let previews = findChild(app, "themePreviews")
        verify(previews !== null)
        app.width = 700
        tryCompare(app, "compactHeader", true)
        verify(!previews.visible)
        verify(!findChild(app, "resetAppearance").visible)
        app.width = 900
        tryCompare(app, "compactHeader", false)
        verify(previews.visible)
        verify(findChild(app, "resetAppearance").visible)
        app.destroy()
    }
    function test_reducedMotionFocusRingAppearsWithoutTransition() {
        fakeThemePreferences.reduceMotion = true
        let app = shellWithPreferences.createObject(null)
        let preview = findChild(app, "themePreview4")
        let ring = findChild(app, "themePreviewFocusRing4")
        preview.forceActiveFocus()
        tryVerify(() => ring.visible)
        compare(ring.opacity, 1)
        fakeThemePreferences.reduceMotion = false
        app.destroy()
    }

    function test_themePreviewKeyboardFocusHasVisibleRing() {
        let app = shell.createObject(null)
        let preview = findChild(app, "themePreview4")
        let ring = findChild(app, "themePreviewFocusRing4")
        verify(preview !== null)
        verify(ring !== null)
        verify(!ring.visible)
        preview.forceActiveFocus()
        tryVerify(() => ring.visible)
        compare(ring.border.width, 3)
        compare(ring.border.color, app.themeAccentColors[4])
        app.destroy()
    }
    function test_externalThemeChangeAnnouncesMotionMode() {
        fakeThemePreferences.themeIndex = 4
        fakeThemePreferences.reduceMotion = true
        let app = shellWithPreferences.createObject(null)
        fakeThemePreferences.themeIndex = 5
        tryCompare(app, "selectedTheme", "Yellow")
        compare(findChild(app, "themePreferenceAnnouncement").text, "Theme preference restored: Yellow (reduced motion).")
        fakeThemePreferences.reduceMotion = false
        compare(findChild(app, "themePreferenceAnnouncement").text, "Reduced motion disabled for Yellow; standard presentation transitions restored.")
        app.destroy()
        fakeThemePreferences.themeIndex = 0
    }

    function test_themeSelectionFollowsExternalPreferenceChanges() {
        fakeThemePreferences.themeIndex = 0
        let app = shellWithPreferences.createObject(null)
        verify(app !== null)
        compare(app.selectedThemeIndex, 0)
        compare(app.selectedTheme, "Light")
        fakeThemePreferences.themeIndex = 4
        tryCompare(app, "selectedThemeIndex", 4)
        compare(app.selectedTheme, "Blue")
        compare(findChild(app, "themeSelector").currentIndex, 4)
        fakeThemePreferences.themeIndex = 99
        tryCompare(app, "selectedThemeIndex", 5)
        compare(app.selectedTheme, "Yellow")
        app.destroy()
    }
    function test_realFolderNavigationAndSelection() {
        let app = shell.createObject(null)
        let model = findChild(app, "fileModel")
        app.browseFolder = Qt.resolvedUrl("../fixtures")
        let folder = -1
        tryVerify(() => {
            for (let i = 0; i < model.count; ++i)
                if (model.get(i, "fileName") === "generated") { folder = i; return true }
            return false
        })
        verify(model.isFolder(folder))
        app.activateEntry(folder)
        tryCompare(model, "count", 3)
        let audio = -1
        for (let i = 0; i < model.count; ++i)
            if (model.get(i, "fileName") === "tone.flac") audio = i
        verify(audio >= 0)
        let sort = findChild(app, "sortOrder")
        compare(sort.text, "Name ↑")
        sort.clicked()
        compare(sort.text, "Name ↓")
        tryVerify(() => model.get(0, "fileName") === "tone.mp3")
        sort.clicked()
        compare(sort.text, "Name ↑")
        tryVerify(() => model.get(0, "fileName") === "tone.aiff")
        findChild(app, "search").text = "no-such-file"
        tryCompare(model, "count", 0)
        findChild(app, "search").text = "tone"
        tryCompare(model, "count", 3)
        let browser = findChild(app, "browser")
        browser.currentIndex = audio
        browser.forceActiveFocus()
        keyClick(Qt.Key_Return)
        compare(app.selectedName, "tone.flac")
        verify(app.selectedAudioUrl.toString().endsWith("/tone.flac"))
        compare(findChild(app, "selectedAudioSource").Accessible.name, "Selected audio source")
        compare(findChild(app, "selectedAudioDuration").text, "Duration: unknown until decoded metadata is available")
        verify(fakeMetadata.lastFile.toString().endsWith("/tone.flac"))
        app.destroy()
    }
    function test_overlayRecoveryPausedStatusAfterStop() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        let retry = findChild(app, "retryOverlay")
        retry.clicked()
        let stop = findChild(app, "stopOverlayRecovery")
        stop.clicked()
        let paused = findChild(app, "overlayRecoveryPausedStatus")
        verify(paused !== null)
        verify(paused.visible)
        compare(paused.text, "Recovery paused · monitoring continues")
        compare(paused.Accessible.description, "Native overlay recovery is paused by user choice; start another retry when ready.")
        verify(findChild(app, "resumeOverlayRecovery").activeFocus)
        compare(findChild(app, "overlayRecoveryPausedStatus").Accessible.description, "Native overlay recovery is paused by user choice; start another retry when ready.")
        verify(findChild(app, "overlayRecoveryPausedDisplayStatus") !== null)
        verify(app.overlayRecoveryPausedByUser)
        verify(findChild(app, "overlayRecoveryPausedDisplayStatus") !== null)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_overlayRecoveryCanResumeFromPausedState() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        let resume = findChild(app, "resumeOverlayRecovery")
        verify(resume.visible)
        resume.clicked()
        compare(app.overlayRecoveryAttemptCount, 1)
        verify(!app.overlayRecoveryPausedByUser)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_overlayRecoveryResumeShortcutIsDiscoverable() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        let shortcut = findChild(app, "overlayResumeRecoveryShortcut")
        verify(shortcut !== null)
        compare(shortcut.sequence, "Ctrl+Alt+Shift+R")
        verify(shortcut.enabled)
        verify(findChild(app, "overlayResumeRecoveryShortcutHint").visible)
        let focusShortcut = findChild(app, "overlayPausedRecoveryFocusShortcut")
        compare(focusShortcut.sequence, "Escape")
        verify(focusShortcut.enabled)
        fakeOverlay.clickThrough = true
        verify(!focusShortcut.enabled)
        fakeOverlay.clickThrough = false
        verify(focusShortcut.enabled)
        compare(focusShortcut.context, Qt.ApplicationShortcut)
        let pageShortcut = findChild(app, "overlayPausedRecoveryPageShortcut")
        compare(pageShortcut.sequence, "Ctrl+Alt+Shift+Pause")
        verify(pageShortcut.enabled)
        verify(findChild(app, "overlayPausedRecoveryPageShortcutHint").visible)
        compare(app.navigation.currentIndex, 1)
        pageShortcut.activated()
        compare(app.navigation.currentIndex, 1)
        verify(findChild(app, "resumeOverlayRecovery").activeFocus)
        findChild(app, "resumeOverlayRecovery").clicked()
        verify(!pageShortcut.enabled)
        verify(!findChild(app, "overlayPausedRecoveryPageShortcutHint").visible)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_overlayPausedRecoveryKeepsResumeFocusPath() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        verify(app.overlayRecoveryPausedByUser)
        verify(findChild(app, "resumeOverlayRecovery").activeFocus)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_explicitShowClearsPausedRecoveryMarker() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        verify(app.overlayRecoveryPausedByUser)
        app.showOverlayAndHideControls()
        verify(app.overlayRecoveryPausedByUser)
        fakeOverlay.available = true
        app.showOverlayAndHideControls()
        verify(!app.overlayRecoveryPausedByUser)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_unlockFailureKeepsUnlockActionFocused() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.refuseUnlock = true
        let app = shellWithOverlay.createObject(null)
        let unlock = findChild(app, "unlockOverlay")
        verify(unlock.enabled)
        verify(!app.unlockOverlayAndRestoreFocus())
        verify(unlock.activeFocus)
        verify(findChild(app, "overlayUnlockFailureStatus") !== null)
        app.destroy()
        fakeOverlay.refuseUnlock = false
        fakeOverlay.clickThrough = false
    }
    function test_clickThroughKeyboardRecoveryStatusNamesUnlockShortcut() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        findChild(app, "navigation").currentIndex = 1
        let status = findChild(app, "overlayKeyboardRecoveryStatus")
        verify(status.visible)
        verify(status.text.indexOf("Ctrl+Alt+U") >= 0)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_unlockFocusShortcutRestoresControlWindow() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        let shortcut = findChild(app, "overlayUnlockFocusShortcut")
        shortcut.activated()
        compare(app.navigation.currentIndex, 1)
        verify(app.visible)
        verify(findChild(app, "unlockOverlay").activeFocus)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_clickThroughFocusShortcutRestoresControlWindow() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = false
        let app = shellWithOverlay.createObject(null)
        let shortcut = findChild(app, "overlayClickThroughFocusShortcut")
        shortcut.activated()
        compare(app.navigation.currentIndex, 1)
        verify(app.visible)
        verify(findChild(app, "clickThrough").activeFocus)
        app.destroy()
    }
    function test_opacityFocusShortcutRestoresControlWindow() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        let shortcut = findChild(app, "overlayOpacityFocusShortcut")
        shortcut.activated()
        compare(app.navigation.currentIndex, 1)
        verify(app.visible)
        verify(findChild(app, "overlayOpacity").activeFocus)
        app.destroy()
    }
    function test_sizeFocusShortcutRestoresControlWindow() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        let shortcut = findChild(app, "overlaySizeFocusShortcut")
        shortcut.activated()
        compare(app.navigation.currentIndex, 1)
        verify(app.visible)
        verify(findChild(app, "overlaySize").activeFocus)
        app.destroy()
    }
    function test_showFocusShortcutRestoresControlWindow() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        let shortcut = findChild(app, "overlayShowFocusShortcut")
        shortcut.activated()
        verify(app.visible)
        verify(findChild(app, "showOverlay").activeFocus)
        app.destroy()
    }
    function test_restoreFocusShortcutRestoresControlWindow() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        let shortcut = findChild(app, "overlayRestoreFocusShortcut")
        shortcut.activated()
        verify(app.visible)
        verify(findChild(app, "restoreOverlay").activeFocus)
        app.destroy()
    }
    function test_displayFocusShortcutRestoresControlWindow() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        let shortcut = findChild(app, "overlayDisplayFocusShortcut")
        shortcut.activated()
        verify(app.visible)
        verify(findChild(app, "overlayDisplay").activeFocus)
        app.destroy()
    }
    function test_reapplyShortcutDoesNotCollideWithResume() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        compare(findChild(app, "overlayReapplyShortcut").sequence, "Ctrl+Alt+Shift+A")
        compare(findChild(app, "overlayResumeRecoveryShortcut").sequence, "Ctrl+Alt+Shift+R")
        app.destroy()
    }
    function test_clickThroughToggleAnnouncesRestoredInteraction() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        let toggle = findChild(app, "clickThrough")
        toggle.checked = false
        toggle.clicked()
        verify(!fakeOverlay.clickThrough)
        verify(!app.overlayDisplayRecoveryPending)
        app.destroy()
    }
    function test_hiddenOverlayRemainsHiddenAcrossVisualRecovery() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.hideOverlayAndRestoreControls()
        verify(app.overlayHiddenByUser)
        app.overlayDisplayRecoveryPending = true
        verify(app.recoverOverlayAfterDisplayChange())
        verify(app.overlayHiddenByUser)
        verify(findChild(app, "restoreOverlay").enabled)
        app.destroy()
    }
    function test_persistedOpacitySurvivesVisualRecovery() {
        fakeOverlay.available = true
        fakeOverlay.opacity = 0.42
        let app = shellWithOverlay.createObject(null)
        app.overlayDisplayRecoveryPending = true
        verify(app.recoverOverlayAfterDisplayChange())
        compare(fakeOverlay.opacity, 0.42)
        verify(app.overlaySettingsRevision > 0)
        app.destroy()
        fakeOverlay.opacity = 0.9
    }
    function test_persistedDisplayIdentitySurvivesVisualRecovery() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 1
        app.overlayDisplayRecoveryPending = true
        verify(app.recoverOverlayAfterDisplayChange())
        compare(fakeOverlay.selectedScreen, 1)
        compare(app.overlayPreferredDisplayName, "Projector")
        verify(app.overlayRecoveryAnnouncement.indexOf("Projector") >= 0)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_persistedGeometrySurvivesVisualRecovery() {
        fakeOverlay.available = true
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        let app = shellWithOverlay.createObject(null)
        app.overlayDisplayRecoveryPending = true
        verify(app.recoverOverlayAfterDisplayChange())
        compare(fakeOverlay.requestedWidth, 620)
        compare(fakeOverlay.requestedHeight, 280)
        verify(app.overlayRecoveryAnnouncement.indexOf("620 by 280") >= 0)
        app.destroy()
    }
    function test_persistedVisualBundleSurvivesDisplayRecovery() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        fakeOverlay.displayName = "Projector"
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        fakeOverlay.opacity = 0.37
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 1
        app.overlayDisplayRecoveryPending = true
        verify(app.recoverOverlayAfterDisplayChange())
        compare(fakeOverlay.selectedScreen, 1)
        compare(fakeOverlay.requestedWidth, 620)
        compare(fakeOverlay.requestedHeight, 280)
        compare(fakeOverlay.opacity, 0.37)
        verify(app.overlayRecoveryAnnouncement.indexOf("Projector") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("620 by 280") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("37 percent opacity") >= 0)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
        fakeOverlay.geometry = ({ x: 120, y: 80, width: 430, height: 240 })
        fakeOverlay.opacity = 0.9
    }
    function test_trayFallbackRestoresControlWindowWhenOverlayUnavailable() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.hide()
        verify(!app.visible)
        verify(!app.showOverlayAndHideControls())
        verify(app.visible)
        verify(app.overlayLaunchFallbackActive)
        verify(app.overlayDisplayRecoveryPending)
        verify(app.overlayRecoveryAnnouncement.indexOf("tray/taskbar fallback") >= 0)
        verify(findChild(app, "overlayDisplay").activeFocus)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_bridgeRecoveryWithMissingSettingsRestoresFallbackFocus() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.hide()
        fakeOverlay.geometry = ({ x: 0, y: 0, width: 0, height: 0 })
        fakeOverlay.available = true
        tryVerify(() => app.visible)
        verify(app.overlayLaunchFallbackActive)
        verify(findChild(app, "retryOverlay").activeFocus)
        fakeOverlay.geometry = ({ x: 120, y: 80, width: 430, height: 240 })
        app.destroy()
    }

    function test_trayFallbackRetainsPersistedDisplaySelection() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 1
        app.overlayPreferredSizeIndex = 2
        verify(!app.showOverlayAndHideControls())
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("Projector") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("next retry") >= 0)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_fallbackReviewKeepsPersistedSettingsKeyboardReady() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        verify(!app.showOverlayAndHideControls())
        let review = findChild(app, "overlayFallbackReviewAction")
        let summary = findChild(app, "overlaySettingsSummary")
        verify(review.visible)
        review.clicked()
        verify(summary.activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_overlayAvailabilityRecoveryFocusesRetainedSettingsReview() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        verify(app.overlayDisplayRecoveryPending)
        fakeOverlay.available = true
        tryCompare(app, "visible", true)
        verify(findChild(app, "overlaySettingsRetentionReviewAction").activeFocus)
        verify(app.overlayRecoveryAnnouncement.indexOf("Show, Restore, and Reapply") >= 0)
        app.destroy()
    }
    function test_retainedSettingsShortcutWorksDuringFallback() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        let shortcut = findChild(app, "overlayRetentionFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "overlaySettingsRetentionReviewAction").activeFocus)
        verify(app.overlayRecoveryAnnouncement.indexOf("Retained overlay settings review") >= 0)
        compare(app.overlayPreferredDisplayName, "Projector")
        app.destroy()
        fakeOverlay.available = true
    }
    function test_retryFocusShortcutPreservesPersistedSettings() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 1
        let shortcut = findChild(app, "overlayRetryFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "retryOverlay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        verify(app.overlayRecoveryAnnouncement.indexOf("Retry overlay control focused") >= 0)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_bridgeLossRestoresRetryFocusAndRetainsSettings() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        fakeOverlay.available = false
        tryCompare(app, "visible", true)
        verify(findChild(app, "retryOverlay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("unavailable") >= 0)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_failedOverlayShowKeepsRetryKeyboardReady() {
        fakeOverlay.available = true
        fakeOverlay.refuseShow = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        verify(!app.showOverlayAndHideControls())
        verify(findChild(app, "retryOverlay").activeFocus)
        verify(app.overlayDisplayRecoveryPending)
        compare(app.overlayPreferredDisplayName, "Projector")
        verify(app.overlayRecoveryAnnouncement.indexOf("could not be restored") >= 0)
        app.destroy()
        fakeOverlay.refuseShow = false
    }
    function test_safeModeClearRestoresRetryFocusWithoutDroppingSettings() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.overlayRecoveryAttemptCount = app.overlayRecoveryAttemptLimit
        let shortcut = findChild(app, "overlayClearSafeModeShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        compare(app.overlayRecoveryAttemptCount, 0)
        verify(findChild(app, "retryOverlay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_stopRecoveryShortcutRetainsPersistedDisplayForResume() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        findChild(app, "retryOverlay").clicked()
        findChild(app, "overlayStopRecoveryShortcut").activated()
        verify(findChild(app, "resumeOverlayRecovery").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("recovery paused") >= 0)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_resumeRecoveryShortcutKeepsPersistedSettings() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        findChild(app, "overlayResumeRecoveryShortcut").activated()
        compare(app.overlayRecoveryAttemptCount, 1)
        verify(!app.overlayRecoveryPausedByUser)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("recovery resumed") >= 0)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_pausedEscapeFocusRetainsPersistedSettings() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        let shortcut = findChild(app, "overlayPausedRecoveryFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "resumeOverlayRecovery").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("paused recovery action") >= 0)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_pausedRecoveryPageShortcutRetainsSettings() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        let shortcut = findChild(app, "overlayPausedRecoveryPageShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "resumeOverlayRecovery").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_clickThroughTakesEscapePrecedenceWithoutClearingPausedSettings() {
        fakeOverlay.available = false
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        verify(!findChild(app, "overlayPausedRecoveryFocusShortcut").enabled)
        verify(findChild(app, "overlayUnlockFocusShortcut").enabled)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
        fakeOverlay.available = true
    }
    function test_unlockShortcutKeepsPausedSettingsAvailable() {
        fakeOverlay.available = false
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        let shortcut = findChild(app, "overlayUnlockFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "unlockOverlay") !== null)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
        fakeOverlay.available = true
    }
    function test_unlockFailureRetainsPersistedSettingsForRetry() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.refuseUnlock = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        verify(!app.unlockOverlayAndRestoreFocus())
        verify(findChild(app, "unlockOverlay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("unlock") >= 0)
        app.destroy()
        fakeOverlay.refuseUnlock = false
        fakeOverlay.clickThrough = false
    }
    function test_successfulUnlockClearsRecoveryMarkersOnly() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.overlayDisplayRecoveryPending = true
        verify(app.unlockOverlayAndRestoreFocus())
        verify(!fakeOverlay.clickThrough)
        verify(!app.overlayDisplayRecoveryPending)
        verify(!app.overlayLaunchFallbackActive)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_clickThroughToggleClearsStaleRecoveryWithoutChangingSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.overlayDisplayRecoveryPending = true
        let toggle = findChild(app, "clickThrough")
        toggle.checked = false
        toggle.clicked()
        verify(!fakeOverlay.clickThrough)
        verify(!app.overlayDisplayRecoveryPending)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_clickThroughFailureRetainsPersistedSettings() {
        fakeOverlay.available = true
        fakeOverlay.refuseClickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let toggle = findChild(app, "clickThrough")
        toggle.checked = true
        toggle.clicked()
        verify(!fakeOverlay.clickThrough)
        verify(toggle.activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("could not be enabled") >= 0)
        app.destroy()
        fakeOverlay.refuseClickThrough = false
    }
    function test_clickThroughDisableFailureRetainsSettingsAndFocus() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.refuseUnlock = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let toggle = findChild(app, "clickThrough")
        toggle.checked = false
        toggle.clicked()
        verify(fakeOverlay.clickThrough)
        verify(toggle.activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("could not be disabled") >= 0)
        app.destroy()
        fakeOverlay.refuseUnlock = false
        fakeOverlay.clickThrough = false
    }
    function test_clickThroughFocusShortcutRetainsBlockedSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlayClickThroughFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "clickThrough").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(app.overlayRecoveryAnnouncement.indexOf("click-through") >= 0)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_opacityFocusShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlayOpacityFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "overlayOpacity").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_sizeFocusShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlaySizeFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "overlaySize").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_displayFocusShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlayDisplayFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "overlayDisplay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_restoreFocusShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlayRestoreFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "restoreOverlay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_showFocusShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlayShowFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "showOverlay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_hideFocusShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlayHideFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "hideOverlay").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_reapplyFocusShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlayReapplyFocusShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "reapplyOverlaySettings").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_summaryReviewShortcutRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let shortcut = findChild(app, "overlaySummaryReviewShortcut")
        verify(shortcut.enabled)
        shortcut.activated()
        verify(findChild(app, "overlaySettingsSummaryReviewAction").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_summaryReviewActionRetainsClickThroughSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        let review = findChild(app, "overlaySettingsSummaryReviewAction")
        verify(review.visible)
        review.clicked()
        verify(findChild(app, "overlaySettingsSummary").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.clickThrough = false
    }
    function test_hideFailureRestoresFocusToOverlayRecoveryControl() {
        fakeOverlay.available = true
        fakeOverlay.refuseHide = true
        let app = shellWithOverlay.createObject(null)
        let restore = findChild(app, "restoreOverlay")
        app.hideOverlayAndRestoreControls()
        verify(app.overlayDisplayRecoveryPending)
        tryVerify(() => restore.activeFocus)
        compare(findChild(app, "restoreOverlayAction").Accessible.name, "Restore overlay")
        fakeOverlay.refuseHide = false
        app.destroy()
    }

    function test_retainedSettingsReviewSurvivesUserHiddenOverlay() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.hideOverlayAndRestoreControls()
        verify(app.overlayHiddenByUser)
        let review = findChild(app, "overlaySettingsRetentionReviewAction")
        verify(review.visible)
        review.clicked()
        verify(findChild(app, "overlaySettingsRetentionStatus").activeFocus)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
    }
    function test_restoreAfterRetainedReviewUsesSavedSettings() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.hideOverlayAndRestoreControls()
        findChild(app, "overlaySettingsRetentionReviewAction").clicked()
        findChild(app, "restoreOverlay").clicked()
        verify(!app.overlayHiddenByUser)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(fakeOverlay.shows > 0)
        app.destroy()
    }
    function test_reapplyAfterRetainedReviewUsesSavedSettings() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.hideOverlayAndRestoreControls()
        findChild(app, "overlaySettingsRetentionReviewAction").clicked()
        findChild(app, "reapplyOverlaySettings").clicked()
        verify(!app.overlayHiddenByUser)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        verify(fakeOverlay.shows > 0)
        app.destroy()
    }
    function test_reapplyFailureKeepsHiddenMarkerAndRetainedSettings() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.hideOverlayAndRestoreControls()
        fakeOverlay.geometry = ({ x: 0, y: 0, width: 0, height: 0 })
        findChild(app, "reapplyOverlaySettings").clicked()
        verify(app.overlayHiddenByUser)
        verify(app.overlayDisplayRecoveryPending)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        app.destroy()
        fakeOverlay.geometry = ({ x: 120, y: 80, width: 430, height: 240 })
    }
    function test_reapplyRejectsPersistedDisplayIndexOutsideAvailableDisplays() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.screenIndex = 4
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Disconnected"
        app.hideOverlayAndRestoreControls()
        findChild(app, "reapplyOverlaySettings").clicked()
        verify(app.overlayDisplayRecoveryPending)
        verify(app.overlayDisplaySelectionRequired)
        verify(app.overlayLaunchFallbackActive)
        verify(app.overlayRecoveryAnnouncement.indexOf("no longer available") >= 0)
        app.destroy()
        fakeOverlay.screenIndex = -1
    }
    function test_captureFreshnessAnnouncesNativeRecoveryGate() {
        fakeOverlay.available = false
        let app = shellWithOverlay.createObject(null)
        app.overlayCaptureEvidenceStale = true
        app.transitionCaptureEvidence(false, "Fresh accepted capture available.", null)
        verify(app.overlayCaptureEvidenceStale)
        verify(app.overlayRecoveryAnnouncement.indexOf("native recovery completes") >= 0)
        app.destroy()
        fakeOverlay.available = true
    }
    function test_missingDisplaySelectionMarksOverlayEvidenceStale() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = []
        fakeOverlay.displayName = ""
        let app = shellWithOverlay.createObject(null)
        app.overlayCaptureEvidenceStale = false
        verify(!app.restoreOverlayDisplaySelectionFromBridge())
        verify(app.overlayDisplaySelectionRequired)
        verify(app.overlayCaptureEvidenceStale)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_reapplyGeometryRecoveryCanRetryWithoutResettingSettings() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredSizeIndex = 2
        app.hideOverlayAndRestoreControls()
        fakeOverlay.geometry = ({ x: 0, y: 0, width: 0, height: 0 })
        findChild(app, "reapplyOverlaySettings").clicked()
        verify(app.overlayHiddenByUser)
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        findChild(app, "restoreOverlay").clicked()
        verify(!app.overlayHiddenByUser)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredSizeIndex, 2)
        compare(fakeOverlay.requestedWidth, 620)
        app.destroy()
    }
    function test_overlayCloseHandlerIsDirectlyContractTestable() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        let request = Qt.createQmlObject("import QtQuick; QtObject { property bool accepted: true }", app)
        app.handleOverlayWindowClosing(request)
        compare(request.accepted, false)
        verify(app.overlayHiddenByUser)
        verify(findChild(app, "restoreOverlay").activeFocus)
        request.destroy()
        app.destroy()
    }
    function test_overlayWindowCloseRestoresControlWindowFallback() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        let overlay = findChild(app, "djOverlay")
        app.showOverlayAndHideControls()
        overlay.close()
        tryVerify(() => app.visible)
        verify(app.overlayHiddenByUser)
        verify(findChild(app, "restoreOverlay").activeFocus)
        app.destroy()
    }

    function test_displayLossReactivatesControlWindowFallback() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Display"]
        let app = shellWithOverlay.createObject(null)
        app.hide()
        fakeOverlay.screenNames = []
        tryVerify(() => app.visible)
        verify(app.overlayDisplayRecoveryPending)
        verify(app.overlayDisplaySelectionRequired)
        verify(findChild(app, "overlayDisplay").activeFocus)
        fakeOverlay.screenNames = ["Display"]
        app.destroy()
    }

    function test_pausedDisplayRecoveryRestoresControlWindowActivation() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = []
        let app = shellWithOverlay.createObject(null)
        app.overlayRecoveryPausedByUser = true
        app.overlayPreferredScreenIndex = 0
        app.overlayDisplayRecoveryPending = true
        fakeOverlay.screenNames = ["Display"]
        tryVerify(() => findChild(app, "resumeOverlayRecovery").activeFocus)
        verify(app.visible)
        verify(app.overlayRecoveryPausedByUser)
        app.destroy()
    }

    function test_displayReconnectReusesRetainedGeometrySettings() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        app.overlayDisplayRecoveryPending = true
        fakeOverlay.screenNames = []
        verify(!app.recoverOverlayAfterDisplayChange())
        compare(app.overlayPreferredDisplayName, "Projector")
        fakeOverlay.screenNames = ["Projector"]
        fakeOverlay.displayName = "Projector"
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        verify(app.recoverOverlayAfterDisplayChange())
        compare(fakeOverlay.requestedWidth, 620)
        compare(app.overlayPreferredDisplayName, "Projector")
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_displayReorderUsesSavedIdentityAndRetainsGeometry() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        compare(fakeOverlay.requestedWidth, 620)
        verify(findChild(app, "retryOverlay").activeFocus)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_pausedDisplayReorderRetainsIdentityUntilResume() {
        fakeOverlay.available = false
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        findChild(app, "retryOverlay").clicked()
        findChild(app, "stopOverlayRecovery").clicked()
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        verify(app.overlayRecoveryPausedByUser)
        verify(findChild(app, "resumeOverlayRecovery").activeFocus)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
        fakeOverlay.available = true
    }
    function test_clickThroughDisplayReorderRetainsIdentityAndUnlockFocus() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        verify(findChild(app, "unlockOverlay").activeFocus)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
        fakeOverlay.clickThrough = false
    }
    function test_unlockAfterClickThroughDisplayReorderClearsMarkers() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        app.overlayDisplayRecoveryPending = true
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        verify(fakeOverlay.clickThrough)
        verify(app.overlayDisplayRecoveryPending)
        verify(app.unlockOverlayAndRestoreFocus())
        verify(!fakeOverlay.clickThrough)
        verify(!app.overlayDisplayRecoveryPending)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        app.destroy()
        fakeOverlay.clickThrough = false
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_failedUnlockAfterDisplayReorderKeepsStaleMarkers() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.refuseUnlock = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        app.overlayDisplayRecoveryPending = true
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        verify(!app.unlockOverlayAndRestoreFocus())
        verify(findChild(app, "unlockOverlay").enabled)
        verify(app.overlayDisplayRecoveryPending)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        app.destroy()
        fakeOverlay.refuseUnlock = false
        fakeOverlay.clickThrough = false
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_successfulUnlockAfterReorderClearsPriorFailureMarkers() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.refuseUnlock = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        app.overlayDisplayRecoveryPending = true
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        verify(!app.unlockOverlayAndRestoreFocus())
        verify(app.overlayDisplayRecoveryPending)
        fakeOverlay.refuseUnlock = false
        verify(app.unlockOverlayAndRestoreFocus())
        verify(!app.overlayDisplayRecoveryPending)
        verify(!app.overlayLaunchFallbackActive)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        app.destroy()
        fakeOverlay.clickThrough = false
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_unlockThenReenableClickThroughKeepsReorderedSettings() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        verify(app.unlockOverlayAndRestoreFocus())
        let toggle = findChild(app, "clickThrough")
        toggle.checked = true
        toggle.clicked()
        verify(fakeOverlay.clickThrough)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        app.destroy()
        fakeOverlay.clickThrough = false
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_reorderedClickThroughReviewKeepsSettingsRevision() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 0
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        let toggle = findChild(app, "clickThrough")
        toggle.checked = false
        toggle.clicked()
        toggle.checked = true
        toggle.clicked()
        let revision = app.overlaySettingsRevision
        findChild(app, "overlaySettingsSummaryReviewAction").clicked()
        verify(findChild(app, "overlaySettingsSummary").activeFocus)
        verify(app.overlaySettingsRevision >= revision)
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 1)
        app.destroy()
        fakeOverlay.clickThrough = false
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_reorderedReviewAnnouncesSettingsRevision() {
        fakeOverlay.available = true
        fakeOverlay.clickThrough = true
        fakeOverlay.screenNames = ["Laptop", "Projector"]
        fakeOverlay.displayName = "Projector"
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayPreferredScreenIndex = 1
        app.overlaySettingsRevision = 4
        let status = findChild(app, "overlaySettingsRetentionStatus")
        verify(status.visible)
        verify(status.Accessible.value.indexOf("revision 4") >= 0)
        compare(app.overlayPreferredDisplayName, "Projector")
        app.destroy()
        fakeOverlay.clickThrough = false
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_recoveryAnnouncementConfirmsPersistedPositionAndSize() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Projector"]
        fakeOverlay.displayName = "Projector"
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        app.overlayDisplayRecoveryPending = true
        verify(app.recoverOverlayAfterDisplayChange())
        verify(app.overlayRecoveryAnnouncement.indexOf("position 640, 120") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("size 620 by 280 pixels") >= 0)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_hiddenRecoveryAnnouncementConfirmsGeometry() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.hideOverlayAndRestoreControls()
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        app.overlayDisplayRecoveryPending = true
        verify(app.recoverOverlayAfterDisplayChange())
        verify(app.overlayHiddenByUser)
        verify(app.overlayRecoveryAnnouncement.indexOf("position 640, 120") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("size 620 by 280 pixels") >= 0)
        app.destroy()
    }
    function test_geometryFailureNamesSavedDisplayInFallback() {
        fakeOverlay.available = true
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        fakeOverlay.geometry = ({ x: 0, y: 0, width: 0, height: 0 })
        verify(!app.restoreOverlayVisualStateFromBridge())
        verify(app.overlayLaunchFallbackActive)
        verify(app.overlayRecoveryAnnouncement.indexOf("Projector") >= 0)
        app.destroy()
        fakeOverlay.geometry = ({ x: 120, y: 80, width: 430, height: 240 })
    }
    function test_invalidPersistedOpacityUsesSafeFallback() {
        fakeOverlay.available = true
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        fakeOverlay.opacity = 0.1
        let app = shellWithOverlay.createObject(null)
        verify(app.restoreOverlayVisualStateFromBridge())
        compare(fakeOverlay.opacity, 0.9)
        verify(app.overlayRecoveryAnnouncement.indexOf("90 percent opacity") >= 0)
        app.destroy()
        fakeOverlay.opacity = 0.9
    }

    function test_invalidPersistedPositionKeepsControlFallbackVisible() {
        fakeOverlay.available = true
        fakeOverlay.geometry = ({ x: NaN, y: 120, width: 620, height: 280 })
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        verify(!app.restoreOverlayVisualStateFromBridge())
        verify(app.overlayLaunchFallbackActive)
        verify(app.overlayDisplayRecoveryPending)
        verify(app.overlayRecoveryAnnouncement.indexOf("position unavailable") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("Projector") >= 0)
        app.destroy()
        fakeOverlay.geometry = ({ x: 120, y: 80, width: 430, height: 240 })
    }

    function test_invalidPersistedSizeKeepsControlFallbackVisible() {
        fakeOverlay.available = true
        fakeOverlay.geometry = ({ x: 640, y: 120, width: NaN, height: 280 })
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Projector"
        verify(!app.restoreOverlayVisualStateFromBridge())
        verify(app.overlayLaunchFallbackActive)
        verify(app.overlayDisplayRecoveryPending)
        verify(app.overlayRecoveryAnnouncement.indexOf("size unavailable") >= 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("Projector") >= 0)
        app.destroy()
        fakeOverlay.geometry = ({ x: 120, y: 80, width: 430, height: 240 })
    }

    function test_validVisualRestoreIsDirectlyTestable() {
        fakeOverlay.available = true
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        fakeOverlay.opacity = 0.42
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredScreenIndex = 0
        verify(app.restoreOverlayVisualStateFromBridge())
        compare(fakeOverlay.requestedWidth, 620)
        compare(fakeOverlay.requestedHeight, 280)
        compare(fakeOverlay.opacity, 0.42)
        verify(app.overlayRecoveryAnnouncement.indexOf("geometry and opacity restored") >= 0)
        app.destroy()
        fakeOverlay.opacity = 0.9
    }

    function test_selectReplacementDisplayReappliesRetainedVisualState() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Missing display"
        app.overlayPreferredScreenIndex = -1
        app.overlayDisplayRecoveryPending = true
        app.overlayDisplaySelectionRequired = true
        verify(app.persistOverlayDisplaySelection(0, "Projector"))
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 0)
        compare(fakeOverlay.requestedWidth, 620)
        verify(!app.overlayDisplaySelectionRequired)
        verify(!app.overlayDisplayRecoveryPending)
        verify(!app.overlayLaunchFallbackActive)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
    function test_selectReplacementDisplayPersistsNativeDisplayIdentity() {
        fakeOverlay.available = true
        fakeOverlay.screenNames = ["Projector", "Laptop"]
        fakeOverlay.displayName = "Projector"
        fakeOverlay.geometry = ({ x: 640, y: 120, width: 620, height: 280 })
        let app = shellWithOverlay.createObject(null)
        app.overlayPreferredDisplayName = "Missing display"
        app.overlayPreferredScreenIndex = -1
        app.overlayDisplayRecoveryPending = true
        app.overlayDisplaySelectionRequired = true
        verify(app.persistOverlayDisplaySelection(0, "stale-picker-label"))
        compare(app.overlayPreferredDisplayName, "Projector")
        compare(app.overlayPreferredScreenIndex, 0)
        verify(app.overlayRecoveryAnnouncement.indexOf("Projector") >= 0)
        app.destroy()
        fakeOverlay.screenNames = ["Display"]
        fakeOverlay.displayName = "Display"
    }
}


