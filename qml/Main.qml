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
    property var packetTypeOptions: ["uint", "int", "float"]
    property var packetByteWidthOptions: [1, 2, 4, 8]
    property var packetEndianOptions: ["little", "big"]
    property var packetChecksumOptions: [
        { text: "SUM-8", value: "sum8" },
        { text: "CRC-8", value: "crc8" },
        { text: "CRC-8/ITU", value: "crc8_itu" },
        { text: "CRC-8/ROHC", value: "crc8_rohc" },
        { text: "CRC-8/MAXIM", value: "crc8_maxim" },
        { text: "CRC-16/IBM", value: "crc16_ibm" },
        { text: "CRC-16/MAXIM", value: "crc16_maxim" },
        { text: "CRC-16/USB", value: "crc16_usb" },
        { text: "CRC-16/MODBUS", value: "crc16_modbus" },
        { text: "CRC-16/CCITT", value: "crc16_ccitt" },
        { text: "CRC-16/CCITT-FALSE", value: "crc16_ccitt_false" },
        { text: "CRC-16/X25", value: "crc16_x25" },
        { text: "CRC-16/XMODEM", value: "crc16_xmodem" },
        { text: "CRC-16/DNP", value: "crc16_dnp" },
        { text: "CRC-32", value: "crc32" },
        { text: "CRC-32/MPEG-2", value: "crc32_mpeg2" },
        { text: "无校验", value: "none" }
    ]
    property bool toolsExpanded: false
    property bool packetPathEditorVisible: false
    property string packetPreviewText: ""

    function packetAllowedByteWidths(typeName) {
        const normalized = typeName ? typeName.toLowerCase() : ""
        if (normalized === "float") {
            return [4, 8]
        }
        return packetByteWidthOptions
    }

    function packetChecksumOptionIndex(value) {
        for (let i = 0; i < packetChecksumOptions.length; ++i) {
            if (packetChecksumOptions[i].value === value) {
                return i
            }
        }
        return 0
    }

    function packetPreviewByteCount(text) {
        const normalized = text ? text.trim() : ""
        if (normalized.length === 0) {
            return 0
        }
        return normalized.split(/\s+/).length
    }

    function appendPacketFieldPreset(baseName, byteWidth, defaultValue, typeName = "uint") {
        if (!app) {
            return
        }

        const nextIndex = app.packetFieldModel.count + 1
        const fieldName = baseName + "_" + nextIndex
        if (!app.packetFieldModel.appendField(fieldName)) {
            return
        }

        const modelIndex = app.packetFieldModel.count - 1
        app.packetFieldModel.setFieldType(modelIndex, typeName)
        app.packetFieldModel.setFieldByteWidth(modelIndex, byteWidth)
        app.packetFieldModel.setSendValue(modelIndex, String(defaultValue))

        if (packetFieldView) {
            packetFieldView.currentIndex = modelIndex
            packetFieldView.positionViewAtEnd()
        }
    }

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

    function refreshPacketPreview() {
        if (!app) {
            packetPreviewText = ""
            return
        }

        packetPreviewText = app.buildPacketPreview()
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
                    packetSchemaPathField.text = app.packetSchemaPath
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
            text: "阶段 1 保持终端模式优先。\n当前已经补到基础组包、自定义接收解析和虚拟串口联调。"
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
                Layout.preferredWidth: sideTabs.currentIndex === 1 ? 680 : 360
                Layout.minimumWidth: sideTabs.currentIndex === 1 ? 620 : 320
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
                        TabButton { text: "组包" }
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

                                Button {
                                    text: "发送选中模板"
                                    Layout.fillWidth: true
                                    enabled: sendListView.currentIndex >= 0
                                    highlighted: true
                                    onClicked: app.sendSavedItem(sendListView.currentIndex)
                                }

                                Label {
                                    text: "提示：双击下面的模板列表，也会直接发送。"
                                    color: textMuted
                                    font.pixelSize: 10
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
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

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    color: "#d8e0e8"
                                    border.color: panelBorder

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 8

                                        Label {
                                            text: "发送数据组包"
                                            color: textStrong
                                            font.pixelSize: 12
                                            font.bold: true
                                        }

                                        Rectangle {
                                            implicitWidth: packetSchemaLoadedLabel.implicitWidth + 14
                                            implicitHeight: 18
                                            radius: 2
                                            color: app.packetSchemaLoaded ? "#dff1e7" : "#f4e8cc"
                                            border.color: app.packetSchemaLoaded ? "#a7c9b5" : "#d8c38d"

                                            Label {
                                                id: packetSchemaLoadedLabel
                                                anchors.centerIn: parent
                                                text: app.packetSchemaLoaded ? "解析已应用" : "解析未应用"
                                                color: app.packetSchemaLoaded ? good : warn
                                                font.pixelSize: 10
                                                font.bold: true
                                            }
                                        }

                                        Item { Layout.fillWidth: true }

                                        Label {
                                            text: "字段 " + app.packetFieldModel.count + " 项"
                                            color: textMuted
                                            font.pixelSize: 11
                                        }

                                        Label {
                                            text: "已解析 " + app.parsedFrameCount + " 帧"
                                            color: textMuted
                                            font.pixelSize: 11
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 42
                                    color: "#eef2f6"
                                    border.color: panelBorder

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 6

                                        Label {
                                            text: "插入字段"
                                            color: textMuted
                                            font.pixelSize: 11
                                        }

                                        ToolButton {
                                            text: "帧头"
                                            onClicked: {
                                                if (packetHeaderField.text.length === 0) {
                                                    packetHeaderField.text = "AA 55"
                                                    app.packetHeaderText = packetHeaderField.text
                                                }
                                                packetHeaderField.forceActiveFocus()
                                                packetHeaderField.selectAll()
                                            }
                                        }

                                        ToolButton {
                                            text: "帧尾"
                                            onClicked: {
                                                if (packetFooterField.text.length === 0) {
                                                    packetFooterField.text = "0D 0A"
                                                    app.packetFooterText = packetFooterField.text
                                                }
                                                packetFooterField.forceActiveFocus()
                                                packetFooterField.selectAll()
                                            }
                                        }

                                        ToolButton {
                                            text: "帧ID"
                                            onClicked: appendPacketFieldPreset("frame_id", 2, 1, "uint")
                                        }

                                        ToolButton {
                                            text: "1Byte"
                                            onClicked: appendPacketFieldPreset("data1", 1, 0, "uint")
                                        }

                                        ToolButton {
                                            text: "2Byte"
                                            onClicked: appendPacketFieldPreset("data2", 2, 0, "uint")
                                        }

                                        ToolButton {
                                            text: "4Byte"
                                            onClicked: appendPacketFieldPreset("data4", 4, 0, "uint")
                                        }

                                        ToolButton {
                                            text: "8Byte"
                                            onClicked: appendPacketFieldPreset("data8", 8, 0, "uint")
                                        }

                                        Item { Layout.fillWidth: true }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    color: "#f6f8fa"
                                    border.color: panelBorder
                                    implicitHeight: packetMetaLayout.implicitHeight + 18

                                    ColumnLayout {
                                        id: packetMetaLayout
                                        anchors.fill: parent
                                        anchors.margins: 9
                                        spacing: 8

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 6

                                            Label {
                                                text: "协议名"
                                                color: textMuted
                                                font.pixelSize: 11
                                            }

                                            TextField {
                                                id: packetNameField
                                                Layout.fillWidth: true
                                                text: app.packetSchemaName
                                                placeholderText: "packet_demo"
                                                onTextEdited: app.packetSchemaName = text
                                            }

                                            Button {
                                                text: "应用解析"
                                                highlighted: true
                                                onClicked: app.applyPacketDefinition()
                                            }

                                            Button {
                                                text: "发送组包"
                                                onClicked: app.sendPacket()
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 6

                                            Label {
                                                text: "当前JSON"
                                                color: textMuted
                                                font.pixelSize: 11
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: basename(app.packetSchemaPath)
                                                color: textStrong
                                                font.pixelSize: 11
                                                font.bold: true
                                                elide: Label.ElideMiddle
                                                verticalAlignment: Text.AlignVCenter
                                            }

                                            Button {
                                                text: packetPathEditorVisible ? "收起路径" : "路径设置"
                                                onClicked: packetPathEditorVisible = !packetPathEditorVisible
                                            }

                                            Button {
                                                text: "导入JSON"
                                                onClicked: app.loadPacketSchema(packetSchemaPathField.text)
                                            }

                                            Button {
                                                text: "导出JSON"
                                                onClicked: app.savePacketSchema(packetSchemaPathField.text)
                                            }
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: app.packetSchemaPath
                                            color: textMuted
                                            font.pixelSize: 10
                                            elide: Label.ElideMiddle
                                        }

                                        RowLayout {
                                            visible: packetPathEditorVisible
                                            Layout.fillWidth: true
                                            spacing: 6

                                            Label {
                                                text: "路径"
                                                color: textMuted
                                                font.pixelSize: 11
                                            }

                                            TextField {
                                                id: packetSchemaPathField
                                                Layout.fillWidth: true
                                                text: app.packetSchemaPath
                                                placeholderText: "examples/linear_demo_schema.json"
                                                onEditingFinished: app.packetSchemaPath = text
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "#fafbfc"
                                    border.color: panelBorder

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 0
                                        spacing: 0

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 40
                                            color: "#edf2f7"
                                            border.color: panelBorder

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 8
                                                anchors.rightMargin: 8
                                                spacing: 8

                                                Label {
                                                    text: "帧头"
                                                    color: textStrong
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                    Layout.preferredWidth: 38
                                                }

                                                TextField {
                                                    id: packetHeaderField
                                                    Layout.fillWidth: true
                                                    text: app.packetHeaderText
                                                    placeholderText: "AA 55"
                                                    selectByMouse: true
                                                    font.family: "Noto Sans Mono"
                                                    inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                                                    onTextEdited: app.packetHeaderText = text
                                                }

                                                Label {
                                                    text: "校验"
                                                    color: textMuted
                                                    font.pixelSize: 11
                                                }

                                                ComboBox {
                                                    id: packetChecksumCombo
                                                    Layout.preferredWidth: 188
                                                    model: packetChecksumOptions
                                                    textRole: "text"
                                                    onActivated: app.packetChecksum = packetChecksumOptions[currentIndex].value
                                                }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 30
                                            color: "#e2e8ee"
                                            border.color: panelBorder

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 8
                                                anchors.rightMargin: 8
                                                spacing: 6

                                                Label { text: "#"; color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 28 }
                                                Label { text: "字段名称"; color: textMuted; font.pixelSize: 10; Layout.fillWidth: true }
                                                Label { text: "类型"; color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 72 }
                                                Label { text: "字节"; color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 52 }
                                                Label { text: "发送值"; color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 84 }
                                                Label { text: "字节序"; color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 72 }
                                                Label { text: "接收值"; color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 88 }
                                                Label { text: "操作"; color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 72 }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            color: "#fafbfc"
                                            border.color: panelBorder

                                            ListView {
                                                id: packetFieldView
                                                anchors.fill: parent
                                                anchors.margins: 1
                                                clip: true
                                                spacing: 1
                                                model: app.packetFieldModel
                                                ScrollBar.vertical: ScrollBar {}

                                                delegate: Rectangle {
                                                    required property int index
                                                    required property string name
                                                    required property string typeName
                                                    required property int byteWidth
                                                    required property string endian
                                                    required property string sendValue
                                                    required property string receivedValue
                                                    readonly property int rowIndex: index

                                                    width: packetFieldView.width
                                                    height: 44
                                                    color: rowIndex % 2 === 0 ? "#fbfbfc" : "#f1f4f7"

                                                    RowLayout {
                                                        anchors.fill: parent
                                                        anchors.leftMargin: 8
                                                        anchors.rightMargin: 8
                                                        spacing: 6

                                                        Label {
                                                            text: String(rowIndex + 1)
                                                            color: textMuted
                                                            font.pixelSize: 11
                                                            Layout.preferredWidth: 28
                                                        }

                                                        TextField {
                                                            Layout.fillWidth: true
                                                            Layout.minimumWidth: 90
                                                            text: name
                                                            placeholderText: "字段名"
                                                            onTextEdited: app.packetFieldModel.setFieldName(rowIndex, text)
                                                        }

                                                        ComboBox {
                                                            Layout.preferredWidth: 72
                                                            model: packetTypeOptions
                                                            currentIndex: {
                                                                const idx = packetTypeOptions.indexOf(typeName)
                                                                return idx >= 0 ? idx : 0
                                                            }
                                                            onActivated: function() {
                                                                app.packetFieldModel.setFieldType(rowIndex, currentText)
                                                            }
                                                        }

                                                        ComboBox {
                                                            Layout.preferredWidth: 52
                                                            model: packetAllowedByteWidths(typeName)
                                                            currentIndex: {
                                                                const widths = packetAllowedByteWidths(typeName)
                                                                const idx = widths.indexOf(byteWidth)
                                                                return idx >= 0 ? idx : 0
                                                            }
                                                            onActivated: function() {
                                                                const widths = packetAllowedByteWidths(typeName)
                                                                app.packetFieldModel.setFieldByteWidth(rowIndex, widths[currentIndex])
                                                            }
                                                        }

                                                        TextField {
                                                            Layout.preferredWidth: 84
                                                            text: sendValue
                                                            selectByMouse: true
                                                            horizontalAlignment: Text.AlignHCenter
                                                            font.family: "Noto Sans Mono"
                                                            onTextEdited: app.packetFieldModel.setSendValue(rowIndex, text)
                                                        }

                                                        ComboBox {
                                                            Layout.preferredWidth: 72
                                                            model: packetEndianOptions
                                                            currentIndex: endian === "big" ? 1 : 0
                                                            enabled: byteWidth > 1
                                                            opacity: enabled ? 1.0 : 0.45
                                                            onActivated: function() {
                                                                app.packetFieldModel.setFieldEndian(rowIndex, currentText)
                                                            }
                                                        }

                                                        Label {
                                                            Layout.preferredWidth: 88
                                                            text: receivedValue
                                                            color: textStrong
                                                            font.pixelSize: 11
                                                            elide: Label.ElideRight
                                                        }

                                                        RowLayout {
                                                            Layout.preferredWidth: 72
                                                            spacing: 2

                                                            ToolButton {
                                                                text: "↑"
                                                                onClicked: app.packetFieldModel.moveFieldUp(rowIndex)
                                                            }

                                                            ToolButton {
                                                                text: "↓"
                                                                onClicked: app.packetFieldModel.moveFieldDown(rowIndex)
                                                            }

                                                            ToolButton {
                                                                text: "删"
                                                                onClicked: app.packetFieldModel.removeField(rowIndex)
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 40
                                            color: "#edf2f7"
                                            border.color: panelBorder

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 8
                                                anchors.rightMargin: 8
                                                spacing: 8

                                                Label {
                                                    text: "帧尾"
                                                    color: textStrong
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                    Layout.preferredWidth: 38
                                                }

                                                TextField {
                                                    id: packetFooterField
                                                    Layout.fillWidth: true
                                                    text: app.packetFooterText
                                                    placeholderText: "可留空"
                                                    selectByMouse: true
                                                    font.family: "Noto Sans Mono"
                                                    inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                                                    onTextEdited: app.packetFooterText = text
                                                }

                                                Label {
                                                    text: "留空则不参与帧尾匹配"
                                                    color: textMuted
                                                    font.pixelSize: 11
                                                }
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 74
                                    color: "#f7f9fb"
                                    border.color: panelBorder

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 4

                                        RowLayout {
                                            Layout.fillWidth: true

                                            Label {
                                                text: "组包预览"
                                                color: textStrong
                                                font.pixelSize: 12
                                                font.bold: true
                                            }

                                            Item { Layout.fillWidth: true }

                                            Label {
                                                text: packetPreviewText.length > 0
                                                      ? "长度 " + packetPreviewByteCount(packetPreviewText) + " Byte"
                                                      : "长度 -"
                                                color: textMuted
                                                font.pixelSize: 11
                                            }
                                        }

                                        Text {
                                            id: packetPreviewBody
                                            Layout.fillWidth: true
                                            text: packetPreviewText.length > 0 ? packetPreviewText : "当前定义无效，无法生成预览"
                                            color: packetPreviewText.length > 0 ? terminalText : textMuted
                                            wrapMode: Text.WrapAnywhere
                                            font.family: "Noto Sans Mono"
                                            font.pixelSize: 12
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                Label {
                                    text: "说明：帧头/帧尾填 HEX；帧ID 和数据字段都可在“类型”列切成 1/2/4/8 字节；改完点“应用解析”后，接收区按当前定义解析。"
                                    color: textMuted
                                    font.pixelSize: 11
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
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
                                            packetSchemaPathField.text = app.packetSchemaPath
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: "#cfd5dc"
                                }

                                Label {
                                    text: "提示：当前主界面按终端模式布局，模板、组包和工作区都收纳在扩展面板里。"
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

        function onPacketSchemaPathChanged() {
            packetSchemaPathField.text = app.packetSchemaPath
        }

        function onPacketSchemaNameChanged() {
            packetNameField.text = app.packetSchemaName
            refreshPacketPreview()
        }

        function onPacketHeaderTextChanged() {
            packetHeaderField.text = app.packetHeaderText
            refreshPacketPreview()
        }

        function onPacketFooterTextChanged() {
            packetFooterField.text = app.packetFooterText
            refreshPacketPreview()
        }

        function onPacketChecksumChanged() {
            packetChecksumCombo.currentIndex = packetChecksumOptionIndex(app.packetChecksum)
            refreshPacketPreview()
        }

        function onPacketSchemaLoadedChanged() {
            refreshPacketPreview()
        }
    }

    Connections {
        target: app.logModel

        function onCountChanged() {
            logView.positionViewAtEnd()
        }
    }

    Connections {
        target: app.packetFieldModel

        function onCountChanged() {
            refreshPacketPreview()
        }

        function onDataChanged() {
            refreshPacketPreview()
        }

        function onModelReset() {
            refreshPacketPreview()
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
        packetChecksumCombo.currentIndex = packetChecksumOptionIndex(app.packetChecksum)
        refreshPacketPreview()
    }
}
