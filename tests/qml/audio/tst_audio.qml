import QtQuick
import QtTest
import "../../../src/desktop/qml"

TestCase {
    name: "AudioSettings"
    when: windowShown
    QtObject { id: metadata }
    QtObject { id: live; property string status: "Stopped"; property string tempo: "—"; property string key: "—"; property string chord: "—" }
    Component { id: shell; Main { visible: true; metadataBridge: metadata; liveBridge: live } }

    function test_unavailableOutputIsExplicitlyDescribed() {
        let app = shell.createObject(null)
        let output = findChild(app, "outputDevice")
        let guidance = findChild(app, "permissionGuidance")
        compare(output.enabled, false)
        compare(output.Accessible.name, "Output device")
        compare(output.Accessible.description, "No output device is currently available for monitoring.")
        compare(guidance.Accessible.name, "Audio capture permission guidance")
        verify(guidance.text.length > 20)
        app.destroy()
    }
}
