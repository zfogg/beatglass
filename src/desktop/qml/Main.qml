import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    visible: true
    width: 900; height: 620
    minimumWidth: 600; minimumHeight: 440
    title: "Beatglass"
    property var metadataBridge: metadata
    property string selectedName: ""
    property string metadataStatus: "Choose an audio file to inspect its metadata."

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
                anchors.fill: parent; anchors.margins: 24; spacing: 16
                Label { text: "Library"; font.pixelSize: 28; font.bold: true }
                Label { text: "Inspect a local audio file"; font.pixelSize: 18 }
                Label {
                    text: "Library indexing, analysis jobs, playback and saved results are not available yet."
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Button { text: "Choose audio file…"; objectName: "chooseFile"; onClicked: picker.open() }
                Frame {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        Label { text: window.selectedName || "No file selected"; font.bold: true; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        Label { objectName: "metadataStatus"; text: window.metadataStatus; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }
        Item {
            objectName: "djPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16
                Label { text: "DJ"; font.pixelSize: 28; font.bold: true }
                Label { text: "Live output monitoring is not available yet."; font.pixelSize: 18; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Label { text: "Capture, live estimates and the overlay are still in development."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Item { Layout.fillHeight: true }
            }
        }
    }
    Connections {
        target: window.metadataBridge
        function onStatusChanged(text) { window.metadataStatus = text }
    }
    FileDialog {
        id: picker
        title: "Choose an audio file"
        onAccepted: {
            window.selectedName = selectedFile.toString()
            window.metadataBridge.inspect(selectedFile)
        }
    }
}
