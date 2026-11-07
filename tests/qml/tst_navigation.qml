import QtQuick
import QtTest
import QtMultimedia
import "../../src/desktop/qml"

TestCase {
    name: "Navigation"
    when: windowShown
    width: 900; height: 620
    QtObject {
        id: fakeMetadata
        property url lastFile
        signal statusChanged(string status)
        function inspect(url) { lastFile = url; statusChanged(url.toString() === "" ? "Select a local file." : "Metadata for " + url) }
    }
    QtObject { id: fakeLive; property string status: "Stopped"; property string tempo: "â€”"; property string key: "â€”"; property string chord: "â€”" }
    QtObject { id: fakeGuard; property string warning: ""; function diagnostic(device) { return warning } }
    QtObject {
        id: fakeCollection
        property var rows: []; property string status: "Ready"
        property bool compositeAvailable: true; property bool analysisActive: false
        property string analysisStatus: "Ready"; property string provisionalSummary: "Track-level key: Unknown"
        property var provisionalWindows: []
        function refresh(search) { rows = [{path: "indexed.wav"}]; status = "Loaded" }
        function selectSource(url) { provisionalWindows = []; provisionalSummary = "Track-level key: Unknown" }
        function analyzeSelected() { analysisStatus = "Provisional analysis complete"; provisionalWindows = [{range:"Frames [0, 100)", key:"C", keyFrom:"0", keyStrength:"Unknown", chord:"Unknown", chordStrength:"Unknown"}]; return true }
        function cancelSelected() { analysisActive = false; provisionalWindows = [] }
    }
    QtObject { id: fakeDj; property var sources: [{name: "Test output", id: "test-id"}]; property string status: "Stopped"; property string tempo: "â€”"; property bool active: false; function refresh() {} function start(id) { active = true; status = id } function stop() { active = false; status = "Stopped"; tempo = "â€”" } }
    QtObject {
        id: fakeLifecycle
        property bool recoveryAvailable: false; property int hideRequests: 0
        signal editOverlayRequested(); signal hideOverlayRequested()
        function hideControls() { hideRequests++; return recoveryAvailable }
    }
    QtObject { id: fakeOverlay; property bool clickThrough: false; property real opacity: 1; property int endCount: 0; function setClickThrough(value) { clickThrough = value; return true } function keepOnScreen() {} function setOpacity(value) { opacity = value } function beginDrag() { return true } function dragToCursor() {} function endDrag() { endCount++ } }
    QtObject {
        id: fakeMacOverlay
        property bool available: true; property bool clickThrough: false; property real opacity: 0.9
        property int attaches: 0; property int shows: 0; property int hides: 0
        property var screenNames: ["Display"]; property int screenIndex: 0; property int selectedScreen: -1; property int requestedWidth: 0
        function attach(overlay, control) { attaches++; return true }
        function showOverlay() { shows++; return true }
        function hideOverlay() { hides++; clickThrough = false }
        function setClickThrough(value) { clickThrough = value; return true }
        function keepOnScreen() {} function setOpacity(value) { opacity = value }
        function moveToScreen(index) { selectedScreen = index; screenIndex = index; return true }
        function resizeOverlay(width, height) { requestedWidth = width; return true }
    }
    Component { id: djShell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; djBridge: fakeDj; overlayBridge: fakeOverlay; playbackGuardBridge: fakeGuard } }
    Component { id: shell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackGuardBridge: fakeGuard } }
    Component { id: collectionShell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackGuardBridge: fakeGuard; collectionBridge: fakeCollection } }
    Component { id: macShell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackGuardBridge: fakeGuard; overlayBridge: fakeMacOverlay } }
    Component { id: lifecycleShell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackGuardBridge: fakeGuard; djBridge: fakeDj; overlayBridge: fakeOverlay; lifecycleBridge: fakeLifecycle } }
    function test_overlayRequestsRecoveryAndRetainsFallback() {
        fakeLifecycle.hideRequests = 0; fakeLifecycle.recoveryAvailable = false
        fakeDj.active = true
        let app = lifecycleShell.createObject(null)
        let player = findChild(app, "player")
        player.source = Qt.resolvedUrl("../fixtures/generated/tone.flac")
        const playbackSource = player.source.toString()
        findChild(app, "navigation").currentIndex = 1
        findChild(app, "showOverlay").clicked()
        compare(fakeLifecycle.hideRequests, 1); verify(app.visible)
        verify(fakeDj.active); compare(player.source.toString(), playbackSource)
        fakeOverlay.clickThrough = true; fakeLifecycle.editOverlayRequested(); verify(!fakeOverlay.clickThrough)
        fakeLifecycle.hideOverlayRequested(); tryCompare(findChild(app, "djOverlay"), "visible", false)
        app.destroy(); fakeDj.active = false
    }
    function test_macOverlayControls() {
        fakeMacOverlay.attaches = 0; fakeMacOverlay.shows = 0; fakeMacOverlay.selectedScreen = -1; fakeMacOverlay.requestedWidth = 0
        let app = macShell.createObject(null)
        compare(fakeMacOverlay.attaches, 1)
        findChild(app, "navigation").currentIndex = 1
        findChild(app, "showOverlay").clicked(); compare(fakeMacOverlay.shows, 1)
        let display = findChild(app, "overlayDisplay"); compare(display.currentIndex, 0)
        fakeMacOverlay.screenNames = ["A", "B"]; fakeMacOverlay.screenIndex = 1; compare(display.currentIndex, 1)
        display.popup.open(); fakeMacOverlay.screenNames = ["B", "A"]; fakeMacOverlay.screenIndex = 0
        compare(display.currentIndex, 0); compare(fakeMacOverlay.selectedScreen, -1)
        display.activated(1); compare(fakeMacOverlay.selectedScreen, -1)
        display.popup.close(); fakeMacOverlay.screenNames = ["Same", "Same"]; fakeMacOverlay.screenIndex = 0
        display.openedScreens = ["Same", "Same"]; display.activated(1); compare(fakeMacOverlay.selectedScreen, -1)
        fakeMacOverlay.screenNames = ["Display"]; display.openedScreens = []; display.activated(0); compare(fakeMacOverlay.selectedScreen, 0)
        let size = findChild(app, "overlaySize"); size.currentIndex = 2; size.activated(2); compare(fakeMacOverlay.requestedWidth, 430)
        app.destroy()
    }
    function test_experimentalCollection() {
        fakeCollection.rows = []; fakeCollection.status = "Ready"
        let app = collectionShell.createObject(null)
        findChild(app, "refreshCollection").clicked()
        compare(findChild(app, "collectionStatus").text, "Loaded")
        compare(findChild(app, "collectionRows").count, 1)
        app.selectedName = "tone.aiff"
        fakeCollection.selectSource(Qt.resolvedUrl("../fixtures/generated/tone.aiff"))
        findChild(app, "analyzeSelected").clicked()
        compare(findChild(app, "provisionalWindows").count, 1)
        verify(findChild(app, "provisionalSummary").text.includes("Track-level key: Unknown"))
        fakeCollection.selectSource(Qt.resolvedUrl("../fixtures/generated/tone.flac"))
        compare(findChild(app, "provisionalWindows").count, 0)
        fakeCollection.analyzeSelected()
        compare(findChild(app, "provisionalWindows").count, 1)
        app.browseFolder = Qt.resolvedUrl("../fixtures")
        compare(app.selectedName, "")
        compare(findChild(app, "provisionalWindows").count, 0)
        app.destroy()
    }
    function test_selectionInvalidatesOnModelChanges() {
        let app = collectionShell.createObject(null)
        let model = findChild(app, "fileModel")
        let browser = findChild(app, "browser")
        app.browseFolder = Qt.resolvedUrl("../fixtures/generated")
        tryCompare(model, "count", 3)
        browser.currentIndex = 0
        compare(app.selectedName, "tone.aiff")
        fakeCollection.analyzeSelected()
        compare(findChild(app, "provisionalWindows").count, 1)
        findChild(app, "sortOrder").clicked()
        compare(browser.currentIndex, -1)
        compare(app.selectedName, "")
        compare(findChild(app, "provisionalWindows").count, 0)
        tryVerify(() => model.get(0, "fileName") === "tone.mp3")
        browser.currentIndex = 0
        verify(fakeMetadata.lastFile.toString().endsWith("/tone.mp3"))
        verify(app.metadataStatus.includes("tone.mp3"))
        fakeCollection.analyzeSelected()
        findChild(app, "search").text = "no-such-file"
        compare(browser.currentIndex, -1)
        compare(findChild(app, "provisionalWindows").count, 0)
        tryCompare(model, "count", 0)
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
    function test_djControlsAndOverlayEscape() {
        fakeDj.active = false; fakeOverlay.clickThrough = false; fakeOverlay.opacity = 0.4; fakeOverlay.endCount = 0
        let app = djShell.createObject(null)
        findChild(app, "navigation").currentIndex = 1
        let start = findChild(app, "startMonitor")
        verify(start.enabled); start.clicked(); verify(fakeDj.active); compare(fakeDj.status, "test-id")
        let show = findChild(app, "showOverlay"); show.clicked()
        compare(findChild(app, "overlayOpacity").value, 0.4)
        let overlay = findChild(app, "djOverlay"); tryCompare(overlay, "visible", true)
        let lock = findChild(app, "lockOverlay"); lock.clicked(); verify(fakeOverlay.clickThrough)
        show.clicked(); verify(!fakeOverlay.clickThrough); compare(fakeOverlay.endCount, 1); tryCompare(overlay, "visible", false)
        findChild(app, "stopMonitor").clicked(); verify(!fakeDj.active)
        app.destroy()
    }
    function test_playbackSelectionRequiresActivation() {
        let app = shell.createObject(null)
        let selector = findChild(app, "playbackOutputSelector")
        if (selector.count < 2 || selector.currentIndex < 0) { app.destroy(); skip("Two playback outputs are unavailable") }
        let original = String(app.playbackDevice.id)
        let next = (selector.currentIndex + 1) % selector.count
        selector.currentIndex = next
        compare(String(app.playbackDevice.id), original)
        selector.activated(next)
        compare(String(app.playbackDevice.id), String(selector.model[next].id))
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
        compare(sort.text, "Name â†‘")
        sort.clicked()
        compare(sort.text, "Name â†“")
        tryVerify(() => model.get(0, "fileName") === "tone.mp3")
        sort.clicked()
        compare(sort.text, "Name â†‘")
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
        verify(fakeMetadata.lastFile.toString().endsWith("/tone.flac"))
        compare(app.formatTime(123400), "2:03.4")
        fakeGuard.warning = "Unsupported output"
        compare(findChild(app, "playbackDeviceWarning").text, "Unsupported output")
        verify(!findChild(app, "playButton").enabled)
        fakeGuard.warning = ""
        let player = findChild(app, "player")
        compare(player.playbackState, MediaPlayer.StoppedState)
        tryVerify(() => player.duration >= 200, 10000)
        verify(player.seekable)
        player.position = 100
        tryVerify(() => Math.abs(player.position - 100) < 50, 5000)
        player.source = Qt.resolvedUrl("../fixtures/generated/missing.flac")
        tryVerify(() => player.error !== MediaPlayer.NoError, 10000)
        verify(findChild(app, "playbackStatus").text.startsWith("Playback error:"))
        app.destroy()
    }
}
