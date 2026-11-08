import QtQuick
import QtTest
import "../../../src/desktop/qml"

TestCase {
    name: "LibraryCatalog"
    when: windowShown
    QtObject {
        id: metadata
    }
    QtObject {
        id: live
        property string status: "Stopped"
        property string tempo: "—"
        property string key: "—"
        property string chord: "—"
    }
    QtObject {
        id: catalog
        function browse(folder, search) {
            return search === "drum" ? [{name: "drum.wav", path: "/tmp/drum.wav", folder: false, audio: true}] : []
        }
    }
    Component { id: shell; Main { visible: true; metadataBridge: metadata; liveBridge: live; libraryCatalogBridge: catalog; browseFolder: "file:///tmp" } }

    function test_catalogStatusReflectsFilteredRows() {
        let app = shell.createObject(null)
        let search = findChild(app, "search")
        let status = findChild(app, "catalogStatus")
        compare(status.text, "Catalog: 0 matching entries")
        search.text = "drum"
        compare(status.text, "Catalog: 1 matching entries")
        app.destroy()
    }
}
