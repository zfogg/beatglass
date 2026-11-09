import QtQuick
import QtTest
import "../../src/desktop/qml"

TestCase {
    name: "Navigation"
    when: windowShown
    width: 900; height: 620
    QtObject {
        id: fakeMetadata
        property url lastFile
        signal statusChanged(string status)
        function inspect(url) { lastFile = url }
    }
    QtObject { id: fakeLive; property string status: "Stopped"; property string tempo: "—"; property string key: "—"; property string chord: "—" }
    QtObject { id: fakeCapture; property string status: "Monitoring" }
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
        property int screenIndex: -1
        property int selectedScreen: -1
        property int requestedWidth: 0
        property bool refuseClickThrough: false
        function attach(overlay, control) { attaches++; return true }
        function showOverlay() { shows++; return true }
        function hideOverlay() { hides++; clickThrough = false }
        function setClickThrough(value) { if (value && refuseClickThrough) return false; clickThrough = value; return true }
        function setOpacity(value) { opacity = value }
        function moveToScreen(index) { selectedScreen = index; screenIndex = index; return true }
        function resizeOverlay(width, height) { requestedWidth = width; return true }
    }
    Component { id: shell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive } }
    Component { id: shellWithPlayback; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackBridge: fakePlayback } }
    Component { id: shellWithAnalysis; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; analysisJobBridge: fakeAnalysis } }
    Component { id: shellWithOverlay; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; overlayBridge: fakeOverlay; captureMonitorBridge: fakeCapture } }
    Component { id: shellWithOverlayPreferences; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; overlayBridge: fakeOverlay; captureMonitorBridge: fakeCapture; themePreferencesBridge: fakeThemePreferences } }
    Component { id: shellWithPreferences; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; themePreferencesBridge: fakeThemePreferences } }
    function test_overlayControlsUseBridge() {
        fakeOverlay.attaches = 0; fakeOverlay.shows = 0; fakeOverlay.hides = 0; fakeOverlay.clickThrough = false; fakeOverlay.refuseClickThrough = false; fakeOverlay.selectedScreen = -1; fakeOverlay.screenIndex = -1; fakeOverlay.requestedWidth = 0
        let app = shellWithOverlay.createObject(null)
        verify(app !== null); compare(fakeOverlay.attaches, 1)
        findChild(app, "navigation").currentIndex = 1
        let show = findChild(app, "showOverlay")
        let toggle = findChild(app, "clickThrough")
        let unlock = findChild(app, "unlockOverlay")
        let hide = findChild(app, "hideOverlay")
        tryVerify(() => show.visible && show.enabled)
        show.clicked(); compare(fakeOverlay.shows, 1); tryCompare(app, "visible", false)
        toggle.checked = true; toggle.clicked(); verify(fakeOverlay.clickThrough); verify(unlock.enabled)
        compare(toggle.Accessible.description, "Allow pointer input to pass through the overlay window; currently enabled")
        unlock.clicked(); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        fakeOverlay.refuseClickThrough = true
        toggle.checked = true; toggle.clicked(); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        let display = findChild(app, "overlayDisplay")
        let size = findChild(app, "overlaySize")
        compare(toggle.Accessible.description, "Allow pointer input to pass through the overlay window; currently disabled")
        compare(toggle.ToolTip.text, "Toggle pointer click-through for the overlay")
        compare(findChild(app, "overlayOpacity").Accessible.description, "Adjust the overlay window opacity; current value 90%")
        compare(findChild(app, "overlayOpacity").ToolTip.text, "Adjust overlay opacity from 20% to 100%")
        compare(display.Accessible.description, "Choose the display where the overlay is shown; current display unavailable")
        compare(display.ToolTip.text, "Choose the display where the overlay is shown")
        compare(size.Accessible.description, "Choose the overlay window size; current size Medium")
        compare(size.ToolTip.text, "Choose Compact, Medium, or Large overlay size")
        compare(findChild(app, "overlayOpacityValue").Accessible.name, "Current overlay opacity")
        compare(findChild(app, "overlayOpacityValue").ToolTip.text, "Current overlay opacity value")
        compare(findChild(app, "overlayInteractionMode").Accessible.name, "Overlay interaction mode")
        compare(findChild(app, "overlayInteractionMode").ToolTip.text, "Current overlay pointer interaction mode")
        compare(findChild(app, "overlayDisplayStatus").Accessible.name, "Current overlay display")
        compare(findChild(app, "overlayDisplayStatus").ToolTip.text, "Current overlay display status")
        display.activated(0); compare(fakeOverlay.selectedScreen, 0); compare(display.currentIndex, 0)
        size.currentIndex = 2; size.activated(2); compare(fakeOverlay.requestedWidth, 430)
        fakeOverlay.opacity = 0.6; tryCompare(findChild(app, "overlayOpacity"), "value", 0.6)
        compare(findChild(app, "overlayOpacity").Accessible.description, "Adjust the overlay window opacity; current value 60%")
        hide.clicked(); compare(fakeOverlay.hides, 1); tryCompare(app, "visible", true)
        app.destroy()
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
        compare(selector.Accessible.description, "Choose the output endpoint used for live capture.")
        compare(selector.ToolTip.text, "Choose the output endpoint used for live capture")
        compare(findChild(app, "refreshEndpoints").Accessible.description, "Refresh the available output capture endpoints.")
        verify(findChild(app, "refreshEndpoints") !== null)
        let status = findChild(app, "endpointStatus")
        verify(status !== null)
        compare(status.Accessible.name, "Output endpoint status")
        compare(findChild(app, "selectedEndpointStatus").Accessible.name, "Selected output endpoint")
        let empty = findChild(app, "endpointEmptyState")
        verify(empty !== null)
        verify(!empty.visible)
        compare(empty.Accessible.name, "Output endpoint empty state")
        compare(empty.ToolTip.text, "No output endpoint is available; refresh after connecting a device")
        let recovery = findChild(app, "endpointRecoveryGuidance")
        verify(recovery !== null)
        verify(!recovery.visible)
        compare(recovery.Accessible.name, "Output endpoint recovery guidance")
        compare(recovery.ToolTip.text, "Connect an output device and refresh endpoint discovery")
        let unavailable = findChild(app, "endpointUnavailableState")
        verify(unavailable !== null)
        verify(unavailable.visible)
        compare(unavailable.Accessible.name, "Output endpoint discovery availability")
        compare(unavailable.ToolTip.text, "Output endpoint discovery is unavailable in this build")
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
        compare(canvas.ToolTip.text, "Click to select a position; use Left/Right/Home/End when focused")
        verify(!focusRing.visible)
        canvas.forceActiveFocus()
        tryVerify(() => focusRing.visible)
        compare(focusRing.border.width, 3)
        let selection = findChild(app, "visualizationSelectionStatus")
        verify(selection !== null)
        compare(selection.text, "No visualization position selected")
        verify(selection.Accessible.description.indexOf("Left/Right/Home/End") >= 0)
        compare(selection.ToolTip.text, "Current selected waveform/FFT position")
        mouseClick(canvas, canvas.width * 0.25, canvas.height * 0.5)
        compare(selection.text, "Selected visualization position 25%")
        let clearSelection = findChild(app, "clearVisualizationSelection")
        verify(clearSelection !== null); verify(clearSelection.enabled)
        compare(clearSelection.ToolTip.text, "Clear the selected waveform or FFT position")
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
        compare(loadState.ToolTip.text, "Current waveform/FFT presentation load state")
        let revision = findChild(app, "visualizationRevisionStatus")
        verify(revision !== null)
        compare(revision.text, "No visualization generation is active")
        compare(revision.ToolTip.text, "Current accepted visualization generation and revision")
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
        fakeAnalysis.status = "Running"; fakeAnalysis.frames = 42; fakeAnalysis.canCancel = true; fakeAnalysis.canRetry = false; fakeAnalysis.cancels = 0; fakeAnalysis.retries = 0
        let app = shellWithAnalysis.createObject(null)
        app.selectedName = "source.wav"
        let progress = findChild(app, "analysisProgress")
        compare(progress.text, "Running · observed 42 frames.")
        let cancel = findChild(app, "cancelAnalysis")
        verify(cancel.visible); verify(cancel.enabled); cancel.clicked(); compare(fakeAnalysis.cancels, 1)
        let retry = findChild(app, "retryAnalysis")
        verify(retry.enabled); retry.clicked(); compare(fakeAnalysis.retries, 1)
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
}
