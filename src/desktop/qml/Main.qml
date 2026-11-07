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
    property var metadataBridge: metadata
    property var liveBridge: livePresentation
    property var djControl: null
    property var overlayControl: null
    property url browseFolder: ""
    property bool sortDescending: false
    property string selectedName: ""
    property string metadataStatus: "Choose an audio file to inspect its metadata."

    Window {
        id: overlay
        objectName: "overlayWindow"
        width: 320; height: 110
        visible: false
        color: "transparent"
        Rectangle {
            anchors.fill: parent; radius: 10; color: "#cc181820"
            Column {
                anchors.centerIn: parent; spacing: 6
                Label { text: "Beatglass · live tempo"; color: "white" }
                Label { objectName: "overlayTempo"; text: "BPM: " + window.liveBridge.tempo; color: "white"; font.pixelSize: 24 }
            }
        }
    }

    function activateEntry(index) {
        if (index < 0 || index >= files.count) return
        const url = files.get(index, "fileUrl")
        if (files.isFolder(index)) {
            browseFolder = url
        } else {
            selectedName = files.get(index, "fileName")
            metadataBridge.inspect(url)
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
            Label { text: "Beatglass"; font.pixelSize: 20; font.bold: true; Layout.leftMargin: 16; Layout.rightMargin: 24 }
            TabBar {
                id: navigation
                objectName: "navigation"
                Layout.fillWidth: true
                TabButton { text: "Library"; objectName: "libraryTab" }
                TabButton { text: "DJ"; objectName: "djTab" }
            }
        }
    }
    StackLayout {
        anchors.fill: parent
        currentIndex: navigation.currentIndex
        Item {
            objectName: "libraryPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 10
                Label { text: "Library"; font.pixelSize: 28; font.bold: true }
                Label {
                    text: "Browse local folders. Indexing, analysis, playback and saved results are not available yet."
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "Choose folder…"; objectName: "chooseFolder"; onClicked: folderPicker.open() }
                    Button {
                        text: "Up"; objectName: "parentFolder"
                        enabled: window.browseFolder.toString() !== "" && files.parentFolder.toString() !== ""
                        onClicked: window.browseFolder = files.parentFolder
                        Accessible.description: "Go to the parent folder"
                    }
                    Label {
                        objectName: "folderPath"
                        text: window.browseFolder.toString() || "No folder selected"
                        elide: Text.ElideMiddle; Layout.fillWidth: true
                        Accessible.name: "Current folder: " + text
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField { id: search; objectName: "search"; placeholderText: "Filter file names"; Accessible.name: "Filter file names"; Layout.fillWidth: true }
                    Button { objectName: "sortOrder"; text: window.sortDescending ? "Name ↓" : "Name ↑"; Accessible.name: "Reverse name sort order"; onClicked: window.sortDescending = !window.sortDescending }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Name"; font.bold: true; Layout.fillWidth: true }
                    Label { text: "Type"; font.bold: true; Layout.preferredWidth: 100 }
                }
                Frame {
                    Layout.fillWidth: true; Layout.fillHeight: true
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
                        Keys.onReturnPressed: window.activateEntry(currentIndex)
                        Keys.onEnterPressed: window.activateEntry(currentIndex)
                        ScrollBar.vertical: ScrollBar {}
                        delegate: ItemDelegate {
                            required property int index
                            required property string fileName
                            required property bool fileIsDir
                            width: ListView.view.width
                            highlighted: ListView.isCurrentItem
                            Accessible.name: fileName + (fileIsDir ? ", folder" : ", audio file")
                            onClicked: { browser.currentIndex = index; window.activateEntry(index) }
                            contentItem: RowLayout {
                                Label { text: fileName; elide: Text.ElideMiddle; Layout.fillWidth: true }
                                Label { text: fileIsDir ? "Folder" : "Audio file"; Layout.preferredWidth: 100 }
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: !window.browseFolder.toString() || (files.status === FolderListModel.Ready && files.count === 0)
                            text: window.browseFolder.toString() ? "No matching audio files or folders, or this folder cannot be read." : "Choose a folder to browse."
                        }
                    }
                }
                Frame {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        Label { text: window.selectedName || "No file selected"; font.bold: true; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        Label { objectName: "metadataStatus"; text: window.metadataStatus; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Label { text: "Analysis details and playback are not available yet."; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    }
                }
            }
        }
        Item {
            objectName: "djPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16
                Label { text: "DJ"; font.pixelSize: 28; font.bold: true }
                Label { text: window.djControl ? "Monitor the Windows default output" : "Live output monitoring is not available yet."; font.pixelSize: 18; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Label { text: "Tempo and the overlay are experimental. Key and chords are still in development."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    visible: window.djControl !== null
                    Button { objectName: "startMonitoring"; text: "Start monitoring"; enabled: window.djControl !== null && !window.djControl.active; onClicked: window.djControl.start() }
                    Button { objectName: "stopMonitoring"; text: "Stop monitoring"; enabled: window.djControl !== null && window.djControl.active; onClicked: window.djControl.stop() }
                }
                Label { text: "Status: " + (window.djControl ? window.djControl.status : window.liveBridge.status) }
                Label { objectName: "liveEstimates"; text: "BPM: " + window.liveBridge.tempo + "    Key: " + window.liveBridge.key + "    Chord: " + window.liveBridge.chord }
                RowLayout {
                    visible: window.overlayControl !== null
                    Button { objectName: "toggleOverlay"; text: overlay.visible ? "Hide overlay" : "Show overlay"; onClicked: { if (overlay.visible) { window.overlayControl.setClickThrough(false); overlay.hide() } else { window.overlayControl.keepOnScreen(); overlay.show() } } }
                    Button { objectName: "toggleClickThrough"; text: window.overlayControl && window.overlayControl.clickThrough ? "Unlock overlay" : "Make click-through"; enabled: overlay.visible; onClicked: window.overlayControl.setClickThrough(!window.overlayControl.clickThrough) }
                    Button { objectName: "recoverOverlay"; text: "Move on screen"; onClicked: window.overlayControl.keepOnScreen() }
                    Label { text: "Opacity" }
                    Slider { objectName: "overlayOpacity"; from: 0.2; to: 1; value: 1; onMoved: window.overlayControl.setOpacity(value); Accessible.name: "Overlay opacity" }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
    Connections {
        target: window.metadataBridge
        function onStatusChanged(text) { window.metadataStatus = text }
    }
    FolderDialog {
        id: folderPicker
        title: "Choose a folder to browse"
        onAccepted: window.browseFolder = selectedFolder
    }
}
