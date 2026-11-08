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
    onClosing: if (overlayBridge) overlayBridge.hideOverlay()
    property var metadataBridge: metadata
    property var playbackBridge: typeof playbackSession !== "undefined" ? playbackSession : null
    property var liveBridge: livePresentation
    property var analysisJobBridge: typeof analysisJob !== "undefined" ? analysisJob : null
    property var captureMonitorBridge: typeof captureMonitor !== "undefined" ? captureMonitor : null
    property var endpointSelectionBridge: typeof endpointSelection !== "undefined" ? endpointSelection : null
    property var themePreferencesBridge: typeof themePreferences !== "undefined" ? themePreferences : null
    property var overlayBridge: typeof macOverlayController !== "undefined" ? macOverlayController : null
    property url browseFolder: ""
    property var catalogRows: []
    property bool sortDescending: false
    property string selectedName: ""
    property url selectedAudioUrl: ""
    property string metadataStatus: "Choose an audio file to inspect its metadata."
    property string catalogDiagnostic: ""
    readonly property bool playbackMatchesSelection: playbackBridge && selectedAudioUrl.toString() !== ""
        && playbackBridge.source.toString() === selectedAudioUrl.toString()
    Shortcut {
        objectName: "overlayEscapeShortcut"
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: if (window.overlayBridge && window.overlayBridge.clickThrough)
                         window.overlayBridge.setClickThrough(false)
    }
    property string selectedTheme: "Light"
    readonly property var themeNames: ["Light", "Dark", "High contrast", "Purple", "Blue", "Yellow"]
    readonly property var themeBackgrounds: ["#f7f8fb", "#171a21", "#000000", "#21152f", "#10243d", "#342b08"]
    readonly property var themeForegrounds: ["#20242b", "#f4f6fb", "#ffffff", "#f4eaff", "#eaf4ff", "#fffbe6"]
    readonly property var themeAccentColors: ["#3f51b5", "#90caf9", "#ffff00", "#c084fc", "#60a5fa", "#facc15"]
    property int selectedThemeIndex: themePreferencesBridge ? themePreferencesBridge.themeIndex : 0
    // Keep the visual selection synchronized when preferences are restored or
    // changed by another settings surface. Assignments from the controls below
    // intentionally remain immediate; this connection handles external changes.
    Connections {
        target: window.themePreferencesBridge
        function onThemeIndexChanged() {
            const index = Math.max(0, Math.min(window.themeNames.length - 1,
                                               window.themePreferencesBridge.themeIndex))
            window.selectedThemeIndex = index
            window.selectedTheme = window.themeNames[index]
        }
    }
    Connections {
        target: window.overlayBridge
        function onAvailableChanged() {
            if (window.overlayBridge && !window.overlayBridge.available && djOverlay.visible)
                window.hideOverlayAndRestoreControls()
        }
    }
    Component.onCompleted: {
        if (overlayBridge) overlayBridge.attach(djOverlay, window)
        refreshCatalog()
    }
    function refreshCatalog() {
        window.catalogRows = window.libraryCatalogBridge && window.browseFolder.toString() !== ""
            ? window.libraryCatalogBridge.browse(window.browseFolder, search.text) : []
        window.catalogDiagnostic = window.libraryCatalogBridge && window.libraryCatalogBridge.lastDiagnostic !== undefined
                ? window.libraryCatalogBridge.lastDiagnostic : ""
    }
    function browserEmptyMessage() {
        if (!window.browseFolder.toString()) return "Choose a folder to browse."
        if (files.status === FolderListModel.Error)
            return "This folder could not be read. Choose another folder or check its permissions."
        if (files.status === FolderListModel.Loading)
            return "Loading folder contents…"
        if (window.catalogDiagnostic !== "") return "This folder could not be read: " + window.catalogDiagnostic
        return "No matching audio files or folders. Try a different search or choose another folder."
    }
    function showOverlayAndHideControls() {
        if (!window.overlayBridge || !window.overlayBridge.available) return false
        if (!window.overlayBridge.showOverlay()) return false
        window.hide()
        return true
    }
    function hideOverlayAndRestoreControls() {
        if (!window.overlayBridge) return
        window.overlayBridge.hideOverlay()
        window.show()
        window.raise()
        window.requestActivate()
    }
    property var libraryCatalogBridge: typeof libraryCatalog !== "undefined" ? libraryCatalog : null
    onBrowseFolderChanged: refreshCatalog()

    Window {
        id: djOverlay
        objectName: "djOverlay"
        title: "Beatglass overlay"
        visible: false
        width: 310; height: 130
        minimumWidth: 230; minimumHeight: 100
        color: "transparent"
        Rectangle {
            id: overlaySurface
            anchors.fill: parent; radius: 12; color: "#e51b2028"; border.color: "#667788"
            property real pulseOpacity: 0.0
            border.width: 1 + pulseOpacity * 3
            Behavior on border.width { enabled: !window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion; NumberAnimation { duration: 100 } }
            Timer {
                id: beatPulse
                objectName: "beatPulse"
                repeat: true
                interval: { var bpm = Number(window.liveBridge.tempo); return isFinite(bpm) && bpm > 0 ? Math.max(120, 60000 / bpm) : 1000 }
                running: djOverlay.visible && (!window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion) && window.liveBridge.status === "Tracking"
                onTriggered: { overlaySurface.pulseOpacity = 1.0; pulseFade.restart() }
            }
            Timer { id: pulseFade; interval: 120; onTriggered: overlaySurface.pulseOpacity = 0.0 }
            Connections { target: window.liveBridge; function onChanged() { beatPulse.restart() } }
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
                Label {
                    objectName: "overlayEscapeHint"
                    visible: window.overlayBridge && window.overlayBridge.clickThrough
                    text: "Press Escape to unlock overlay controls"
                    color: "#dbeafe"
                    Accessible.name: "Overlay recovery hint"
                    Accessible.description: text
                }
                Label { text: "BPM " + window.liveBridge.tempo; color: "white"; font.pixelSize: 24; font.bold: true }
                Label { text: "Key " + window.liveBridge.key + "    Chord " + window.liveBridge.chord; color: "white" }
                Label { text: window.captureMonitorBridge ? "Capture " + window.captureMonitorBridge.status : "Capture unavailable"; color: "#cbd5e1"; objectName: "overlayCaptureStatus"; Accessible.name: "Overlay capture status"; Accessible.description: text }
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
            selectedAudioUrl = url
            metadataBridge.inspect(url)
        }
    }
    function clearSelectedAudio() {
        window.selectedName = ""
        window.selectedAudioUrl = ""
        window.metadataStatus = "Choose an audio file to inspect its metadata."
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
                TabButton { text: "Library"; objectName: "libraryTab"; Accessible.name: "Library"; Accessible.description: "Browse local audio files." }
                TabButton { text: "DJ"; objectName: "djTab"; Accessible.name: "DJ"; Accessible.description: "View live DJ controls and overlay settings." }
                TabButton { text: "Settings"; objectName: "settingsTab"; Accessible.name: "Settings"; Accessible.description: "Configure appearance, audio and overlay settings." }
            }
            ComboBox {
                id: themeSelector
                objectName: "themeSelector"
                model: window.themeNames
                currentIndex: window.selectedThemeIndex
                Accessible.name: "Color theme"
                Accessible.description: "Choose one of six color themes for the Beatglass desktop."
                ToolTip.text: "Choose a color theme"
                onActivated: {
                    window.selectedThemeIndex = currentIndex
                    window.selectedTheme = currentText
                    if (window.themePreferencesBridge) window.themePreferencesBridge.themeIndex = currentIndex
                }
            }
            Switch {
                id: reduceMotion
                objectName: "reduceMotion"
                text: "Reduce motion"
                checked: window.themePreferencesBridge ? window.themePreferencesBridge.reduceMotion : false
                Accessible.name: "Reduce motion"
                Accessible.role: Accessible.CheckBox
                Accessible.description: "Reduce animation and transition effects in the desktop presentation."
                onClicked: if (window.themePreferencesBridge) window.themePreferencesBridge.reduceMotion = checked
            }
            Button {
                objectName: "resetAppearance"
                text: "Reset appearance"
                Accessible.name: "Reset appearance settings"
                Accessible.description: "Restore the default Light theme and turn off reduced motion."
                ToolTip.text: "Restore the default theme and motion setting"
                onClicked: {
                    if (window.themePreferencesBridge) window.themePreferencesBridge.reset()
                    window.selectedThemeIndex = 0
                    window.selectedTheme = window.themeNames[0]
                    themeSelector.currentIndex = 0
                }
            }
            Row {
                id: themePreviews
                objectName: "themePreviews"
                spacing: 3
                Repeater {
                    model: window.themeNames.length
                    delegate: Button {
                        required property int index
                        objectName: "themePreview" + index
                        width: 18; height: 18
                        padding: 0
                        checkable: true
                        checked: window.selectedThemeIndex === index
                        Accessible.name: window.themeNames[index] + " theme preview"
                        Accessible.description: "Select " + window.themeNames[index] + " theme; "
                                              + (checked ? "selected" : "not selected")
                        Accessible.role: Accessible.RadioButton
                        ToolTip.text: "Select " + window.themeNames[index] + " theme"
                        background: Rectangle {
                            radius: 3
                            color: window.themeBackgrounds[index]
                            border.width: window.selectedThemeIndex === index ? 2 : 1
                            border.color: window.themeAccentColors[index]
                        }
                        onClicked: {
                            window.selectedThemeIndex = index
                            window.selectedTheme = window.themeNames[index]
                            themeSelector.currentIndex = index
                                if (window.themePreferencesBridge) window.themePreferencesBridge.themeIndex = index
                        }
                    }
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
                    objectName: "catalogStatus"
                    text: window.browseFolder ? (files.count + " catalog entries shown. Use the search field or open a folder to browse.") : "Choose a folder to browse local audio files."
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                    Accessible.name: "Library catalog status"
                    Accessible.description: text
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "Choose folder…"; objectName: "chooseFolder"; Accessible.description: "Choose a local folder to browse"; onClicked: folderPicker.open() }
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
                    TextField { id: search; objectName: "search"; placeholderText: "Filter file names"; Accessible.name: "Filter file names"; Accessible.description: "Filter the current folder by audio or folder name"; Layout.fillWidth: true; onTextChanged: window.refreshCatalog() }
                    Button { objectName: "sortOrder"; text: window.sortDescending ? "Name ↓" : "Name ↑"; Accessible.name: "Reverse name sort order"; Accessible.description: "Toggle ascending or descending file-name order"; onClicked: window.sortDescending = !window.sortDescending }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Name"; font.bold: true; Layout.fillWidth: true }
                    Label { text: "Type"; font.bold: true; Layout.preferredWidth: 100 }
                }
                Label {
                    objectName: "catalogStatus"
                    text: window.browseFolder.toString() === "" ? "Catalog: choose a folder" : "Catalog: " + window.catalogRows.length + " matching entries"
                    Accessible.name: text
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
                        Keys.onSpacePressed: window.activateEntry(currentIndex)
                        ScrollBar.vertical: ScrollBar {}
                        delegate: ItemDelegate {
                            required property int index
                            required property string fileName
                            required property bool fileIsDir
                            width: ListView.view.width
                            highlighted: ListView.isCurrentItem
                            Accessible.name: fileName + (fileIsDir ? ", folder" : ", audio file")
                            Accessible.description: fileIsDir ? "Open this folder" : "Select this audio file for metadata, analysis and playback"
                            onClicked: { browser.currentIndex = index; window.activateEntry(index) }
                            contentItem: RowLayout {
                                Label { text: fileName; elide: Text.ElideMiddle; Layout.fillWidth: true }
                                Label { text: fileIsDir ? "Folder" : "Audio file"; Layout.preferredWidth: 100 }
                            }
                        }
                        Column {
                            anchors.centerIn: parent
                            objectName: "browserEmptyState"
                            visible: !window.browseFolder.toString() || (files.status === FolderListModel.Ready && files.count === 0)
                            spacing: 8
                            Label {
                                objectName: "browserEmptyMessage"
                                text: window.browserEmptyMessage()
                                Accessible.name: "Library browser empty state"
                                Accessible.description: text
                            }
                            Button {
                                objectName: "chooseFolderEmpty"
                                text: "Choose folder…"
                                Accessible.name: "Choose a folder"
                                Accessible.description: "Choose a local folder to populate the Library."
                                onClicked: folderPicker.open()
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Button { text: "Open"; objectName: "openPlayback"; Accessible.description: "Open the selected audio source for playback"; enabled: window.selectedAudioUrl.toString() !== "" && window.playbackBridge; onClicked: window.playbackBridge.openSource(window.selectedAudioUrl, 1) }
                            Button { text: "Reopen selected"; objectName: "reopenSelectedPlayback"; visible: window.selectedAudioUrl.toString() !== "" && window.playbackBridge && !window.playbackMatchesSelection; enabled: visible; Accessible.description: "Replace the current playback source with the selected audio source"; onClicked: window.playbackBridge.openSource(window.selectedAudioUrl, window.playbackBridge.epoch + 1) }
                            Button { text: window.playbackBridge && window.playbackBridge.state === "Paused" ? "Resume" : "Pause"; objectName: "pausePlayback"; Accessible.description: "Pause or resume the selected audio source"; enabled: window.playbackBridge && window.playbackBridge.state === "Playing"; onClicked: window.playbackBridge.pause() }
                            Button { text: "Stop"; objectName: "stopPlayback"; Accessible.description: "Stop playback and clear the current source"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: window.playbackBridge.stop() }
                            Button { text: "−5 s"; objectName: "seekBackPlayback"; Accessible.description: "Seek playback backward five seconds"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: window.playbackBridge.seek(Math.max(0, window.playbackBridge.currentFrame - 5 * 44100), window.playbackBridge.epoch + 1) }
                            Button { text: "+5 s"; objectName: "seekForwardPlayback"; Accessible.description: "Seek playback forward five seconds"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: window.playbackBridge.seek(window.playbackBridge.currentFrame + 5 * 44100, window.playbackBridge.epoch + 1) }
                            SpinBox { id: seekFrame; objectName: "seekFrame"; from: 0; to: 2147483647; value: window.playbackBridge ? Math.min(seekFrame.to, window.playbackBridge.currentFrame) : 0; editable: true; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; Accessible.name: "Seek target frame"; Accessible.description: "Enter an audio frame and apply it as the playback position" }
                            Button { text: "Seek"; objectName: "seekToFrame"; enabled: seekFrame.enabled; Accessible.description: "Seek playback to the entered audio frame"; onClicked: window.playbackBridge.seek(seekFrame.value, window.playbackBridge.epoch + 1) }
                            Label { text: window.playbackBridge ? "Playback: " + window.playbackBridge.state : "Playback unavailable"; objectName: "playbackState"; Accessible.name: "Playback state"; Accessible.description: text }
                            Label { text: window.playbackBridge && window.playbackBridge.source.toString() !== "" ? "Source: " + window.playbackBridge.source.fileName : "No source opened"; objectName: "playbackSource"; Accessible.name: "Playback source"; Accessible.description: text; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Label { text: window.playbackBridge ? "Frame " + window.playbackBridge.currentFrame + " · Epoch " + window.playbackBridge.epoch : ""; objectName: "playbackPosition"; Accessible.name: "Playback position"; Accessible.description: text }
                            Label { text: window.playbackBridge && window.playbackBridge.state !== "Stopped" ? "Seek uses audio frames; track duration is not available yet." : ""; objectName: "playbackSeekLimitation"; Accessible.name: "Playback seek limitation"; Accessible.description: text; visible: text !== ""; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Label { text: window.playbackMatchesSelection ? "Playing selected source" : (window.selectedAudioUrl.toString() !== "" && window.playbackBridge && window.playbackBridge.source.toString() !== "" ? "Playback is on another source" : ""); objectName: "playbackSelectionStatus"; Accessible.name: "Playback selection status"; Accessible.description: text; visible: text !== "" }
                        }
                        Label {
                            objectName: "playbackUnavailable"
                            text: "Playback controls are unavailable in this build; metadata and analysis details remain read-only."
                            visible: !window.playbackBridge
                            Accessible.name: "Playback availability"
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                    }
                }
                Frame {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        Label {
                            objectName: "selectedAudioName"
                            text: window.selectedName || "No file selected"
                            font.bold: true; elide: Text.ElideMiddle; Layout.fillWidth: true
                            Accessible.name: "Selected audio file"
                            Accessible.description: text
                        }
                        Label {
                            objectName: "selectedAudioSource"
                            text: window.selectedAudioUrl.toString() !== "" ? "Source: " + window.selectedAudioUrl : "Source unavailable"
                            elide: Text.ElideMiddle; Layout.fillWidth: true
                            Accessible.name: "Selected audio source"
                            Accessible.description: text
                        }
                        Label {
                            objectName: "selectedAudioDuration"
                            text: window.selectedAudioUrl.toString() !== "" ? "Duration: unknown until decoded metadata is available" : "Duration unavailable"
                            Accessible.name: "Selected audio duration"
                            Accessible.description: text
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Button {
                            objectName: "clearSelectedAudio"
                            text: "Clear selection"
                            enabled: window.selectedName !== ""
                            Accessible.name: "Clear selected audio"
                            Accessible.description: "Clear the selected audio source and return analysis details to the empty state."
                            onClicked: window.clearSelectedAudio()
                        }
                        Label {
                            objectName: "metadataStatus"; text: window.metadataStatus
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                            Accessible.name: "Audio metadata status"
                            Accessible.description: text
                        }
                        Button {
                            objectName: "retryMetadata"
                            text: "Retry metadata"
                            visible: window.selectedAudioUrl.toString() !== "" && window.metadataStatus.indexOf("Error:") === 0
                            enabled: visible
                            Accessible.name: "Retry metadata"
                            Accessible.description: "Retry reading metadata for the selected audio source after an error."
                            onClicked: window.metadataBridge.inspect(window.selectedAudioUrl)
                        }
                        Label {
                            objectName: "metadataErrorGuidance"
                            text: "Metadata could not be read. Retry this file or clear the selection and choose another source."
                            visible: window.metadataStatus.indexOf("Error:") === 0
                            Accessible.name: "Metadata error recovery guidance"
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            objectName: "detailsAvailability"
                            text: window.selectedName ? ("BPM " + analysisPresentation.bpm + " · Key " + analysisPresentation.key + " · Beats " + analysisPresentation.beatCount + " · Observed frames " + analysisPresentation.observedFrames) : "Select an audio file to inspect its read-only details."
                            Layout.fillWidth: true; wrapMode: Text.WordWrap
                            Accessible.name: "Analysis and playback status"
                            Accessible.description: text
                        }
                        Label {
                            objectName: "analysisUncertainty"
                            text: window.selectedName
                                  ? "Track key: Unknown · No validated track-level key. Window evidence is provisional and source-attributed."
                                  : "Track key: Unknown until a source is selected."
                            Accessible.name: "Analysis uncertainty"
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            objectName: "analysisFreshness"
                            text: window.selectedName && window.analysisJobBridge && window.analysisJobBridge.status === "Complete" ? "Analysis freshness: source-linked analysis complete." : "Analysis freshness: waiting for source-linked analysis evidence."
                            Accessible.name: "Analysis freshness"
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            objectName: "sourceWorkflowStatus"
                            text: window.selectedAudioUrl.toString() === ""
                                  ? "Select an audio source to inspect and play it."
                                  : "Source workflow · playback: " + (window.playbackBridge ? window.playbackBridge.state : "unavailable") + " · " + window.metadataStatus
                            Accessible.name: "Selected source workflow status"
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        RowLayout { Layout.fillWidth: true
                            Label {
                                text: !window.selectedName ? "Queued · select an audio source to begin analysis." : (window.analysisJobBridge ? (window.analysisJobBridge.status + " · observed " + window.analysisJobBridge.frames + " frames.") : "Analysis progress unavailable")
                                objectName: "analysisProgress"
                                Accessible.name: "Analysis progress"
                                Accessible.description: text
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisBatchStatus"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchAcceptedCount !== undefined
                                text: visible ? ("Batch queue · accepted " + window.analysisJobBridge.batchAcceptedCount
                                      + " · rejected " + window.analysisJobBridge.batchRejectedCount
                                      + (window.analysisJobBridge.batchCapacity !== undefined ? (" · capacity " + window.analysisJobBridge.batchCapacity) : "")) : ""
                                Accessible.name: "Batch analysis queue status"
                                Accessible.description: visible ? text : "Batch queue status is unavailable until the analysis bridge reports it."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Button {
                                text: "Retry analysis"
                                objectName: "retryAnalysis"
                                visible: window.analysisJobBridge !== null
                                enabled: visible && window.analysisJobBridge.canRetry
                                Accessible.name: "Retry analysis"
                                Accessible.description: "Retry the last source-attributed analysis operation after cancellation or failure."
                                onClicked: window.analysisJobBridge.retry()
                                ToolTip.text: Accessible.description
                            }
                            Button {
                                objectName: "cancelAnalysis"
                                text: "Cancel analysis"
                                visible: window.analysisJobBridge !== null
                                enabled: visible && window.analysisJobBridge.canCancel
                                Accessible.name: "Cancel analysis"
                                Accessible.description: "Cancel the active source-attributed analysis operation."
                                onClicked: window.analysisJobBridge.cancel()
                            }
                            BusyIndicator {
                                objectName: "analysisWaitingIndicator"
                                running: window.selectedName !== "" && window.analysisJobBridge && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running")
                                visible: running
                                Accessible.name: "Analysis waiting indicator"
                                Accessible.description: "Analysis is waiting for the first observed frame; progress percentage is not available yet."
                            }
                        }
                    }
                }
            }
        }
        Item {
            objectName: "djPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 16
                Label { text: "DJ"; font.pixelSize: 28; font.bold: true }
                RowLayout {
                    Button { text: window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" ? "Stop monitoring" : (window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "Retry monitoring" : "Start monitoring"); objectName: "toggleMonitoring"; enabled: window.captureMonitorBridge !== null; Accessible.description: "Start monitoring, or retry after a capture error."; onClicked: window.captureMonitorBridge.status === "Monitoring" ? window.captureMonitorBridge.stop() : window.captureMonitorBridge.start() }
                    Label { text: window.captureMonitorBridge ? "Capture: " + window.captureMonitorBridge.status : "Capture unavailable"; objectName: "captureStatus"; Accessible.name: "Capture monitor status"; Accessible.description: text }
                }
                Label { text: "Live output monitoring: " + window.liveBridge.status; font.pixelSize: 18; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Live output monitoring"; Accessible.description: text }
                Label { text: window.liveBridge.status === "Stopped" ? "Start monitoring from the DJ controls when an output source is available." : "Live estimates update below as output audio is captured."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    objectName: "visualizationModeControls"
                    Button { text: "Waveform"; objectName: "waveformView"; checkable: true; checked: visualizationCanvas.viewMode === "waveform"; Accessible.name: "Waveform view"; Accessible.description: "Show the selectable time-domain waveform."; onClicked: visualizationCanvas.viewMode = "waveform" }
                    Button { text: "FFT"; objectName: "spectrumView"; checkable: true; checked: visualizationCanvas.viewMode === "spectrum"; Accessible.name: "FFT view"; Accessible.description: "Show the selectable frequency-domain FFT."; onClicked: visualizationCanvas.viewMode = "spectrum" }
                    Label { text: visualizationCanvas.viewMode === "waveform" ? "Time domain" : "Frequency domain"; objectName: "visualizationModeStatus"; Accessible.name: "Visualization mode" }
                }
                Canvas {
                    id: visualizationCanvas; objectName: "visualizationCanvas"
                    property string viewMode: "waveform"
                    Layout.fillWidth: true; Layout.preferredHeight: 120
                    Accessible.name: "Live waveform and spectrum"
                    Accessible.description: "Bounded waveform and FFT magnitudes from the current output capture."
                    onPaint: {
                        var ctx = getContext("2d"); ctx.reset();
                        ctx.fillStyle = "#10151f"; ctx.fillRect(0, 0, width, height)
                        var wave = typeof visualizationPresentation !== "undefined" ? visualizationPresentation.waveform : []
                        var spec = typeof visualizationPresentation !== "undefined" ? visualizationPresentation.spectrum : []
                        if (viewMode === "waveform") {
                            ctx.strokeStyle = "#60a5fa"; ctx.beginPath()
                            for (var i = 0; i < wave.length; ++i) {
                                var x = i * width / Math.max(1, wave.length - 1)
                                var lo = wave[i][0], hi = wave[i][1]
                                ctx.moveTo(x, height * (0.5 - hi * 0.4)); ctx.lineTo(x, height * (0.5 - lo * 0.4))
                            }
                            ctx.stroke()
                        } else {
                            ctx.fillStyle = "#c084fc"; var bar = width / Math.max(1, spec.length)
                            for (var k = 0; k < spec.length; ++k) ctx.fillRect(k * bar, height, Math.max(1, bar - 1), -Math.min(height * 0.45, spec[k] * height * 2))
                        }
                    }
                    Connections { target: typeof visualizationPresentation !== "undefined" ? visualizationPresentation : null; function onChanged() { visualizationCanvas.requestPaint() } }
                }
                Label {
                    objectName: "visualizationStatus"
                    text: typeof visualizationPresentation === "undefined"
                          ? "Visualization unavailable"
                          : (visualizationPresentation.waveform.length === 0 && visualizationPresentation.spectrum.length === 0
                             ? "Waiting for captured audio data"
                             : "Waveform samples: " + visualizationPresentation.waveform.length + " · Spectrum bins: " + visualizationPresentation.spectrum.length)
                    Accessible.name: "Visualization data status"
                    Accessible.description: text
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "visualizationDataStatus"
                    text: (typeof visualizationPresentation !== "undefined" ? visualizationPresentation.waveform.length : 0) + " waveform samples · " + (typeof visualizationPresentation !== "undefined" ? visualizationPresentation.spectrum.length : 0) + " spectrum bins"
                    Accessible.name: "Visualization data status"; Accessible.description: text
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "visualizationCaptureStatus"
                    text: window.liveBridge.status === "Stopped" ? "Capture is stopped; start monitoring to populate the visualization." : "Capture is active; visualization updates from the current output stream."
                    Accessible.name: "Visualization capture status"
                    Accessible.description: text
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "djStatus"; text: "Status: " + window.liveBridge.status
                    Accessible.name: "DJ monitoring status"; Accessible.description: text
                }
                Label {
                    objectName: "djEstimates"
                    text: "BPM: " + window.liveBridge.tempo + "    Key: " + window.liveBridge.key + "    Chord: " + window.liveBridge.chord
                    Accessible.name: "DJ estimates"; Accessible.description: text
                }
                 Label {
                     objectName: "djEvidenceStatus"
                     text: {
                         const state = window.liveBridge.status
                         if (state === "Tracking") return "Live evidence is provisional and source-attributed to the selected output mix. Overlapping decks or notifications may make key/chord labels ambiguous."
                         if (state === "Reacquiring") return "Reacquiring the selected output source; previous BPM, key, and chord evidence has been cleared."
                         if (state === "Silent") return "No active output evidence; BPM, key, and chord labels are unavailable until audio resumes."
                         if (state === "Error") return "Output capture error; live evidence is unavailable. Check the selected endpoint and retry monitoring."
                         return "Live evidence is unavailable until output monitoring starts."
                     }
                     Accessible.name: "DJ evidence status"
                     Accessible.description: text
                     wrapMode: Text.WordWrap; Layout.fillWidth: true
                 }
                RowLayout {
                    visible: window.overlayBridge !== null
                    Button { text: "Show overlay"; objectName: "showOverlay"; Accessible.description: "Show the DJ overlay window"; enabled: window.overlayBridge && window.overlayBridge.available; onClicked: window.showOverlayAndHideControls() }
                    Button { text: "Unlock overlay"; objectName: "unlockOverlay"; Accessible.description: "Restore keyboard and pointer access to the overlay"; enabled: window.overlayBridge && window.overlayBridge.clickThrough; onClicked: window.overlayBridge.setClickThrough(false) }
                    Button { text: "Hide overlay"; objectName: "hideOverlay"; Accessible.description: "Hide the DJ overlay window"; enabled: window.overlayBridge && window.overlayBridge.available; onClicked: window.hideOverlayAndRestoreControls() }
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    CheckBox {
                        id: clickThroughToggle
                        text: "Click through"; objectName: "clickThrough"
                        Accessible.name: "Click through"
                        Accessible.role: Accessible.CheckBox
                        Accessible.description: "Allow pointer input to pass through the overlay window"
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
                    Label { objectName: "overlayInteractionMode"; text: window.overlayBridge ? (window.overlayBridge.clickThrough ? "Input mode: click-through" : "Input mode: interactive") : "Input mode unavailable"; Accessible.name: "Overlay interaction mode"; Accessible.description: text }
                    Label { text: "Opacity" }
                    Slider { id: overlayOpacityControl; objectName: "overlayOpacity"; Accessible.name: "Overlay opacity"; Accessible.description: "Adjust the overlay window opacity"; from: 0.2; to: 1; value: window.overlayBridge ? window.overlayBridge.opacity : 0.9; onMoved: if (window.overlayBridge) window.overlayBridge.setOpacity(value) }
                    Connections {
                        target: window.overlayBridge
                        function onOpacityChanged() { if (window.overlayBridge) overlayOpacityControl.value = window.overlayBridge.opacity }
                    }
                    Label { objectName: "overlayOpacityValue"; text: window.overlayBridge ? "Current opacity: " + Math.round(window.overlayBridge.opacity * 100) + "%" : "Opacity unavailable"; Accessible.name: "Current overlay opacity"; Accessible.description: text }
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    Label { text: "Display" }
                    ComboBox {
                        objectName: "overlayDisplay"
                    Accessible.name: "Overlay display"; Accessible.description: "Choose the display where the overlay is shown"
                        enabled: window.overlayBridge && window.overlayBridge.available && count > 0
                        model: window.overlayBridge ? window.overlayBridge.screenNames : []
                        currentIndex: window.overlayBridge && window.overlayBridge.screenIndex >= 0 ? window.overlayBridge.screenIndex : 0
                        onActivated: if (window.overlayBridge) window.overlayBridge.moveToScreen(currentIndex)
                        Layout.fillWidth: true
                    }
                    Label { objectName: "overlayDisplayStatus"; text: window.overlayBridge && window.overlayBridge.screenIndex >= 0 && window.overlayBridge.screenIndex < window.overlayBridge.screenNames.length ? "Current display: " + window.overlayBridge.screenNames[window.overlayBridge.screenIndex] : "Current display unavailable"; Accessible.name: "Current overlay display"; Accessible.description: text }
                    Label { text: "Size" }
                    ComboBox {
                        objectName: "overlaySize"
                    Accessible.name: "Overlay size"; Accessible.description: "Choose the overlay window size"
                        model: ["Compact", "Medium", "Large"]; currentIndex: 1
                        enabled: window.overlayBridge && window.overlayBridge.available
                        onActivated: if (window.overlayBridge) {
                            const sizes = [[250, 110], [310, 130], [430, 180]]
                            window.overlayBridge.resizeOverlay(sizes[currentIndex][0], sizes[currentIndex][1])
                        }
                    }
                }
                Label { visible: window.overlayBridge !== null && !window.overlayBridge.available; text: "Overlay unavailable on this display."; objectName: "overlayUnavailable"; Accessible.name: "Overlay availability"; Accessible.description: text; wrapMode: Text.WordWrap }
                Item { Layout.fillHeight: true }
            }
        }
        Item {
            objectName: "settingsPage"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 14
                Label { text: "Settings"; font.pixelSize: 28; font.bold: true; color: window.themeForegrounds[window.selectedThemeIndex] }
                GroupBox {
                    title: "Audio capture"
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent; spacing: 8
                        Label { text: "Output device"; font.bold: true }
                            RowLayout {
                                ComboBox {
                                    objectName: "endpointSelector"
                                    model: window.endpointSelectionBridge ? window.endpointSelectionBridge.endpoints : []
                                    textRole: "name"
                                    enabled: count > 0; Layout.fillWidth: true
                                    Accessible.name: "Output capture endpoint"
                                    Accessible.description: "Choose the output endpoint used for live capture."
                                    onActivated: if (window.endpointSelectionBridge) window.endpointSelectionBridge.select(currentValue.id)
                                }
                                Button { text: "Refresh"; objectName: "refreshEndpoints"; enabled: window.endpointSelectionBridge !== null; Accessible.description: "Refresh the available output capture endpoints."; onClicked: window.endpointSelectionBridge.refresh() }
                            }
                            Label { text: window.endpointSelectionBridge ? window.endpointSelectionBridge.status : "Output selection unavailable"; objectName: "endpointStatus"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Output endpoint status"; Accessible.description: text }
                            Label { text: window.endpointSelectionBridge && window.endpointSelectionBridge.selectedEndpoint !== "" ? "Selected endpoint: " + window.endpointSelectionBridge.selectedEndpoint : "No output endpoint selected"; objectName: "selectedEndpointStatus"; Accessible.name: "Selected output endpoint"; Accessible.description: text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        ComboBox {
                            id: outputDevice
                            objectName: "outputDevice"
                            model: ["Default system output"]
                            currentIndex: 0
                            enabled: true
                            Accessible.name: "Output device"
                            Accessible.description: "Monitoring currently follows the operating system default output device."
                            Layout.fillWidth: true
                        }
                        Label {
                            objectName: "permissionGuidance"
                            text: Qt.platform.os === "osx"
                                  ? "On macOS 14.2 or later, allow Beatglass in System Settings → Privacy & Security → Screen & System Audio Recording, then relaunch Beatglass before monitoring."
                                  : "System audio capture permission is not available yet; monitoring will remain stopped until a device is detected."
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                            Accessible.name: "Audio capture permission guidance"
                        }
                    }
                }
                GroupBox {
                    title: "Appearance"
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent; spacing: 8
                        Label { text: "Choose a color theme or enable reduced motion from the toolbar."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                }
                Label {
                    objectName: "settingsStatus"
                    text: "Settings are saved for this desktop profile. User audio files are never modified."
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
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
