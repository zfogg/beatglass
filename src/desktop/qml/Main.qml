import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
ApplicationWindow {
  visible: true; width: 620; height: 280; title: "Beatglass groundwork"
  Column { anchors.centerIn: parent; spacing: 16
    Label { text: "File metadata"; font.pixelSize: 24 }
    Button { text: "Choose audio file"; onClicked: picker.open() }
    Label { id: status; text: "Choose a file to inspect its metadata." }
    Label { text: "Analysis and DJ mode are not yet available in this shell." }
  }
  FileDialog { id: picker; onAccepted: status.text = metadata.inspect(selectedFile) }
}
