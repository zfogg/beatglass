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
        let reset = findChild(app, "resetAppearance")
        let motion = findChild(app, "reduceMotion")
        compare(motion.Accessible.name, "Reduce motion")
        compare(motion.Accessible.description, "Reduce animation and transition effects in the desktop presentation.")
        compare(findChild(app, "libraryTab").Accessible.description, "Browse local audio files.")
        compare(findChild(app, "djTab").Accessible.description, "View live DJ controls and overlay settings.")
        compare(findChild(app, "settingsTab").Accessible.description, "Configure appearance, audio and overlay settings.")
        compare(reset.Accessible.name, "Reset appearance settings")
        compare(reset.Accessible.description, "Restore the default Light theme and turn off reduced motion.")
        selector.currentIndex = 5; selector.activated(5)
        compare(app.selectedTheme, "Yellow")
        reset.clicked()
        compare(app.selectedTheme, "Light")
        compare(app.selectedThemeIndex, 0)
        compare(selector.currentIndex, 0)
        app.destroy()
    }
}
