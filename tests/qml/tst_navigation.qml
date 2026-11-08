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
    QtObject {
        id: fakePlayback
        property string state: "Stopped"
        property url source: ""
        property int epoch: 1
        property int opens: 0
        function openSource(url, nextEpoch) { source = url; epoch = nextEpoch; state = "Playing"; opens++; return true }
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
        property int selectedScreen: -1
        property int requestedWidth: 0
        property bool refuseClickThrough: false
        function attach(overlay, control) { attaches++; return true }
        function showOverlay() { shows++; return true }
        function hideOverlay() { hides++; clickThrough = false }
        function setClickThrough(value) { if (value && refuseClickThrough) return false; clickThrough = value; return true }
        function setOpacity(value) { opacity = value }
        function moveToScreen(index) { selectedScreen = index; return true }
        function resizeOverlay(width, height) { requestedWidth = width; return true }
    }
    Component { id: shell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive } }
    Component { id: shellWithPlayback; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackBridge: fakePlayback } }
    Component { id: shellWithOverlay; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; overlayBridge: fakeOverlay } }
    Component { id: shellWithPreferences; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; themePreferencesBridge: fakeThemePreferences } }
    function test_overlayControlsUseBridge() {
        fakeOverlay.attaches = 0; fakeOverlay.shows = 0; fakeOverlay.hides = 0; fakeOverlay.clickThrough = false; fakeOverlay.refuseClickThrough = false; fakeOverlay.selectedScreen = -1; fakeOverlay.requestedWidth = 0
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
        unlock.clicked(); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        fakeOverlay.refuseClickThrough = true
        toggle.checked = true; toggle.clicked(); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        let display = findChild(app, "overlayDisplay")
        let size = findChild(app, "overlaySize")
        compare(toggle.Accessible.description, "Allow pointer input to pass through the overlay window")
        compare(findChild(app, "overlayOpacity").Accessible.description, "Adjust the overlay window opacity")
        compare(display.Accessible.description, "Choose the display where the overlay is shown")
        compare(size.Accessible.description, "Choose the overlay window size")
        compare(findChild(app, "overlayOpacityValue").Accessible.name, "Current overlay opacity")
        compare(findChild(app, "overlayInteractionMode").Accessible.name, "Overlay interaction mode")
        compare(findChild(app, "overlayDisplayStatus").Accessible.name, "Current overlay display")
        display.activated(0); compare(fakeOverlay.selectedScreen, 0)
        size.currentIndex = 2; size.activated(2); compare(fakeOverlay.requestedWidth, 430)
        hide.clicked(); compare(fakeOverlay.hides, 1); tryCompare(app, "visible", true)
        app.destroy()
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
        compare(findChild(app, "openPlayback").Accessible.description, "Open the selected audio source for playback")
        compare(findChild(app, "seekForwardPlayback").Accessible.description, "Seek playback forward five seconds")
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
        compare(findChild(app, "refreshEndpoints").Accessible.description, "Refresh the available output capture endpoints.")
        verify(findChild(app, "refreshEndpoints") !== null)
        let status = findChild(app, "endpointStatus")
        verify(status !== null)
        compare(status.Accessible.name, "Output endpoint status")
        compare(findChild(app, "selectedEndpointStatus").Accessible.name, "Selected output endpoint")
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
        app.destroy()
    }
    function test_overlayUnavailableStateIsAccessible() {
        let app = shell.createObject(null)
        let state = findChild(app, "overlayUnavailable")
        verify(state !== null)
        compare(state.Accessible.name, "Overlay availability")
        app.destroy()
    }

    function test_visualizationCanvasIsAccessible() {
        let app = shell.createObject(null)
        let canvas = findChild(app, "visualizationCanvas")
        verify(canvas !== null)
        compare(canvas.Accessible.name, "Live waveform and spectrum")
        compare(findChild(app, "visualizationStatus").Accessible.name, "Visualization data status")
        app.destroy()
    }

    function test_visualizationDataStatusIsAccessible() {
        let app = shell.createObject(null)
        let status = findChild(app, "visualizationDataStatus")
        verify(status !== null)
        compare(status.Accessible.name, "Visualization data status")
        app.destroy()
    }

    function test_overlayBeatPulseHonorsReducedMotion() {
        let app = shell.createObject(null)
        let pulse = findChild(app, "beatPulse")
        verify(pulse !== null)
        compare(pulse.repeat, true)
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
        compare(findChild(app, "analysisProgress").text, "Queued · select an audio source to begin analysis.")
        let retry = findChild(app, "retryAnalysis")
        verify(!retry.enabled)
        compare(retry.Accessible.description, "Retry is unavailable until the analysis job lifecycle bridge exposes cancellation and retry operations.")
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
        let guidance = findChild(app, "permissionGuidance")
        verify(guidance.text.length > 20)
         if (Qt.platform.os === "osx") verify(guidance.text.indexOf("14.2") >= 0)
        compare(findChild(app, "settingsStatus").text, "Settings are saved for this desktop profile. User audio files are never modified.")
        app.destroy()
    }
    function test_themeSelectorOffersAllSixThemes() {
        let app = shell.createObject(null)
        let selector = findChild(app, "themeSelector")
        verify(selector !== null)
        compare(selector.count, 6)
        compare(selector.Accessible.name, "Color theme")
        compare(selector.Accessible.description, "Choose one of six color themes for the Beatglass desktop.")
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
            compare(preview.Accessible.description, "Select " + app.themeNames[i] + " theme")
        }
        app.destroy()
    }

    function test_djStatusAndEstimatesAreAccessible() {
        let app = shell.createObject(null)
        let status = findChild(app, "djStatus")
        let estimates = findChild(app, "djEstimates")
        verify(status !== null); verify(estimates !== null)
        compare(status.Accessible.name, "DJ monitoring status")
        compare(estimates.Accessible.name, "DJ estimates")
        app.destroy()
    }

    function test_overlayControlsDescribeKeyboardActions() {
        let app = shell.createObject(null)
        compare(findChild(app, "showOverlay").Accessible.description, "Show the DJ overlay window")
        compare(findChild(app, "unlockOverlay").Accessible.description, "Restore keyboard and pointer access to the overlay")
        compare(findChild(app, "hideOverlay").Accessible.description, "Hide the DJ overlay window")
        app.destroy()
    }

    function test_escapeShortcutProvidesOverlayRecovery() {
        let app = shell.createObject(null)
        let shortcut = findChild(app, "overlayEscapeShortcut")
        verify(shortcut !== null)
        compare(shortcut.sequence, "Escape")
        compare(shortcut.context, Qt.ApplicationShortcut)
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
        verify(fakeMetadata.lastFile.toString().endsWith("/tone.flac"))
        app.destroy()
    }
}
