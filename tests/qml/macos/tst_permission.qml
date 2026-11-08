import QtQuick
import QtTest
import "../../../src/desktop/qml"

TestCase {
    name: "MacAudioPermission"
    when: windowShown
    QtObject { id: metadata }
    QtObject { id: live; property string status: "Stopped"; property string tempo: "—"; property string key: "—"; property string chord: "—" }
    Component { id: shell; Main { visible: true; metadataBridge: metadata; liveBridge: live } }

    function test_permissionGuidanceIsAccessibleAndActionable() {
        let app = shell.createObject(null)
        let guidance = findChild(app, "permissionGuidance")
        compare(guidance.Accessible.name, "Audio capture permission guidance")
        verify(guidance.text.indexOf("System Settings") >= 0 || guidance.text.indexOf("permission") >= 0)
        app.destroy()
    }
}
