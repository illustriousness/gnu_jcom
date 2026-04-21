import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1540
    height: 920
    minimumWidth: 1220
    minimumHeight: 760
    visible: true
    title: "gnu_jcom"
    color: "#d7dade"

    readonly property color chromeBorder: "#aab1b9"
    readonly property color chromeFill: "#eceef1"
    readonly property color toolbarStart: "#5b7387"
    readonly property color toolbarEnd: "#516a7f"
    readonly property color panelFill: "#f3f4f6"
    readonly property color panelBorder: "#bcc3ca"
    readonly property color textStrong: "#20252b"
    readonly property color textMuted: "#65717d"
    readonly property color accent: "#118bd1"
    readonly property color accentDark: "#0f78b4"
    readonly property color good: "#34815e"
    readonly property color warn: "#bc7724"
    readonly property color bad: "#b34d45"
    readonly property color terminalFill: "#f8f8f8"
    readonly property color terminalGrid: "#eef1f4"
    readonly property color terminalText: "#2e3740"

    property var baudRates: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]
    property var dataBitsOptions: [5, 6, 7, 8]
    property var stopBitsOptions: [1, 2]
    property var parityOptions: ["None", "Even", "Odd", "Space", "Mark"]
    property var flowOptions: ["None", "Hardware", "Software"]
    property var lineEndingOptions: ["None", "LF", "CRLF", "CR"]
    property bool toolsExpanded: false

    function syncCombo(combo, values, value) {
        const idx = values.indexOf(value)
        if (idx >= 0 && combo.currentIndex !== idx) {
            combo.currentIndex = idx
        }
    }

    function resetTemplateEditor() {
        sendListView.currentIndex = -1
        templateNameField.text = ""
        templatePayloadField.text = ""
        templateModeGroup.currentIndex = 0
    }

    function loadTemplateEditor(index) {
        const item = app.sendItemAt(index)
        if (!item || Object.keys(item).length === 0) {
            return
        }

        templateNameField.text = item.name
        templatePayloadField.text = item.payload
        templateModeGroup.currentIndex = item.mode
    }

    function basename(path) {
        if (!path || path.length === 0) {
            return "-"
        }

        const normalized = path.replace(/\\/g, "/")
        const parts = normalized.split("/")
        return parts.length > 0 ? parts[parts.length - 1] : normalized
    }

    function setComposerMode(useHex) {
        const nextMode = useHex ? 1 : 0
        if (nextMode === app.composeMode) {
            return
        }

        composerArea.text = app.convertPayloadForMode(composerArea.text, app.composeMode, nextMode)
        app.composeMode = nextMode

        if (periodicSendCheck.checked) {
            if (!app.startPeriodicSend(composerArea.text, app.composeMode, periodicIntervalBox.value)) {
                periodicSendCheck.checked = false
            }
        }
    }

    menuBar: MenuBar {
        Menu {
            title: "终端模式"

            Action {
                text: "清空接收区"
                onTriggered: app.clearLogs()
            }

            Action {
                text: "保存日志"
                onTriggered: app.saveLog(logPathField.text)
            }
        }

        Menu {
            title: "设置"

            Action {
                text: window.toolsExpanded ? "收起扩展" : "展开扩展"
                onTriggered: window.toolsExpanded = !window.toolsExpanded
            }

            Action {
                text: "恢复默认路径"
                onTriggered: {
                    workspaceField.text = app.defaultWorkspacePath
                    app.workspacePath = workspaceField.text
                    logPathField.text = app.defaultLogExportPath
                    app.logExportPath = logPathField.text
                }
            }
        }

        Menu {
            title: "帮助"

            Action {
                text: "关于"
                onTriggered: aboutDialog.open()
            }
        }
    }

    Dialog {
        id: aboutDialog
        modal: true
        anchors.centerIn: parent
        title: "关于 gnu_jcom"
        standardButtons: Dialog.Ok

        contentItem: Label {
            text: "阶段 1 是终端模式优先的 Arch / Hyprland 串口工具。\n当前重点是串口会话、发送模板、日志、工作区保存。"
            color: textStrong
            wrapMode: Text.WordWrap
            padding: 16
        }
    }

    footer: Rectangle {
        implicitHeight: 24
        color: "#d1d5da"
        border.color: chromeBorder

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 10

            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: app.lastError.length > 0 ? bad : (app.portOpen ? good : warn)
            }

            Label {
                text: app.statusMessage
                color: textStrong
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Label.ElideRight
            }

            Label {
                text: app.selectedPort.length > 0 ? app.selectedPort : "No port"
                color: textMuted
                font.pixelSize: 11
            }

            Label {
                text: "Workspace " + basename(app.workspacePath)
                color: textMuted
                font.pixelSize: 11
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.bottomMargin: footer.height
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            gradient: Gradient {
                GradientStop { position: 0.0; color: toolbarStart }
                GradientStop { position: 1.0; color: toolbarEnd }
            }
            border.color: "#6d7c88"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 10

                Label {
                    text: "COM口"
                    color: "#ffffff"
                    font.pixelSize: 14
                    font.bold: true
                }

                ComboBox {
                    id: portCombo
                    Layout.preferredWidth: 150
                    model: app.availablePorts
                    enabled: model.length > 0
                    onActivated: app.selectedPort = currentText
                }

                Label {
                    text: "波特率"
                    color: "#ffffff"
                    font.pixelSize: 14
                    font.bold: true
                }

                ComboBox {
                    id: baudCombo
                    Layout.preferredWidth: 112
                    editable: true
                    model: baudRates
                    onAccepted: app.baudRate = Number(editText)
                    onActivated: app.baudRate = Number(currentText)
                }

                Button {
                    text: app.portOpen ? "关闭" : "打开"
                    Layout.preferredWidth: 90
                    highlighted: true
                    onClicked: {
                        if (app.portOpen) {
                            app.closePort()
                        } else {
                            app.openPort()
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: app.lastError.length > 0 ? "错误" : (app.portOpen ? "已连接" : "空闲")
                    color: app.lastError.length > 0 ? "#ffd7d3" : "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: chromeFill
            border.color: chromeBorder

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                Button {
                    text: "清空"
                    onClicked: app.clearLogs()
                }

                Button {
                    text: "保存数据"
                    onClicked: app.saveLog(logPathField.text)
                }

                Button {
                    text: "刷新串口"
                    onClicked: app.refreshPorts()
                }

                Item {
                    Layout.fillWidth: true
                }

                CheckBox {
                    text: "HEX显示"
                    checked: app.hexDisplay
                    onToggled: app.hexDisplay = checked
                }

                CheckBox {
                    text: "扩展"
                    checked: window.toolsExpanded
                    onToggled: window.toolsExpanded = checked
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            Layout.margins: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#dfe3e8"
                border.color: chromeBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 0
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: panelFill
                        border.color: panelBorder

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                Layout.preferredWidth: 36
                                Layout.fillHeight: true
                                color: "#e5e8ec"
                                border.color: panelBorder

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    spacing: 6

                                    Repeater {
                                        model: ["⇅", "◔", "≣", "?"]

                                        Rectangle {
                                            Layout.alignment: Qt.AlignHCenter
                                            width: 20
                                            height: 20
                                            radius: 2
                                            color: index === 0 ? "#73b67d"
                                                   : index === 1 ? "#7fc1e5"
                                                   : index === 2 ? "#7cb3d8"
                                                   : "#ea9b73"
                                            border.color: "#9fa8b2"

                                            Label {
                                                anchors.centerIn: parent
                                                text: modelData
                                                color: "#ffffff"
                                                font.pixelSize: 11
                                                font.bold: true
                                            }
                                        }
                                    }

                                    Item {
                                        Layout.fillHeight: true
                                    }

                                    Label {
                                        text: "接\n收\n区"
                                        color: textMuted
                                        font.pixelSize: 11
                                        horizontalAlignment: Text.AlignHCenter
                                        Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: terminalFill
                                border.color: panelBorder

                                ListView {
                                    id: logView
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    clip: true
                                    spacing: 1
                                    model: app.logModel

                                    delegate: Rectangle {
                                        required property int index
                                        required property string timestamp
                                        required property string kind
                                        required property string message
                                        required property string hex

                                        width: logView.width
                                        height: Math.max(24, lineText.implicitHeight + 8)
                                        color: index % 2 === 0 ? "#fbfbfc" : terminalGrid

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            Label {
                                                text: timestamp
                                                color: "#7d8b99"
                                                font.family: "Noto Sans Mono"
                                                font.pixelSize: 10
                                                Layout.preferredWidth: 74
                                            }

                                            Rectangle {
                                                width: 36
                                                height: 16
                                                radius: 2
                                                color: kind === "error" ? "#b34d45"
                                                       : kind === "warn" ? "#bc7724"
                                                       : kind === "tx" ? "#4583b3"
                                                       : kind === "rx" ? "#5b9d78"
                                                       : "#8693a1"

                                                Label {
                                                    anchors.centerIn: parent
                                                    text: kind.toUpperCase()
                                                    color: "#ffffff"
                                                    font.pixelSize: 8
                                                    font.bold: true
                                                }
                                            }

                                            Label {
                                                id: lineText
                                                text: (kind === "rx" || kind === "tx") && app.hexDisplay && hex.length > 0 ? hex : message
                                                color: terminalText
                                                wrapMode: Text.WrapAnywhere
                                                font.family: kind === "rx" || kind === "tx" ? "Noto Sans Mono" : "Noto Sans"
                                                font.pixelSize: 12
                                                Layout.fillWidth: true
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 126
                        color: panelFill
                        border.color: panelBorder

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                Layout.preferredWidth: 36
                                Layout.fillHeight: true
                                color: "#e5e8ec"
                                border.color: panelBorder

                                Label {
                                    anchors.centerIn: parent
                                    text: "发\n送\n区"
                                    color: textMuted
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#fbfbfc"
                                border.color: panelBorder

                                TextArea {
                                    id: composerArea
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    placeholderText: app.composeMode === 0 ? "输入待发送内容" : "AA 55 9B 1E B8"
                                    wrapMode: TextArea.Wrap
                                    selectByMouse: true
                                    font.family: app.composeMode === 0 ? "Noto Sans" : "Noto Sans Mono"
                                    font.pixelSize: 12
                                    onTextChanged: {
                                        if (periodicSendCheck.checked) {
                                            if (!app.startPeriodicSend(text, app.composeMode, periodicIntervalBox.value)) {
                                                periodicSendCheck.checked = false
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 188
                                Layout.fillHeight: true
                                color: "#eef1f4"
                                border.color: panelBorder

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 6

                                    Button {
                                        text: "发送"
                                        Layout.fillWidth: true
                                        highlighted: true
                                        onClicked: app.sendPayload(composerArea.text, app.composeMode)
                                    }

                                    Button {
                                        text: "清空"
                                        Layout.fillWidth: true
                                        onClicked: composerArea.text = ""
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        CheckBox {
                                            id: periodicSendCheck
                                            text: "周期发送"
                                            onToggled: {
                                                if (checked) {
                                                    if (!app.startPeriodicSend(composerArea.text, app.composeMode, periodicIntervalBox.value)) {
                                                        checked = false
                                                    }
                                                } else {
                                                    app.stopPeriodicSend()
                                                }
                                            }
                                        }

                                        SpinBox {
                                            id: periodicIntervalBox
                                            Layout.fillWidth: true
                                            from: 100
                                            to: 600000
                                            stepSize: 100
                                            value: app.periodicIntervalMs
                                            editable: true
                                            onValueModified: {
                                                app.periodicIntervalMs = value
                                                if (periodicSendCheck.checked) {
                                                    if (!app.startPeriodicSend(composerArea.text, app.composeMode, value)) {
                                                        periodicSendCheck.checked = false
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        CheckBox {
                                            id: hexSendCheck
                                            text: "HEX发送"
                                            checked: app.composeMode === 1
                                            onToggled: setComposerMode(checked)
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }

                                        CheckBox {
                                            text: "自动重连"
                                            checked: app.autoReconnect
                                            onToggled: app.autoReconnect = checked
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Label {
                                            text: "文本结尾"
                                            color: textMuted
                                            font.pixelSize: 11
                                        }

                                        ComboBox {
                                            id: lineEndingCombo
                                            Layout.fillWidth: true
                                            model: lineEndingOptions
                                            onActivated: {
                                                app.lineEnding = currentIndex
                                                if (periodicSendCheck.checked) {
                                                    if (!app.startPeriodicSend(composerArea.text, app.composeMode, periodicIntervalBox.value)) {
                                                        periodicSendCheck.checked = false
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Button {
                                            text: app.periodicActive ? "停止定时" : "启动定时"
                                            Layout.fillWidth: true
                                            onClicked: {
                                                if (app.periodicActive) {
                                                    app.stopPeriodicSend()
                                                    periodicSendCheck.checked = false
                                                } else {
                                                    const started = app.startPeriodicSend(composerArea.text, app.composeMode, periodicIntervalBox.value)
                                                    periodicSendCheck.checked = started
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                visible: window.toolsExpanded
                Layout.preferredWidth: 360
                Layout.minimumWidth: 320
                Layout.fillHeight: true
                color: "#e4e8ec"
                border.color: chromeBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    TabBar {
                        id: sideTabs
                        Layout.fillWidth: true

                        TabButton { text: "模板" }
                        TabButton { text: "工程" }
                        TabButton { text: "设置" }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: sideTabs.currentIndex

                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                TextField {
                                    id: templateNameField
                                    Layout.fillWidth: true
                                    placeholderText: "模板名称"
                                }

                                QtObject {
                                    id: templateModeGroup
                                    property int currentIndex: 0
                                }

                                RowLayout {
                                    spacing: 6

                                    Label {
                                        text: "模式"
                                        color: textMuted
                                        font.pixelSize: 11
                                    }

                                    ButtonGroup { id: templateModeButtons }

                                    Repeater {
                                        model: ["ASCII", "HEX"]

                                        Button {
                                            text: modelData
                                            checkable: true
                                            checked: templateModeGroup.currentIndex === index
                                            ButtonGroup.group: templateModeButtons
                                            onClicked: templateModeGroup.currentIndex = index
                                        }
                                    }
                                }

                                TextArea {
                                    id: templatePayloadField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 110
                                    placeholderText: "模板内容"
                                    wrapMode: TextArea.Wrap
                                    selectByMouse: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Button {
                                        text: "添加"
                                        Layout.fillWidth: true
                                        highlighted: true
                                        onClicked: {
                                            if (app.addSendItem(templateNameField.text, templatePayloadField.text, templateModeGroup.currentIndex)) {
                                                resetTemplateEditor()
                                            }
                                        }
                                    }

                                    Button {
                                        text: "更新"
                                        Layout.fillWidth: true
                                        enabled: sendListView.currentIndex >= 0
                                        onClicked: app.updateSendItem(sendListView.currentIndex, templateNameField.text, templatePayloadField.text, templateModeGroup.currentIndex)
                                    }

                                    Button {
                                        text: "删除"
                                        Layout.fillWidth: true
                                        enabled: sendListView.currentIndex >= 0
                                        onClicked: {
                                            app.removeSendItem(sendListView.currentIndex)
                                            resetTemplateEditor()
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "#fafbfc"
                                    border.color: panelBorder

                                    ListView {
                                        id: sendListView
                                        anchors.fill: parent
                                        anchors.margins: 1
                                        clip: true
                                        spacing: 1
                                        model: app.sendListModel

                                        delegate: Rectangle {
                                            required property int index
                                            required property string name
                                            required property string payload
                                            required property int mode

                                            width: sendListView.width
                                            height: 48
                                            color: sendListView.currentIndex === index ? "#dceaf5" : (index % 2 === 0 ? "#fbfbfc" : "#f0f3f6")
                                            border.color: sendListView.currentIndex === index ? accentDark : "#d0d6dd"

                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: {
                                                    sendListView.currentIndex = index
                                                    loadTemplateEditor(index)
                                                }
                                                onDoubleClicked: app.sendSavedItem(index)
                                            }

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 8
                                                anchors.rightMargin: 8
                                                spacing: 8

                                                Label {
                                                    text: mode === 0 ? "ASCII" : "HEX"
                                                    color: textMuted
                                                    font.pixelSize: 10
                                                    Layout.preferredWidth: 42
                                                }

                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 0

                                                    Label {
                                                        text: name
                                                        color: textStrong
                                                        font.pixelSize: 12
                                                        font.bold: true
                                                        Layout.fillWidth: true
                                                        elide: Label.ElideRight
                                                    }

                                                    Label {
                                                        text: payload
                                                        color: textMuted
                                                        font.pixelSize: 11
                                                        Layout.fillWidth: true
                                                        elide: Label.ElideRight
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                Label {
                                    text: "工作区路径"
                                    color: textMuted
                                    font.pixelSize: 11
                                }

                                TextField {
                                    id: workspaceField
                                    Layout.fillWidth: true
                                    text: app.workspacePath
                                    onEditingFinished: app.workspacePath = text
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Button {
                                        text: "保存工作区"
                                        Layout.fillWidth: true
                                        onClicked: app.saveWorkspace(workspaceField.text)
                                    }

                                    Button {
                                        text: "加载工作区"
                                        Layout.fillWidth: true
                                        onClicked: app.loadWorkspace(workspaceField.text)
                                    }
                                }

                                Label {
                                    text: "日志路径"
                                    color: textMuted
                                    font.pixelSize: 11
                                }

                                TextField {
                                    id: logPathField
                                    Layout.fillWidth: true
                                    text: app.logExportPath
                                    onEditingFinished: app.logExportPath = text
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Button {
                                        text: "保存日志"
                                        Layout.fillWidth: true
                                        onClicked: app.saveLog(logPathField.text)
                                    }

                                    Button {
                                        text: "默认路径"
                                        Layout.fillWidth: true
                                        onClicked: {
                                            workspaceField.text = app.defaultWorkspacePath
                                            app.workspacePath = workspaceField.text
                                            logPathField.text = app.defaultLogExportPath
                                            app.logExportPath = logPathField.text
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: "#cfd5dc"
                                }

                                Label {
                                    text: "提示：当前主界面按终端模式布局，发送模板和工作区收纳到扩展面板里。"
                                    color: textMuted
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }

                                Item {
                                    Layout.fillHeight: true
                                }
                            }
                        }

                        Item {
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                GridLayout {
                                    columns: 2
                                    Layout.fillWidth: true
                                    columnSpacing: 8
                                    rowSpacing: 8

                                    Label { text: "数据位"; color: textMuted; font.pixelSize: 11 }
                                    ComboBox {
                                        id: dataBitsCombo
                                        Layout.fillWidth: true
                                        model: dataBitsOptions
                                        onActivated: app.dataBits = Number(currentText)
                                    }

                                    Label { text: "停止位"; color: textMuted; font.pixelSize: 11 }
                                    ComboBox {
                                        id: stopBitsCombo
                                        Layout.fillWidth: true
                                        model: stopBitsOptions
                                        onActivated: app.stopBits = Number(currentText)
                                    }

                                    Label { text: "校验"; color: textMuted; font.pixelSize: 11 }
                                    ComboBox {
                                        id: parityCombo
                                        Layout.fillWidth: true
                                        model: parityOptions
                                        onActivated: app.parity = currentIndex
                                    }

                                    Label { text: "流控"; color: textMuted; font.pixelSize: 11 }
                                    ComboBox {
                                        id: flowCombo
                                        Layout.fillWidth: true
                                        model: flowOptions
                                        onActivated: app.flowControl = currentIndex
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: "#cfd5dc"
                                }

                                Label {
                                    text: "会话状态"
                                    color: textStrong
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                GridLayout {
                                    columns: 2
                                    Layout.fillWidth: true
                                    columnSpacing: 8
                                    rowSpacing: 6

                                    Label { text: "端口"; color: textMuted; font.pixelSize: 11 }
                                    Label { text: app.selectedPort.length > 0 ? app.selectedPort : "-"; color: textStrong; font.pixelSize: 11 }

                                    Label { text: "波特率"; color: textMuted; font.pixelSize: 11 }
                                    Label { text: String(app.baudRate); color: textStrong; font.pixelSize: 11 }

                                    Label { text: "模式"; color: textMuted; font.pixelSize: 11 }
                                    Label { text: app.composeMode === 0 ? "ASCII" : "HEX"; color: textStrong; font.pixelSize: 11 }

                                    Label { text: "自动重连"; color: textMuted; font.pixelSize: 11 }
                                    Label { text: app.autoReconnect ? "开启" : "关闭"; color: textStrong; font.pixelSize: 11 }

                                    Label { text: "定时发送"; color: textMuted; font.pixelSize: 11 }
                                    Label { text: app.periodicActive ? "运行中" : "已停止"; color: textStrong; font.pixelSize: 11 }
                                }

                                Rectangle {
                                    visible: app.lastError.length > 0
                                    Layout.fillWidth: true
                                    color: "#f7e4e1"
                                    border.color: "#ce9992"
                                    implicitHeight: errorText.implicitHeight + 16

                                    Label {
                                        id: errorText
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        text: app.lastError
                                        color: bad
                                        wrapMode: Text.WordWrap
                                        font.pixelSize: 11
                                    }
                                }

                                Item {
                                    Layout.fillHeight: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: app

        function onAvailablePortsChanged() {
            if (app.availablePorts.length > 0) {
                const idx = app.availablePorts.indexOf(app.selectedPort)
                portCombo.currentIndex = idx >= 0 ? idx : 0
            } else {
                portCombo.currentIndex = -1
            }
        }

        function onSelectedPortChanged() {
            if (app.availablePorts.length > 0) {
                const idx = app.availablePorts.indexOf(app.selectedPort)
                portCombo.currentIndex = idx >= 0 ? idx : 0
            }
        }

        function onBaudRateChanged() {
            syncCombo(baudCombo, baudRates, app.baudRate)
            baudCombo.editText = String(app.baudRate)
        }

        function onDataBitsChanged() {
            syncCombo(dataBitsCombo, dataBitsOptions, app.dataBits)
        }

        function onStopBitsChanged() {
            syncCombo(stopBitsCombo, stopBitsOptions, app.stopBits)
        }

        function onParityChanged() {
            parityCombo.currentIndex = app.parity
        }

        function onFlowControlChanged() {
            flowCombo.currentIndex = app.flowControl
        }

        function onLineEndingChanged() {
            lineEndingCombo.currentIndex = app.lineEnding
        }

        function onPeriodicIntervalMsChanged() {
            periodicIntervalBox.value = app.periodicIntervalMs
        }

        function onPeriodicActiveChanged() {
            periodicSendCheck.checked = app.periodicActive
        }

        function onComposeModeChanged() {
            hexSendCheck.checked = app.composeMode === 1
        }

        function onWorkspacePathChanged() {
            workspaceField.text = app.workspacePath
        }

        function onLogExportPathChanged() {
            logPathField.text = app.logExportPath
        }
    }

    Connections {
        target: app.logModel

        function onCountChanged() {
            logView.positionViewAtEnd()
        }
    }

    Component.onCompleted: {
        if (app.availablePorts.length > 0) {
            const idx = app.availablePorts.indexOf(app.selectedPort)
            portCombo.currentIndex = idx >= 0 ? idx : 0
        }
        syncCombo(baudCombo, baudRates, app.baudRate)
        baudCombo.editText = String(app.baudRate)
        syncCombo(dataBitsCombo, dataBitsOptions, app.dataBits)
        syncCombo(stopBitsCombo, stopBitsOptions, app.stopBits)
        parityCombo.currentIndex = app.parity
        flowCombo.currentIndex = app.flowControl
        lineEndingCombo.currentIndex = app.lineEnding
        periodicIntervalBox.value = app.periodicIntervalMs
    }
}
