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
    property var overlayBridge: typeof macOverlayController !== "undefined" ? macOverlayController : null
    property url browseFolder: ""
    property bool sortDescending: false
    property string selectedName: ""
    property string metadataStatus: "Choose an audio file to inspect its metadata."
    property string selectedTheme: "Light"
    readonly property var themeNames: ["Light", "Dark", "High contrast", "Purple", "Blue", "Yellow"]
    readonly property var themeBackgrounds: ["#f7f8fb", "#171a21", "#000000", "#21152f", "#10243d", "#342b08"]
    readonly property var themeForegrounds: ["#20242b", "#f4f6fb", "#ffffff", "#f4eaff", "#eaf4ff", "#fffbe6"]
    readonly property var themeAccentColors: ["#3f51b5", "#90caf9", "#ffff00", "#c084fc", "#60a5fa", "#facc15"]
    property int selectedThemeIndex: 0
    Component.onCompleted: {
        if (overlayBridge) overlayBridge.attach(djOverlay, window)
    }

    Window {
        id: djOverlay
        objectName: "djOverlay"
        title: "Beatglass overlay"
        visible: false
        width: 310; height: 130
        minimumWidth: 230; minimumHeight: 100
        color: "transparent"
        Rectangle {
            anchors.fill: parent; radius: 12; color: "#e51b2028"; border.color: "#667788"
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
                anchors.fill: parent; anchors.margins: 14; spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Beatglass · " + window.liveBridge.status; color: "white"; Layout.fillWidth: true; elide: Text.ElideRight }
                    Button {
                        text: "×"; Accessible.name: "Hide overlay"
                        visible: window.overlayBridge && !window.overlayBridge.clickThrough
                        onClicked: window.overlayBridge.hideOverlay()
                    }
                }
                Label { text: "BPM " + window.liveBridge.tempo; color: "white"; font.pixelSize: 24; font.bold: true }
                Label { text: "Key " + window.liveBridge.key + "    Chord " + window.liveBridge.chord; color: "white" }
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
            ComboBox {
                id: themeSelector
                objectName: "themeSelector"
                model: window.themeNames
                currentIndex: window.selectedThemeIndex
                Accessible.name: "Color theme"
                ToolTip.text: "Choose a color theme"
                onActivated: {
                    window.selectedThemeIndex = currentIndex
                    window.selectedTheme = currentText
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
                Label { text: "Library"; color: window.themeForegrounds[window.selectedThemeIndex]; font.pixelSize: 28; font.bold: true }
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
                Label { text: "Live output monitoring is not available yet."; font.pixelSize: 18; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Label { text: "Capture, live estimates and the overlay are still in development."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Label { text: "Status: " + window.liveBridge.status }
                Label { text: "BPM: " + window.liveBridge.tempo + "    Key: " + window.liveBridge.key + "    Chord: " + window.liveBridge.chord }
                RowLayout {
                    visible: window.overlayBridge !== null
                    Button { text: "Show overlay"; objectName: "showOverlay"; enabled: window.overlayBridge && window.overlayBridge.available; onClicked: window.overlayBridge.showOverlay() }
                    Button { text: "Unlock overlay"; objectName: "unlockOverlay"; enabled: window.overlayBridge && window.overlayBridge.clickThrough; onClicked: window.overlayBridge.setClickThrough(false) }
                    Button { text: "Hide overlay"; objectName: "hideOverlay"; enabled: window.overlayBridge && window.overlayBridge.available; onClicked: window.overlayBridge.hideOverlay() }
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    CheckBox {
                        id: clickThroughToggle
                        text: "Click through"; objectName: "clickThrough"
                        enabled: window.overlayBridge && window.overlayBridge.available
                        onClicked: if (window.overlayBridge) {
                            window.overlayBridge.setClickThrough(checked)
                            checked = window.overlayBridge.clickThrough
                        }
                        Connections {
                            target: window.overlayBridge
                            function onClickThroughChanged() { clickThroughToggle.checked = window.overlayBridge.clickThrough }
                        }
                    }
                    Label { text: "Opacity" }
                    Slider { objectName: "overlayOpacity"; from: 0.2; to: 1; value: window.overlayBridge ? window.overlayBridge.opacity : 0.9; onMoved: if (window.overlayBridge) window.overlayBridge.setOpacity(value) }
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    Label { text: "Display" }
                    ComboBox {
                        objectName: "overlayDisplay"
                        enabled: window.overlayBridge && window.overlayBridge.available && count > 0
                        model: window.overlayBridge ? window.overlayBridge.screenNames : []
                        onActivated: if (window.overlayBridge) window.overlayBridge.moveToScreen(currentIndex)
                        Layout.fillWidth: true
                    }
                    Label { text: "Size" }
                    ComboBox {
                        objectName: "overlaySize"
                        model: ["Compact", "Medium", "Large"]; currentIndex: 1
                        enabled: window.overlayBridge && window.overlayBridge.available
                        onActivated: if (window.overlayBridge) {
                            const sizes = [[250, 110], [310, 130], [430, 180]]
                            window.overlayBridge.resizeOverlay(sizes[currentIndex][0], sizes[currentIndex][1])
                        }
                    }
                }
                Label { visible: window.overlayBridge !== null && !window.overlayBridge.available; text: "Overlay unavailable on this display."; wrapMode: Text.WordWrap }
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
