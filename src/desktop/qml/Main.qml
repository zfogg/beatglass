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
    property var playbackDevice: playbackDevices.defaultAudioOutput
    readonly property string playbackWarning: playbackGuardBridge.diagnostic(playbackDevice)
    property var liveBridge: livePresentation
    property var collectionBridge: typeof experimentalCollection === "undefined" ? null : experimentalCollection
    property var djBridge: typeof windowsDj === "undefined" ? null : windowsDj
    property var overlayBridge: null
    property url browseFolder: ""
    property bool sortDescending: false
    property string selectedName: ""
    property string metadataStatus: "Choose an audio file to inspect its metadata."
    onBrowseFolderChanged: {
        invalidateSelection()
    }
    onSortDescendingChanged: invalidateSelection()

    function invalidateSelection() {
        browser.currentIndex = -1
        selectEntry(-1)
    }

    function selectEntry(index) {
        const isFile = index >= 0 && index < files.count && !files.isFolder(index)
        selectedName = isFile ? files.get(index, "fileName") : ""
        const url = isFile ? files.get(index, "fileUrl") : ""
        if (window.collectionBridge) window.collectionBridge.selectSource(url)
        metadataBridge.inspect(url)
    }

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
            selectEntry(index)
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
    Connections {
        target: files
        ignoreUnknownSignals: true
        function onCountChanged() { window.invalidateSelection() }
        function onRowsInserted() { window.invalidateSelection() }
        function onRowsRemoved() { window.invalidateSelection() }
        function onRowsMoved() { window.invalidateSelection() }
        function onModelReset() { window.invalidateSelection() }
        function onLayoutChanged() { window.invalidateSelection() }
        function onDataChanged() { window.invalidateSelection() }
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
        window.playbackDevice = playbackDevices.defaultAudioOutput
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
                    text: window.collectionBridge && window.collectionBridge.compositeAvailable ? "Browse local folders and run provisional, uncached analysis." : "Browse local folders. Indexing, analysis and saved results are not available yet."
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                RowLayout {
                    visible: !!window.collectionBridge
                    Label { text: "Experimental indexed files" }
                    Button { objectName: "refreshCollection"; text: "Refresh"; onClicked: window.collectionBridge.refresh("") }
                    Label { objectName: "collectionStatus"; text: window.collectionBridge ? window.collectionBridge.status : ""; Layout.fillWidth: true }
                }
                ListView {
                    objectName: "collectionRows"; visible: !!window.collectionBridge
                    Layout.fillWidth: true; Layout.preferredHeight: visible ? 100 : 0; clip: true
                    model: window.collectionBridge ? window.collectionBridge.rows : []
                    delegate: Label { required property var modelData; width: ListView.view.width; text: modelData.path; elide: Text.ElideMiddle }
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
                    TextField { id: search; objectName: "search"; placeholderText: "Filter file names"; Accessible.name: "Filter file names"; Layout.fillWidth: true; onTextChanged: window.invalidateSelection() }
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
                        onCurrentIndexChanged: window.selectEntry(currentIndex)
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
                        Label { text: player.source.toString() === "" ? "No file loaded for playback" : "Playback source: " + player.source.toString(); elide: Text.ElideMiddle; Layout.fillWidth: true }
                        RowLayout {
                            visible: !!window.collectionBridge && window.collectionBridge.compositeAvailable
                            Button { objectName: "analyzeSelected"; text: "Analyze provisionally"; enabled: !!window.collectionBridge && !!window.selectedName && !window.collectionBridge.analysisActive; onClicked: window.collectionBridge.analyzeSelected() }
                            Button { objectName: "cancelAnalysis"; text: "Cancel"; enabled: !!window.collectionBridge && window.collectionBridge.analysisActive; onClicked: window.collectionBridge.cancelSelected() }
                            Label { objectName: "analysisStatus"; text: window.collectionBridge ? window.collectionBridge.analysisStatus : ""; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        }
                        Label { objectName: "provisionalSummary"; visible: !!window.collectionBridge && window.collectionBridge.compositeAvailable; text: window.collectionBridge ? window.collectionBridge.provisionalSummary : ""; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        ListView {
                            objectName: "provisionalWindows"; visible: !!window.collectionBridge && window.collectionBridge.compositeAvailable && count > 0
                            Layout.fillWidth: true; Layout.preferredHeight: visible ? 120 : 0; clip: true
                            model: window.collectionBridge ? window.collectionBridge.provisionalWindows : []
                            ScrollBar.vertical: ScrollBar {}
                            delegate: Label {
                                required property var modelData
                                width: ListView.view.width; wrapMode: Text.WordWrap
                                text: modelData.range + ": rolling/local key " + modelData.key + " (evidence from frame " + modelData.keyFrom + ", raw strength " + modelData.keyStrength + "); local chord " + modelData.chord + " (raw strength " + modelData.chordStrength + ")"
                            }
                        }
                        RowLayout {
                            Label { text: "Playback output" }
                            ComboBox {
                                id: playbackOutputSelector
                                objectName: "playbackOutputSelector"
                                model: playbackDevices.audioOutputs
                                textRole: "description"
                                Layout.fillWidth: true
                                Accessible.name: "Playback output"
                                onActivated: function(index) {
                                    player.stop()
                                    if (index >= 0 && index < playbackDevices.audioOutputs.length)
                                        window.playbackDevice = playbackDevices.audioOutputs[index]
                                }
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
                Label { text: window.djBridge ? "Monitor a selected Windows output. Other sounds on that output are included." : "Live output monitoring is not available on this platform yet."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    ComboBox { id: sourceBox; objectName: "sourceBox"; model: window.djBridge ? window.djBridge.sources : []; textRole: "name"; Layout.fillWidth: true; Accessible.name: "Output to monitor"; enabled: window.djBridge && !window.djBridge.active }
                    Button { text: "Refresh outputs"; enabled: !!window.djBridge; onClicked: window.djBridge.refresh() }
                }
                RowLayout {
                    Button { objectName: "startMonitor"; text: "Start monitoring"; enabled: !!window.djBridge && !window.djBridge.active && sourceBox.currentIndex >= 0; onClicked: window.djBridge.start(sourceBox.model[sourceBox.currentIndex].id) }
                    Button { objectName: "stopMonitor"; text: "Stop monitoring"; enabled: !!window.djBridge && window.djBridge.active; onClicked: window.djBridge.stop() }
                }
                Label { objectName: "djStatus"; text: "Status: " + (window.djBridge ? window.djBridge.status : "Unavailable") }
                Label { objectName: "djTempo"; text: "BPM: " + (window.djBridge ? window.djBridge.tempo : "—"); font.pixelSize: 22 }
                Label { text: "Key, chords and beat phase are not available yet."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    Button { objectName: "showOverlay"; text: djOverlay.visible ? "Hide overlay" : "Show overlay"; enabled: !!window.overlayBridge; onClicked: { if (djOverlay.visible) { window.overlayBridge.endDrag(); window.overlayBridge.setClickThrough(false); djOverlay.hide() } else { window.overlayBridge.keepOnScreen(); djOverlay.show() } } }
                    Button { objectName: "lockOverlay"; text: window.overlayBridge && window.overlayBridge.clickThrough ? "Unlock overlay" : "Enable click-through"; enabled: !!window.overlayBridge && djOverlay.visible; onClicked: window.overlayBridge.setClickThrough(!window.overlayBridge.clickThrough) }
                    Button { objectName: "recoverOverlay"; text: "Move on screen"; enabled: !!window.overlayBridge; onClicked: window.overlayBridge.keepOnScreen() }
                    Label { text: "Drag when unlocked. Unlock: Ctrl+Shift+U while Beatglass is active"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                RowLayout {
                    Label { text: "Overlay opacity" }
                    Slider { objectName: "overlayOpacity"; from: 0.25; to: 1.0; value: window.overlayBridge ? window.overlayBridge.opacity : 1; enabled: !!window.overlayBridge; Accessible.name: "Overlay opacity"; onMoved: window.overlayBridge.setOpacity(value); Layout.fillWidth: true }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
    Window {
        id: djOverlay
        objectName: "djOverlay"
        visible: false
        width: 240; height: 110
        Component.onCompleted: { x = window.x + 100; y = window.y + 100 }
        title: "Beatglass overlay"
        color: "transparent"
        Rectangle {
            anchors.fill: parent; radius: 10; color: "#d9202530"; border.color: "#92a2b8"
            MouseArea { anchors.fill: parent; cursorShape: Qt.SizeAllCursor; onPressed: if (window.overlayBridge) window.overlayBridge.beginDrag(); onPositionChanged: if (pressed && window.overlayBridge) window.overlayBridge.dragToCursor() }
            Column {
                anchors.centerIn: parent; spacing: 5
                Label { text: window.djBridge ? window.djBridge.status : "Unavailable"; color: "white"; font.pixelSize: 16 }
                Label { text: "BPM " + (window.djBridge ? window.djBridge.tempo : "—"); color: "white"; font.pixelSize: 30; font.bold: true }
            }
        }
    }
    Shortcut { sequence: "Ctrl+Shift+U"; context: Qt.ApplicationShortcut; enabled: !!window.overlayBridge; onActivated: window.overlayBridge.setClickThrough(false) }
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
