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
        id: fakeOverlay
        property bool available: true
        property bool clickThrough: false
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
        function setOpacity(value) {}
        function moveToScreen(index) { selectedScreen = index; return true }
        function resizeOverlay(width, height) { requestedWidth = width; return true }
    }
    Component { id: shell; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive } }
    Component { id: shellWithOverlay; Main { visible: true; metadataBridge: fakeMetadata; liveBridge: fakeLive; overlayBridge: fakeOverlay } }
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
        show.clicked(); compare(fakeOverlay.shows, 1)
        toggle.checked = true; toggle.clicked(); verify(fakeOverlay.clickThrough); verify(unlock.enabled)
        unlock.clicked(); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        fakeOverlay.refuseClickThrough = true
        toggle.checked = true; toggle.clicked(); verify(!fakeOverlay.clickThrough); verify(!toggle.checked)
        let display = findChild(app, "overlayDisplay")
        let size = findChild(app, "overlaySize")
        display.activated(0); compare(fakeOverlay.selectedScreen, 0)
        size.currentIndex = 2; size.activated(2); compare(fakeOverlay.requestedWidth, 430)
        hide.clicked(); compare(fakeOverlay.hides, 1)
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
        app.destroy()
    }
}
