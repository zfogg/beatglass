import QtQuick
import QtTest
import "../../../src/desktop/qml"

TestCase {
    name: "ResetAppearance"
    when: windowShown
    QtObject { id: metadata }
    QtObject { id: live; property string status: "Stopped"; property string tempo: "—"; property string key: "—"; property string chord: "—" }
    Component { id: shell; Main { visible: true; metadataBridge: metadata; liveBridge: live } }

    function test_resetRestoresLightTheme() {
        let app = shell.createObject(null)
        let selector = findChild(app, "themeSelector")
        selector.currentIndex = 5; selector.activated(5)
        compare(app.selectedTheme, "Yellow")
        findChild(app, "resetAppearance").clicked()
        compare(app.selectedTheme, "Light")
        compare(app.selectedThemeIndex, 0)
        compare(selector.currentIndex, 0)
        app.destroy()
    }
}
