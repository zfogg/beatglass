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
        function inspect(url) { lastFile = url }
    }
    QtObject { id: fakeLive; property string status: "Stopped"; property string tempo: "—"; property string key: "—"; property string chord: "—" }
    QtObject { id: fakeGuard; property string warning: ""; function diagnostic(device) { return warning } }
    QtObject { id: fakeDj; property var sources: [{name: "Test output", id: "test-id"}]; property string status: "Stopped"; property string tempo: "—"; property bool active: false; function refresh() {} function start(id) { active = true; status = id } function stop() { active = false; status = "Stopped"; tempo = "—" } }
    QtObject { id: fakeOverlay; property bool clickThrough: false; function setClickThrough(value) { clickThrough = value; return true } function keepOnScreen() {} function setOpacity(value) {} }
    Component { id: djShell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; djBridge: fakeDj; overlayBridge: fakeOverlay; playbackGuardBridge: fakeGuard } }
    Component { id: shell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; playbackGuardBridge: fakeGuard } }
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
        fakeDj.active = false; fakeOverlay.clickThrough = false
        let app = djShell.createObject(null)
        findChild(app, "navigation").currentIndex = 1
        let start = findChild(app, "startMonitor")
        verify(start.enabled); start.clicked(); verify(fakeDj.active); compare(fakeDj.status, "test-id")
        let show = findChild(app, "showOverlay"); show.clicked()
        let overlay = findChild(app, "djOverlay"); tryCompare(overlay, "visible", true)
        let lock = findChild(app, "lockOverlay"); lock.clicked(); verify(fakeOverlay.clickThrough)
        show.clicked(); verify(!fakeOverlay.clickThrough); tryCompare(overlay, "visible", false)
        findChild(app, "stopMonitor").clicked(); verify(!fakeDj.active)
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
