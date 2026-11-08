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
    Shortcut {
        objectName: "playbackPauseShortcut"
        sequence: "Ctrl+Alt+Space"
        context: Qt.ApplicationShortcut
        enabled: window.playbackBridge !== null
            && (window.playbackBridge.state === "Playing" || window.playbackBridge.state === "Paused")
        onActivated: window.playbackBridge.state === "Paused"
            ? window.playbackBridge.resume()
            : window.playbackBridge.pause()
    }
    Shortcut { objectName: "libraryTabShortcut"; sequence: "Ctrl+Shift+1"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 0; libraryTab.forceActiveFocus() } }
    Shortcut { objectName: "djTabShortcut"; sequence: "Ctrl+Shift+2"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 1; djTab.forceActiveFocus() } }
    Shortcut { objectName: "settingsTabShortcut"; sequence: "Ctrl+Shift+3"; context: Qt.ApplicationShortcut; onActivated: { navigation.currentIndex = 2; settingsTab.forceActiveFocus() } }
    Shortcut {
        objectName: "visualizationWaveformShortcut"
        sequence: "Ctrl+1"
        context: Qt.ApplicationShortcut
        onActivated: { visualizationCanvas.viewMode = "waveform"; waveformView.forceActiveFocus() }
    }
    Shortcut {
        objectName: "visualizationSpectrumShortcut"
        sequence: "Ctrl+2"
        context: Qt.ApplicationShortcut
        onActivated: { visualizationCanvas.viewMode = "spectrum"; spectrumView.forceActiveFocus() }
    }
    property string selectedTheme: "Light"
    readonly property bool compactHeader: width < 760
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
            Connections {
                target: window.themePreferencesBridge
                function onReduceMotionChanged() {
                    if (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion) {
                        beatPulse.stop()
                        pulseFade.stop()
                        overlaySurface.pulseOpacity = 0.0
                    }
                }
            }
            Connections {
                target: window.liveBridge
                function onChanged() {
                    if (!window.themePreferencesBridge || !window.themePreferencesBridge.reduceMotion)
                        beatPulse.restart()
                }
            }
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
                        text: "×"; objectName: "overlayWindowHide"; Accessible.name: "Hide overlay"; Accessible.description: "Hide the overlay window and restore the control window"
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
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                }
                Label { text: "BPM " + window.liveBridge.tempo; color: "white"; font.pixelSize: 24; font.bold: true }
                Label { text: "Key " + window.liveBridge.key + "    Chord " + window.liveBridge.chord; color: "white" }
                Label { text: window.captureMonitorBridge ? (window.captureMonitorBridge.status === "Error" ? "Capture error — retry monitoring" : "Capture " + window.captureMonitorBridge.status) : "Capture unavailable"; color: window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "#fca5a5" : "#cbd5e1"; objectName: "overlayCaptureStatus"; Accessible.name: "Overlay capture status: " + text; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Current output capture status" }
                Button {
                    objectName: "overlayRetryCapture"
                    visible: window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" && window.captureMonitorBridge.start
                    text: "Retry capture"
                    ToolTip.text: "Retry monitoring the selected output source"
                    Accessible.name: "Retry overlay capture"
                    Accessible.description: "Retry monitoring the selected output source"
                    onClicked: window.captureMonitorBridge.start()
                }
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
    function clearVisualizationForCaptureState() {
        if (typeof visualizationCanvas !== "undefined") {
            visualizationCanvas.selectedPosition = -1
            visualizationCanvas.requestPaint()
        }
        if (typeof visualizationPresentation !== "undefined"
                && visualizationPresentation.clear)
            visualizationPresentation.clear()
    }
    function visualizationPositionToFrame(position, durationFrames) {
        if (!isFinite(position) || !isFinite(durationFrames) || durationFrames < 0)
            return -1
        const clamped = Math.max(0, Math.min(1, position))
        return Math.min(durationFrames, Math.floor(clamped * durationFrames))
    }
    function seekPlaybackToVisualizationPosition(position, durationFrames) {
        const frame = visualizationPositionToFrame(position, durationFrames)
        if (frame < 0 || !window.playbackBridge || window.playbackBridge.state === "Stopped")
            return false
        return window.playbackBridge.seek(frame, window.playbackBridge.epoch + 1)
    }
    function syncVisualizationSelectionFromPlayback(frame, durationFrames) {
        if (!isFinite(frame) || !isFinite(durationFrames) || durationFrames <= 0)
            return false
        visualizationCanvas.selectedPosition = Math.max(0, Math.min(1, frame / durationFrames))
        visualizationCanvas.requestPaint()
        return true
    }
    Connections {
        target: window.liveBridge
        function onStatusChanged() {
            if (window.liveBridge.status === "Stopped" || window.liveBridge.status === "Error"
                    || window.liveBridge.status === "Starting"
                    || window.liveBridge.status === "Reacquiring")
                window.clearVisualizationForCaptureState()
        }
    }
    Connections {
        target: window.playbackBridge
        function onSourceChanged() { window.clearVisualizationForCaptureState() }
        function onEpochChanged() { window.clearVisualizationForCaptureState() }
        function onStateChanged() {
            if (window.playbackBridge.state === "Stopped")
                window.clearVisualizationForCaptureState()
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
                TabButton { id: libraryTab; text: "Library"; objectName: "libraryTab"; Accessible.name: "Library"; Accessible.description: "Browse local audio files. Shortcut Ctrl+Shift+1; focus returns to this tab."; background: Rectangle { color: libraryTab.checked ? "#273b5d" : "transparent"; border.width: libraryTab.activeFocus ? 3 : 1; border.color: libraryTab.activeFocus ? "#60a5fa" : "#718096"; radius: 5 } }
                TabButton { id: djTab; text: "DJ"; objectName: "djTab"; Accessible.name: "DJ"; Accessible.description: "View live DJ controls and overlay settings. Shortcut Ctrl+Shift+2; focus returns to this tab."; background: Rectangle { color: djTab.checked ? "#273b5d" : "transparent"; border.width: djTab.activeFocus ? 3 : 1; border.color: djTab.activeFocus ? "#60a5fa" : "#718096"; radius: 5 } }
                TabButton { id: settingsTab; text: "Settings"; objectName: "settingsTab"; Accessible.name: "Settings"; Accessible.description: "Configure appearance, audio and overlay settings. Shortcut Ctrl+Shift+3; focus returns to this tab."; background: Rectangle { color: settingsTab.checked ? "#273b5d" : "transparent"; border.width: settingsTab.activeFocus ? 3 : 1; border.color: settingsTab.activeFocus ? "#60a5fa" : "#718096"; radius: 5 } }
            }
            ComboBox {
                id: themeSelector
                objectName: "themeSelector"
                model: window.themeNames
                currentIndex: window.selectedThemeIndex
                activeFocusOnTab: true
                Accessible.role: Accessible.ComboBox
                Accessible.value: currentText
                Accessible.name: "Color theme"
                Accessible.description: "Choose one of six color themes for the Beatglass desktop; current theme " + currentText
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
                Accessible.value: checked ? "enabled" : "disabled"
                activeFocusOnTab: true
                Accessible.name: "Reduce motion"
                Accessible.role: Accessible.CheckBox
                Accessible.description: "Reduce animation and transition effects in the desktop presentation; currently " + (checked ? "enabled" : "disabled")
                onClicked: if (window.themePreferencesBridge) window.themePreferencesBridge.reduceMotion = checked
            }
            Button {
                objectName: "resetAppearance"
                text: "Reset appearance"
                visible: !window.compactHeader
                Accessible.name: "Reset appearance settings"
                Accessible.description: "Restore the default Light theme and turn off reduced motion."
                ToolTip.text: "Restore Light theme and full motion"
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
                visible: !window.compactHeader
                spacing: 3
                Repeater {
                    model: window.themeNames.length
                    delegate: Button {
                        id: themePreviewButton
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
                            objectName: "themePreviewSurface" + index
                            radius: 3
                            color: window.themeBackgrounds[index]
                            border.width: window.selectedThemeIndex === index ? 2 : 1
                            border.color: window.themeAccentColors[index]
                            // Keep keyboard focus visible even when a theme's
                            // palette is dark or low contrast.  The ring is
                            // deliberately outside the swatch so it cannot be
                            // mistaken for the selected-state border.
                            Rectangle {
                                objectName: "themePreviewFocusRing" + index
                                anchors.fill: parent
                                anchors.margins: -3
                                radius: parent.radius + 2
                                color: "transparent"
                                border.width: 3
                                border.color: window.themeAccentColors[index]
                                visible: themePreviewButton.activeFocus
                            }
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
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "Choose folder…"; objectName: "chooseFolder"; activeFocusOnTab: true; Accessible.name: "Choose audio folder"; Accessible.description: "Choose a local folder to browse"; onClicked: folderPicker.open() }
                    Button {
                        text: "Up"; objectName: "parentFolder"
                        activeFocusOnTab: true
                        enabled: window.browseFolder.toString() !== "" && files.parentFolder.toString() !== ""
                        onClicked: window.browseFolder = files.parentFolder
                        Accessible.name: "Go to parent folder"
                        Accessible.description: "Go to the parent folder"
                    }
                    Label {
                        objectName: "folderPath"
                        text: window.browseFolder.toString() || "No folder selected"
                        elide: Text.ElideMiddle; Layout.fillWidth: true
                        Accessible.name: "Current folder: " + text
                        Accessible.role: Accessible.StaticText
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField { id: search; objectName: "search"; placeholderText: "Filter file names"; Accessible.name: "Filter file names"; Accessible.description: "Filter the current folder by audio or folder name"; Layout.fillWidth: true; onTextChanged: window.refreshCatalog() }
                    Button { objectName: "sortOrder"; text: window.sortDescending ? "Name ↓" : "Name ↑"; activeFocusOnTab: true; Accessible.name: "Reverse name sort order"; Accessible.description: "Current order: " + (window.sortDescending ? "descending" : "ascending") + ". Activate to reverse file-name order."; onClicked: window.sortDescending = !window.sortDescending }
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
                        Accessible.description: "Use Up and Down to select an entry, then Enter or Space to open a folder or select an audio file."
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
                                Accessible.role: Accessible.StaticText
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
                            Button { text: "Open"; objectName: "openPlayback"; activeFocusOnTab: true; Accessible.name: "Open selected playback source"; Accessible.description: window.selectedAudioUrl.toString() === "" ? "Select an audio source before opening playback" : "Open the selected audio source for playback"; enabled: window.selectedAudioUrl.toString() !== "" && window.playbackBridge; onClicked: window.playbackBridge.openSource(window.selectedAudioUrl, 1) }
                            SpinBox { objectName: "playbackSampleRate"; from: 8000; to: 192000; value: 44100; editable: true; Accessible.name: "Playback sample rate"; Accessible.value: value + " hertz"; Accessible.description: "Native playback sample rate in hertz"; ToolTip.text: "Choose the native playback sample rate in hertz" }
                            SpinBox { objectName: "playbackChannels"; from: 1; to: 8; value: 2; editable: true; Accessible.name: "Playback channels"; Accessible.value: value + " channels"; Accessible.description: "Native playback channel count"; ToolTip.text: "Choose the native playback channel count" }
                            Button { text: "Start native"; objectName: "startNativePlayback"; enabled: window.playbackBridge; Accessible.name: "Start native playback"; Accessible.role: Accessible.Button; Accessible.description: window.playbackBridge ? "Start native playback at " + playbackSampleRate.value + " hertz with " + playbackChannels.value + " channels on the selected output device" : "Native playback is unavailable in this build"; ToolTip.text: "Start native playback using the selected format"; onClicked: window.playbackBridge.startNative(window.playbackBridge.epoch + 1, playbackSampleRate.value, playbackChannels.value, 4096) }
                            Label { text: window.playbackBridge && window.playbackBridge.nativeRunning ? "Native output active" : "Native output stopped"; objectName: "nativePlaybackState"; Accessible.name: "Native playback state"; Accessible.role: Accessible.StaticText; Accessible.description: text }
                            Label { text: window.playbackBridge ? window.playbackBridge.nativeError : ""; objectName: "nativePlaybackError"; Accessible.name: "Native playback error"; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Native playback error details"; visible: text !== ""; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Button { text: "Retry native"; objectName: "retryNativePlayback"; enabled: window.playbackBridge && window.playbackBridge.nativeError !== ""; Accessible.name: "Retry native playback"; Accessible.role: Accessible.Button; Accessible.description: "Retry starting native playback after an output error"; ToolTip.text: "Retry native playback after the output error"; onClicked: window.playbackBridge.retryNative() }
                            Button { text: "Reopen selected"; objectName: "reopenSelectedPlayback"; visible: window.selectedAudioUrl.toString() !== "" && window.playbackBridge && !window.playbackMatchesSelection; enabled: visible; Accessible.name: "Reopen selected playback source"; Accessible.description: "Replace the current playback source with the selected audio source"; ToolTip.text: "Replace playback with the selected audio source"; onClicked: window.playbackBridge.openSource(window.selectedAudioUrl, window.playbackBridge.epoch + 1) }
                            Button { text: window.playbackBridge && window.playbackBridge.state === "Paused" ? "Resume" : "Pause"; objectName: "pausePlayback"; Accessible.name: text + " playback"; Accessible.description: window.playbackBridge && window.playbackBridge.state === "Paused" ? "Resume the selected audio source." : "Pause the selected audio source."; ToolTip.text: "Pause or resume playback"; enabled: window.playbackBridge && (window.playbackBridge.state === "Playing" || window.playbackBridge.state === "Paused"); onClicked: window.playbackBridge.state === "Paused" ? window.playbackBridge.resume() : window.playbackBridge.pause() }
                            Button { text: "Stop"; objectName: "stopPlayback"; Accessible.name: "Stop playback"; Accessible.description: "Stop playback and clear the current source."; ToolTip.text: "Stop playback and clear the current source"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: window.playbackBridge.stop() }
                            Button { text: "−5 s"; objectName: "seekBackPlayback"; Accessible.name: "Seek backward five seconds"; Accessible.description: "Move playback five seconds earlier"; ToolTip.text: "Seek backward 5 seconds"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: window.playbackBridge.seek(Math.max(0, window.playbackBridge.currentFrame - 5 * 44100), window.playbackBridge.epoch + 1) }
                            Button { text: "+5 s"; objectName: "seekForwardPlayback"; Accessible.name: "Seek forward five seconds"; Accessible.description: "Move playback five seconds later"; ToolTip.text: "Seek forward 5 seconds"; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; onClicked: window.playbackBridge.seek(window.playbackBridge.currentFrame + 5 * 44100, window.playbackBridge.epoch + 1) }
                            SpinBox { id: seekFrame; objectName: "seekFrame"; from: 0; to: 2147483647; value: window.playbackBridge ? Math.min(seekFrame.to, window.playbackBridge.currentFrame) : 0; editable: true; enabled: window.playbackBridge && window.playbackBridge.state !== "Stopped"; Accessible.name: "Seek target frame"; Accessible.value: value; Accessible.description: "Enter an audio frame and apply it as the playback position"; ToolTip.text: "Enter a source frame, then choose Seek" }
                            Button { text: "Seek"; objectName: "seekToFrame"; enabled: seekFrame.enabled; Accessible.name: "Seek to entered frame"; Accessible.description: "Seek playback to the entered audio frame"; ToolTip.text: "Seek playback to the entered source frame"; onClicked: window.playbackBridge.seek(seekFrame.value, window.playbackBridge.epoch + 1) }
                            ComboBox { objectName: "playbackDevice"; model: window.playbackBridge ? window.playbackBridge.devices : []; activeFocusOnTab: true; enabled: window.playbackBridge; Accessible.name: "Playback output device"; Accessible.role: Accessible.ComboBox; Accessible.value: currentText; Accessible.description: currentText !== "" ? "Current output device: " + currentText + ". Choose the native playback output device." : "No output device selected. Choose the native playback output device."; ToolTip.text: "Choose the output device used for native playback"; onActivated: if (window.playbackBridge) window.playbackBridge.deviceId = currentIndex >= 0 ? currentIndex : -1 }
                            Button { text: "Refresh outputs"; objectName: "refreshPlaybackDevices"; activeFocusOnTab: true; enabled: window.playbackBridge; Accessible.name: "Refresh playback outputs"; Accessible.description: "Refresh native playback output devices"; ToolTip.text: "Refresh the available playback output devices"; onClicked: window.playbackBridge.refreshDevices() }
                            Label { text: window.playbackBridge ? "Playback: " + window.playbackBridge.state : "Playback unavailable"; objectName: "playbackState"; Accessible.name: "Playback state: " + text; Accessible.value: window.playbackBridge ? window.playbackBridge.state : "unavailable"; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Current native playback state" }
                            Label { text: window.playbackBridge && window.playbackBridge.source.toString() !== "" ? "Source: " + window.playbackBridge.source.fileName : "No source opened"; objectName: "playbackSource"; Accessible.name: "Playback source: " + text; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Current playback source"; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Label { text: window.playbackBridge ? "Frame " + window.playbackBridge.currentFrame + " · Epoch " + window.playbackBridge.epoch : ""; objectName: "playbackPosition"; Accessible.name: "Playback position: " + text; Accessible.value: window.playbackBridge ? window.playbackBridge.currentFrame : 0; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Current source frame and playback epoch" }
                            Label { text: window.playbackBridge && window.playbackBridge.state !== "Stopped" ? "Seek uses audio frames; track duration is not available yet." : ""; objectName: "playbackSeekLimitation"; Accessible.name: "Playback seek limitation"; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Playback seek currently uses source audio frames"; visible: text !== ""; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                            Label { text: window.playbackMatchesSelection ? "Playing selected source" : (window.selectedAudioUrl.toString() !== "" && window.playbackBridge && window.playbackBridge.source.toString() !== "" ? "Playback is on another source" : ""); objectName: "playbackSelectionStatus"; Accessible.name: "Playback selection status: " + text; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Whether playback matches the selected library source"; visible: text !== "" }
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
                            Accessible.name: "Selected audio file: " + text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                        }
                        Label {
                            objectName: "selectedAudioSource"
                            text: window.selectedAudioUrl.toString() !== "" ? "Source: " + window.selectedAudioUrl : "Source unavailable"
                            elide: Text.ElideMiddle; Layout.fillWidth: true
                            Accessible.name: "Selected audio source: " + text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                        }
                        Label {
                            objectName: "selectedAudioDuration"
                            text: window.selectedAudioUrl.toString() !== "" ? "Duration: unknown until decoded metadata is available" : "Duration unavailable"
                            Accessible.name: "Selected audio duration: " + text
                            Accessible.role: Accessible.StaticText
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
                            Accessible.name: "Audio metadata status: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
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
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            objectName: "detailsAvailability"
                            text: window.selectedName ? ("BPM " + analysisPresentation.bpm + " · Key " + analysisPresentation.key + " · Beats " + analysisPresentation.beatCount + " · Observed frames " + analysisPresentation.observedFrames) : "Select an audio file to inspect its read-only details."
                            Layout.fillWidth: true; wrapMode: Text.WordWrap
                            Accessible.name: "Analysis and playback status: " + text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                        }
                        Label {
                            objectName: "analysisUncertainty"
                            text: window.selectedName
                                  ? "Track key: Unknown · No validated track-level key. Window evidence is provisional and source-attributed."
                                  : "Track key: Unknown until a source is selected."
                            Accessible.name: "Analysis uncertainty: " + text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            objectName: "analysisFreshness"
                            text: window.selectedName && window.analysisJobBridge && window.analysisJobBridge.status === "Complete" ? "Analysis freshness: source-linked analysis complete." : "Analysis freshness: waiting for source-linked analysis evidence."
                            Accessible.name: "Analysis freshness: " + text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Label {
                            objectName: "sourceWorkflowStatus"
                            text: window.selectedAudioUrl.toString() === ""
                                  ? "Select an audio source to inspect and play it."
                                  : "Source workflow · playback: " + (window.playbackBridge ? window.playbackBridge.state : "unavailable") + " · " + window.metadataStatus
                            Accessible.name: "Selected source workflow status"
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        RowLayout { Layout.fillWidth: true
                            Label {
                                text: !window.selectedName ? "Queued · select an audio source to begin analysis." : (window.analysisJobBridge ? (window.analysisJobBridge.status + " · observed " + window.analysisJobBridge.frames + " frames.") : "Analysis progress unavailable")
                                objectName: "analysisProgress"
                                Accessible.name: "Analysis progress: " + text
                                Accessible.role: Accessible.StaticText
                                Accessible.description: text
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "analysisBatchStatus"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.batchAcceptedCount !== undefined
                                text: visible ? ("Batch queue · accepted " + window.analysisJobBridge.batchAcceptedCount
                                      + " · rejected " + window.analysisJobBridge.batchRejectedCount
                                      + (window.analysisJobBridge.batchCapacity !== undefined ? (" · capacity " + window.analysisJobBridge.batchCapacity) : "")) : ""
                                Accessible.name: "Batch analysis queue status: " + text
                                Accessible.role: Accessible.StaticText
                                Accessible.description: visible ? text : "Batch queue status is unavailable until the analysis bridge reports it."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            Label {
                                objectName: "persistedRecoveryStatus"
                                visible: typeof storageRecovery !== "undefined"
                                text: visible ? (storageRecovery.status + (storageRecovery.retryableRunIds.length > 0 ? (" · retryable runs " + storageRecovery.retryableRunIds.length) : "")) : ""
                                Accessible.name: "Persisted analysis recovery status: " + text
                                Accessible.role: Accessible.StaticText
                                Accessible.description: visible ? text : "Persisted recovery status is unavailable."
                                Layout.fillWidth: true; wrapMode: Text.WordWrap
                            }
                            ComboBox {
                                objectName: "persistedRecoveryRuns"
                                visible: typeof storageRecovery !== "undefined"
                                model: visible ? storageRecovery.retryableRunIds : []
                                enabled: visible && count > 0
                                Accessible.name: "Persisted retryable analysis runs"
                                onActivated: storageRecovery.selectRun(currentText)
                            }
                            Button {
                                objectName: "discardPersistedRecovery"
                                visible: typeof storageRecovery !== "undefined"
                                enabled: visible && storageRecovery.selectedRunId !== ""
                                text: "Discard saved run"
                                Accessible.name: "Discard saved retryable analysis run"
                                Accessible.description: "Remove the selected retryable run record without touching the source audio file."
                                onClicked: discardRecoveryDialog.open()
                            }
                            Dialog {
                                id: discardRecoveryDialog
                                title: "Discard saved analysis run?"
                                modal: true
                                standardButtons: Dialog.Ok | Dialog.Cancel
                                contentItem: Label {
                                    text: "This removes only the retry record. The source audio file is not changed."
                                    wrapMode: Text.WordWrap
                                    Accessible.name: "Discard retryable run confirmation"
                                    Accessible.role: Accessible.StaticText
                                }
                                onAccepted: storageRecovery.discardSelectedRun()
                            }
                            Button {
                                objectName: "retryAnalysisBatch"
                                text: "Retry batch"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.retryBatch !== undefined && window.analysisJobBridge.batchAcceptedCount > 0
                                enabled: visible && (window.analysisJobBridge.status === "Failed" || window.analysisJobBridge.status === "Cancelled")
                                Accessible.name: "Retry analysis batch"
                                Accessible.description: "Retry cancelled or failed analysis items retained in the current batch."
                                onClicked: window.analysisJobBridge.retryBatch()
                            }
                            Button {
                                objectName: "cancelAnalysisBatch"
                                text: "Cancel batch"
                                visible: window.analysisJobBridge !== null && window.analysisJobBridge.cancelBatch !== undefined && window.analysisJobBridge.batchAcceptedCount > 0
                                enabled: visible && (window.analysisJobBridge.status === "Queued" || window.analysisJobBridge.status === "Running")
                                Accessible.name: "Cancel analysis batch"
                                Accessible.description: "Cancel all queued or running analysis items retained in the current batch."
                                onClicked: window.analysisJobBridge.cancelBatch()
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
                    Button { text: window.captureMonitorBridge && window.captureMonitorBridge.status === "Monitoring" ? "Stop monitoring" : (window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "Retry monitoring" : (window.captureMonitorBridge && window.captureMonitorBridge.status === "Starting" ? "Starting…" : (window.captureMonitorBridge && window.captureMonitorBridge.status === "Reacquiring" ? "Reacquiring…" : "Start monitoring"))); objectName: "toggleMonitoring"; activeFocusOnTab: true; enabled: window.captureMonitorBridge !== null && window.captureMonitorBridge.status !== "Starting" && window.captureMonitorBridge.status !== "Reacquiring"; Accessible.name: text; Accessible.role: Accessible.Button; Accessible.description: window.captureMonitorBridge && window.captureMonitorBridge.status === "Error" ? "Retry monitoring after a capture error." : "Start or stop live output monitoring."; onClicked: window.captureMonitorBridge.status === "Monitoring" ? window.captureMonitorBridge.stop() : window.captureMonitorBridge.start() }
                    Label { text: window.captureMonitorBridge ? (window.captureMonitorBridge.status === "Error" ? "Capture error — retry monitoring" : "Capture: " + window.captureMonitorBridge.status) : "Capture unavailable"; objectName: "captureStatus"; Accessible.name: "Capture monitor status: " + text; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text }
                }
                Label { text: "Live output monitoring: " + window.liveBridge.status; font.pixelSize: 18; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Live output monitoring"; Accessible.role: Accessible.StaticText; Accessible.description: text }
                Label { text: window.liveBridge.status === "Stopped" ? "Start monitoring from the DJ controls when an output source is available." : "Live estimates update below as output audio is captured."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    objectName: "visualizationModeControls"
                    Button {
                        id: waveformView
                        text: "Waveform"; objectName: "waveformView"; checkable: true; checked: visualizationCanvas.viewMode === "waveform"; Accessible.value: checked ? "selected" : "not selected"
                        Accessible.name: "Waveform view"; Accessible.description: "Show the selectable time-domain waveform."
                        ToolTip.text: "Show the time-domain waveform (Ctrl+1)"
                        onClicked: visualizationCanvas.viewMode = "waveform"
                        Keys.onRightPressed: { visualizationCanvas.viewMode = "spectrum"; spectrumView.forceActiveFocus(); event.accepted = true }
                        Keys.onLeftPressed: { visualizationCanvas.viewMode = "waveform"; event.accepted = true }
                        background: Rectangle {
                            color: waveformView.checked ? "#273b5d" : "#182231"
                            border.width: waveformView.activeFocus ? 3 : 1
                            border.color: waveformView.activeFocus ? "#60a5fa" : "#718096"
                            radius: 6
                        }
                    }
                    Button {
                        id: spectrumView
                        text: "FFT"; objectName: "spectrumView"; checkable: true; checked: visualizationCanvas.viewMode === "spectrum"; Accessible.value: checked ? "selected" : "not selected"
                        Accessible.name: "FFT view"; Accessible.description: "Show the selectable frequency-domain FFT."
                        ToolTip.text: "Show the frequency-domain FFT (Ctrl+2)"
                        onClicked: visualizationCanvas.viewMode = "spectrum"
                        Keys.onLeftPressed: { visualizationCanvas.viewMode = "waveform"; waveformView.forceActiveFocus(); event.accepted = true }
                        Keys.onRightPressed: { visualizationCanvas.viewMode = "spectrum"; event.accepted = true }
                        background: Rectangle {
                            color: spectrumView.checked ? "#432c56" : "#182231"
                            border.width: spectrumView.activeFocus ? 3 : 1
                            border.color: spectrumView.activeFocus ? "#c084fc" : "#718096"
                            radius: 6
                        }
                    }
                    Label {
                        text: visualizationCanvas.viewMode === "waveform" ? "Time domain" : "Frequency domain"
                        objectName: "visualizationModeStatus"
                    Accessible.name: "Visualization mode: " + text
                    Accessible.value: visualizationCanvas.viewMode
                        Accessible.role: Accessible.StaticText
                        Accessible.description: text + ". Use Left and Right arrows to switch views, or Ctrl+1 and Ctrl+2."
                    }
                }
                Canvas {
                    id: visualizationCanvas; objectName: "visualizationCanvas"
                    property string viewMode: "waveform"
                    property real selectedPosition: -1
                    activeFocusOnTab: true
                    Keys.onLeftPressed: {
                        selectedPosition = Math.max(0, (selectedPosition < 0 ? 0 : selectedPosition) - 0.01)
                        requestPaint()
                    }
                    Keys.onRightPressed: {
                        selectedPosition = Math.min(1, (selectedPosition < 0 ? 0 : selectedPosition) + 0.01)
                        requestPaint()
                    }
                    Keys.onHomePressed: { selectedPosition = 0; requestPaint() }
                    Keys.onEndPressed: { selectedPosition = 1; requestPaint() }
                    readonly property bool modeHasData: typeof visualizationPresentation !== "undefined"
                        && (viewMode === "waveform"
                            ? visualizationPresentation.waveform.length > 0
                            : visualizationPresentation.spectrum.length > 0)
                    Layout.fillWidth: true; Layout.preferredHeight: 120
                    Accessible.name: "Live " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT spectrum")
                    Accessible.description: (visualizationCanvas.viewMode === "waveform"
                        ? "Bounded time-domain waveform from the current output capture."
                        : "Bounded frequency-domain FFT magnitudes from the current output capture.")
                        + " " + (visualizationCanvas.selectedPosition < 0 ? "No position selected." : "Selected position " + Math.round(visualizationCanvas.selectedPosition * 100) + " percent.")
                    ToolTip.text: "Click to select a position; use Left/Right/Home/End when focused"
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
                        if (selectedPosition >= 0) {
                            ctx.strokeStyle = "#facc15"; ctx.lineWidth = 2; ctx.beginPath()
                            ctx.moveTo(selectedPosition * width, 0); ctx.lineTo(selectedPosition * width, height); ctx.stroke()
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            visualizationCanvas.selectedPosition = Math.max(0, Math.min(1, mouse.x / visualizationCanvas.width))
                            visualizationCanvas.requestPaint()
                        }
                    }
                    Rectangle {
                        objectName: "visualizationFocusRing"
                        anchors.fill: parent
                        color: "transparent"
                        border.width: 3
                        border.color: "#60a5fa"
                        visible: visualizationCanvas.activeFocus
                        z: 2
                    }
                    Connections { target: typeof visualizationPresentation !== "undefined" ? visualizationPresentation : null; function onChanged() { visualizationCanvas.requestPaint() } }
                }
                Label {
                    objectName: "visualizationEmptyState"
                    visible: !visualizationCanvas.modeHasData
                    text: window.liveBridge.status === "Error"
                        ? "Capture error; retry monitoring to populate the " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT") + "."
                        : window.liveBridge.status === "Stopped"
                        ? "Start monitoring to populate the " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT") + "."
                        : "Waiting for captured audio data to populate the " + (visualizationCanvas.viewMode === "waveform" ? "waveform" : "FFT") + "."
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Accessible.name: "Visualization empty state"
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                }
                Label {
                    objectName: "visualizationLoadState"
                    text: typeof visualizationPresentation === "undefined"
                        ? "Unavailable"
                        : (visualizationCanvas.modeHasData ? "Ready" : (window.liveBridge.status === "Tracking" ? "Waiting for capture data" : "Idle"))
                    Accessible.name: "Visualization load state: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Current waveform/FFT presentation load state"
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "visualizationStatus"
                    text: typeof visualizationPresentation === "undefined"
                          ? "Visualization unavailable"
                          : (visualizationPresentation.waveform.length === 0 && visualizationPresentation.spectrum.length === 0
                             ? "Waiting for captured audio data"
                             : "Waveform samples: " + visualizationPresentation.waveform.length + " · Spectrum bins: " + visualizationPresentation.spectrum.length)
                    Accessible.name: "Visualization data status"
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "visualizationDataStatus"
                    text: (typeof visualizationPresentation !== "undefined" ? visualizationPresentation.waveform.length : 0) + " waveform samples · " + (typeof visualizationPresentation !== "undefined" ? visualizationPresentation.spectrum.length : 0) + " spectrum bins"
                    Accessible.name: "Visualization data status: " + text; Accessible.role: Accessible.StaticText; Accessible.description: text
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "visualizationSelectedDataStatus"
                    text: visualizationCanvas.viewMode === "waveform"
                        ? ((typeof visualizationPresentation !== "undefined" ? visualizationPresentation.waveform.length : 0) + " samples in selected waveform view")
                        : ((typeof visualizationPresentation !== "undefined" ? visualizationPresentation.spectrum.length : 0) + " bins in selected FFT view")
                    Accessible.name: "Selected visualization data status: " + text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Data available in the selected waveform or FFT view"
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "visualizationSelectionStatus"
                    text: visualizationCanvas.selectedPosition < 0 ? "No visualization position selected" : "Selected visualization position " + Math.round(visualizationCanvas.selectedPosition * 100) + "%"
                    Accessible.name: "Visualization selection status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: "Click the canvas or use Left/Right/Home/End to select a presentation position. " + text
                    ToolTip.text: "Current selected waveform/FFT position"
                }
                Button {
                    objectName: "clearVisualizationSelection"
                    activeFocusOnTab: true
                    text: "Clear position"
                    ToolTip.text: "Clear the selected waveform or FFT position"
                    enabled: visualizationCanvas.selectedPosition >= 0
                    Accessible.name: "Clear visualization position"
                    Accessible.description: "Remove the selected waveform or FFT position"
                    onClicked: {
                        visualizationCanvas.selectedPosition = -1
                        visualizationCanvas.requestPaint()
                    }
                }
                Label {
                    objectName: "visualizationRevisionStatus"
                    text: typeof visualizationPresentation === "undefined"
                        ? "No visualization generation is active"
                        : "Visualization generation " + visualizationPresentation.generation + " · revision " + visualizationPresentation.revision
                    Accessible.name: "Visualization generation status: " + text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Current accepted visualization generation and revision"
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "visualizationCaptureStatus"
                    text: window.liveBridge.status === "Stopped" ? "Capture is stopped; start monitoring to populate the visualization." : "Capture is active; visualization updates from the current output stream."
                    Accessible.name: "Visualization capture status: " + text
                    Accessible.role: Accessible.StaticText
                    Accessible.description: text
                    ToolTip.text: "Current capture state feeding waveform and FFT data"
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                Label {
                    objectName: "djStatus"; text: "Status: " + window.liveBridge.status
                    Accessible.name: "DJ monitoring status"; Accessible.role: Accessible.StaticText; Accessible.description: text
                }
                Label {
                    objectName: "djEstimates"
                    text: "BPM: " + window.liveBridge.tempo + "    Key: " + window.liveBridge.key + "    Chord: " + window.liveBridge.chord
                    Accessible.name: "DJ estimates"; Accessible.role: Accessible.StaticText; Accessible.description: text
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
                     Accessible.name: "DJ evidence status: " + text
                     Accessible.role: Accessible.StaticText
                     Accessible.description: text
                     wrapMode: Text.WordWrap; Layout.fillWidth: true
                 }
                RowLayout {
                    visible: window.overlayBridge !== null
                    Button { text: "Show overlay"; objectName: "showOverlay"; Accessible.name: "Show DJ overlay"; Accessible.role: Accessible.Button; Accessible.description: "Show the DJ overlay window"; ToolTip.text: "Show the DJ overlay and hide the control window"; enabled: window.overlayBridge && window.overlayBridge.available; onClicked: window.showOverlayAndHideControls() }
                    Button { text: "Unlock overlay"; objectName: "unlockOverlay"; Accessible.name: "Unlock DJ overlay"; Accessible.description: "Restore keyboard and pointer access to the overlay"; ToolTip.text: "Restore keyboard and pointer access to the overlay"; enabled: window.overlayBridge && window.overlayBridge.clickThrough; onClicked: window.overlayBridge.setClickThrough(false) }
                    Button { text: "Hide overlay"; objectName: "hideOverlay"; Accessible.name: "Hide DJ overlay"; Accessible.description: "Hide the DJ overlay window"; ToolTip.text: "Hide the DJ overlay and restore the control window"; enabled: window.overlayBridge && window.overlayBridge.available; onClicked: window.hideOverlayAndRestoreControls() }
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    CheckBox {
                        id: clickThroughToggle
                        text: "Click through"; objectName: "clickThrough"
                        activeFocusOnTab: true
                        Accessible.name: checked ? "Disable click-through" : "Enable click-through"
                        Accessible.value: checked ? "enabled" : "disabled"
                        Accessible.role: Accessible.CheckBox
                        Accessible.description: "Allow pointer input to pass through the overlay window; currently " + (checked ? "enabled" : "disabled")
                        ToolTip.text: "Toggle pointer click-through for the overlay"
                        enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.screenNames.length > 0
                        onClicked: if (window.overlayBridge) {
                            window.overlayBridge.setClickThrough(checked)
                            checked = window.overlayBridge.clickThrough
                        }
                        Connections {
                            target: window.overlayBridge
                            function onClickThroughChanged() { clickThroughToggle.checked = window.overlayBridge.clickThrough }
                        }
                    }
                    Label { objectName: "overlayInteractionMode"; text: window.overlayBridge ? (window.overlayBridge.clickThrough ? "Input mode: click-through" : "Input mode: interactive") : "Input mode unavailable"; Accessible.name: "Overlay interaction mode: " + text; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text; ToolTip.text: "Current overlay pointer interaction mode" }
                    Label { text: "Opacity" }
                    Slider { id: overlayOpacityControl; objectName: "overlayOpacity"; Accessible.name: "Overlay opacity"; Accessible.role: Accessible.Slider; Accessible.value: Math.round(value * 100) + "%"; Accessible.description: "Adjust the overlay window opacity; current value " + Math.round(value * 100) + "%"; ToolTip.text: "Adjust overlay opacity from 20% to 100%"; from: 0.2; to: 1; value: window.overlayBridge ? window.overlayBridge.opacity : 0.9; enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.screenNames.length > 0; onMoved: if (window.overlayBridge) window.overlayBridge.setOpacity(value) }
                    Connections {
                        target: window.overlayBridge
                        function onOpacityChanged() { if (window.overlayBridge) overlayOpacityControl.value = window.overlayBridge.opacity }
                    }
                    Label { objectName: "overlayOpacityValue"; text: window.overlayBridge ? "Current opacity: " + Math.round(window.overlayBridge.opacity * 100) + "%" : "Opacity unavailable"; Accessible.name: "Current overlay opacity: " + text; Accessible.role: Accessible.StaticText; Accessible.description: text }
                }
                RowLayout {
                    visible: window.overlayBridge !== null
                    Label { text: "Display" }
                    ComboBox {
                        objectName: "overlayDisplay"
                    Accessible.name: "Overlay display"; Accessible.description: "Choose the display where the overlay is shown; current display " + (currentIndex >= 0 && count > currentIndex ? currentText : "unavailable"); ToolTip.text: "Choose the display where the overlay is shown"
                        enabled: window.overlayBridge && window.overlayBridge.available && count > 0
                        model: window.overlayBridge ? window.overlayBridge.screenNames : []
                        currentIndex: window.overlayBridge && window.overlayBridge.screenIndex >= 0 ? window.overlayBridge.screenIndex : 0
                        onActivated: if (window.overlayBridge) window.overlayBridge.moveToScreen(currentIndex)
                        Layout.fillWidth: true
                    }
                    Label { objectName: "overlayDisplayStatus"; text: window.overlayBridge && window.overlayBridge.screenIndex >= 0 && window.overlayBridge.screenIndex < window.overlayBridge.screenNames.length ? "Current display: " + window.overlayBridge.screenNames[window.overlayBridge.screenIndex] : "Current display unavailable"; Accessible.name: "Current overlay display: " + text; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text }
                    Label {
                        objectName: "overlayDisplayEmptyState"
                        visible: window.overlayBridge && window.overlayBridge.screenNames.length === 0
                        text: "No displays are available for the overlay."
                        Accessible.name: "Overlay display empty state"
                        Accessible.description: text
                    }
                    Label { text: "Size" }
                    ComboBox {
                        objectName: "overlaySize"
                    Accessible.name: "Overlay size"; Accessible.value: currentText; Accessible.description: "Choose the overlay window size; current size " + currentText; ToolTip.text: "Choose Compact, Medium, or Large overlay size"
                        model: ["Compact", "Medium", "Large"]; currentIndex: 1
                        enabled: window.overlayBridge && window.overlayBridge.available && window.overlayBridge.screenNames.length > 0
                        onActivated: if (window.overlayBridge) {
                            const sizes = [[250, 110], [310, 130], [430, 180]]
                            window.overlayBridge.resizeOverlay(sizes[currentIndex][0], sizes[currentIndex][1])
                        }
                    }
                }
                Label { visible: window.overlayBridge !== null && !window.overlayBridge.available; text: "Overlay unavailable on this display."; objectName: "overlayUnavailable"; Accessible.name: "Overlay availability: " + text; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text; wrapMode: Text.WordWrap }
                Button { visible: window.overlayBridge !== null && !window.overlayBridge.available; text: "Retry overlay"; objectName: "retryOverlay"; Accessible.name: "Retry DJ overlay"; Accessible.description: "Retry showing the native DJ overlay"; ToolTip.text: "Retry showing the native DJ overlay on the selected display"; onClicked: window.showOverlayAndHideControls() }
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
                                    activeFocusOnTab: true
                                    Accessible.role: Accessible.ComboBox
                                    model: window.endpointSelectionBridge ? window.endpointSelectionBridge.endpoints : []
                                    textRole: "name"
                                    enabled: count > 0; Layout.fillWidth: true
                                    Accessible.name: "Output capture endpoint"
                                    Accessible.description: currentText !== "" ? "Current capture endpoint: " + currentText + ". Choose the output endpoint used for live capture." : "No capture endpoint selected. Choose the output endpoint used for live capture."
                                    onActivated: if (window.endpointSelectionBridge) window.endpointSelectionBridge.select(currentValue.id)
                                }
                                Button { text: "Refresh"; objectName: "refreshEndpoints"; activeFocusOnTab: true; enabled: window.endpointSelectionBridge !== null; Accessible.name: "Refresh capture endpoints"; Accessible.role: Accessible.Button; Accessible.description: "Refresh the available output capture endpoints."; onClicked: window.endpointSelectionBridge.refresh() }
                            }
                            Label {
                                objectName: "endpointEmptyState"
                                visible: window.endpointSelectionBridge && window.endpointSelectionBridge.endpoints.length === 0
                                text: "No output endpoints detected. Refresh after connecting an output device."
                                Accessible.name: "Output endpoint empty state"
                                Accessible.role: Accessible.StaticText
                                Accessible.description: text
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label { text: window.endpointSelectionBridge ? window.endpointSelectionBridge.status : "Output selection unavailable"; objectName: "endpointStatus"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Output endpoint status: " + text; Accessible.value: text; Accessible.role: Accessible.StaticText; Accessible.description: text }
                            Label {
                                objectName: "endpointUnavailableState"
                                visible: window.endpointSelectionBridge === null
                                text: "Output endpoint discovery is unavailable in this build."
                                Accessible.name: "Output endpoint discovery availability"
                                Accessible.value: text
                                Accessible.role: Accessible.StaticText
                                Accessible.description: text
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label {
                                objectName: "endpointRecoveryGuidance"
                                visible: window.endpointSelectionBridge && window.endpointSelectionBridge.status.indexOf("Error") >= 0
                                text: "Output endpoint recovery is available: connect an output device and refresh the endpoint list."
                                Accessible.name: "Output endpoint recovery guidance"
                                Accessible.value: text
                                Accessible.role: Accessible.StaticText
                                Accessible.description: text
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label { text: window.endpointSelectionBridge && window.endpointSelectionBridge.selectedEndpoint !== "" ? "Selected endpoint: " + window.endpointSelectionBridge.selectedEndpoint : "No output endpoint selected"; objectName: "selectedEndpointStatus"; Accessible.name: "Selected output endpoint: " + text; Accessible.role: Accessible.StaticText; Accessible.description: text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        ComboBox {
                            id: outputDevice
                            objectName: "outputDevice"
                            model: ["Default system output"]
                            currentIndex: 0
                            enabled: true
                            Accessible.name: "Output device"
                            Accessible.role: Accessible.ComboBox
                            Accessible.description: "Current output device: " + currentText + ". Monitoring follows the operating system default output device."
                            Layout.fillWidth: true
                        }
                        Label {
                            objectName: "permissionGuidance"
                            text: Qt.platform.os === "osx"
                                  ? "On macOS 14.2 or later, allow Beatglass in System Settings → Privacy & Security → Screen & System Audio Recording, then relaunch Beatglass before monitoring."
                                  : "System audio capture permission is not available yet; monitoring will remain stopped until a device is detected."
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                            Accessible.name: "Audio capture permission guidance"
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                        }
                    }
                }
                GroupBox {
                    title: "Appearance"
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent; spacing: 8
                        Label { text: "Choose a color theme or enable reduced motion from the toolbar."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Label {
                            objectName: "appearanceState"
                            text: "Theme: " + window.selectedTheme + " · Motion: " + (window.themePreferencesBridge && window.themePreferencesBridge.reduceMotion ? "reduced" : "full")
                            Accessible.name: "Current appearance settings: " + text
                            Accessible.value: text
                            Accessible.role: Accessible.StaticText
                            Accessible.description: text
                            Layout.fillWidth: true
                        }
                    }
                }
                Label {
                    objectName: "settingsStatus"
                    text: "Settings are saved for this desktop profile. User audio files are never modified."
                    Accessible.name: "Settings status: " + text
                    Accessible.value: text
                    Accessible.role: Accessible.StaticText
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
