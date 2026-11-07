import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import QtMultimedia

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    visible: true
    width: 1000; height: 700
    minimumWidth: 600; minimumHeight: 440
    title: "Beatglass"
    property var metadataBridge: metadata
    property var playbackGuardBridge: playbackGuard
    readonly property var playbackDevice: playbackOutputSelector.currentIndex >= 0 && playbackOutputSelector.currentIndex < playbackDevices.audioOutputs.length ? playbackDevices.audioOutputs[playbackOutputSelector.currentIndex] : playbackDevices.defaultAudioOutput
    readonly property string playbackWarning: playbackGuardBridge.diagnostic(playbackDevice)
    property var liveBridge: livePresentation
    property url browseFolder: ""
    property bool sortDescending: false
    property string selectedName: ""
    property string metadataStatus: "Choose an audio file to inspect its metadata."

    function formatTime(ms) {
        const tenths = Math.floor(Math.max(0, ms) / 100)
        const minutes = Math.floor(tenths / 600)
        const seconds = Math.floor(tenths / 10) % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds + "." + (tenths % 10)
    }
    function activateEntry(index) {
        if (index < 0 || index >= files.count) return
        const url = files.get(index, "fileUrl")
        if (files.isFolder(index)) {
            browseFolder = url
        } else {
            player.stop()
            player.source = url
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
    MediaDevices {
        id: playbackDevices
        onAudioOutputsChanged: {
            player.stop()
            window.selectDefaultPlaybackOutput()
        }
    }
    function selectDefaultPlaybackOutput() {
        playbackOutputSelector.currentIndex = -1
        for (let i = 0; i < playbackDevices.audioOutputs.length; ++i) {
            if (playbackDevices.audioOutputs[i].isDefault) { playbackOutputSelector.currentIndex = i; break }
        }
    }
    Component.onCompleted: selectDefaultPlaybackOutput()
    MediaPlayer {
        id: player
        objectName: "player"
        audioOutput: AudioOutput { device: window.playbackDevice }
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
                    text: "Browse local folders. Indexing, analysis and saved results are not available yet."
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
                        RowLayout {
                            Label { text: "Playback output" }
                            ComboBox {
                                id: playbackOutputSelector
                                objectName: "playbackOutputSelector"
                                model: playbackDevices.audioOutputs
                                textRole: "description"
                                Layout.fillWidth: true
                                Accessible.name: "Playback output"
                                onActivated: player.stop()
                            }
                        }
                        Label {
                            objectName: "playbackDeviceWarning"
                            text: window.playbackWarning
                            visible: text !== ""; wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        RowLayout {
                            Button {
                                objectName: "playButton"
                                text: player.playbackState === MediaPlayer.PlayingState ? "Pause" : "Play"
                                enabled: player.source.toString() !== "" && player.error === MediaPlayer.NoError && window.playbackWarning === ""
                                onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
                            }
                            Button { text: "Stop"; enabled: player.playbackState !== MediaPlayer.StoppedState; onClicked: player.stop() }
                            Slider {
                                id: seek
                                objectName: "seek"
                                Layout.fillWidth: true
                                from: 0; to: Math.max(1, player.duration)
                                value: player.position
                                enabled: player.seekable && player.duration > 0
                                Accessible.name: "Playback position"
                                onMoved: player.position = value
                            }
                            Label { text: window.formatTime(player.position) + " / " + window.formatTime(player.duration) }
                        }
                        Label {
                            objectName: "playbackStatus"
                            text: player.error === MediaPlayer.NoError ? "" : "Playback error: " + player.errorString
                            visible: text !== ""; wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label { text: "Chord timeline and analysis details are not available yet."; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    }
                }
            }
        }
        Item {
            objectName: "djPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16
                Label { text: "DJ"; font.pixelSize: 28; font.bold: true }
                Label { text: "Live output monitoring is not available yet."; font.pixelSize: 18; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Label { text: "Capture, live estimates and the overlay are still in development."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Label { text: "Status: " + window.liveBridge.status }
                Label { text: "BPM: " + window.liveBridge.tempo + "    Key: " + window.liveBridge.key + "    Chord: " + window.liveBridge.chord }
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
