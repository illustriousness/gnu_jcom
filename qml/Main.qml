import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: root
    width: 1000
    height: 640
    minimumWidth: 780
    minimumHeight: 520
    visible: true
    title: "GNU JCOM"
    color: vscBg

    // ── 主题配色（深色高对比）─────────────────────────────────────────────────
    readonly property color vscBg:        "#161b22"
    readonly property color vscSidebar:   "#1c2128"
    readonly property color vscTabBar:    "#0d1117"
    readonly property color vscTabActive: "#161b22"
    readonly property color vscTabHover:  "#21262d"
    readonly property color vscInput:     "#21262d"
    readonly property color vscBorder:    "#30363d"
    readonly property color vscCardBorder: "#4a5568"
    readonly property color vscHover:     "#1f2937"
    readonly property color vscSelected:  "#2d333b"
    readonly property color vscAccent:    "#58a6ff"
    readonly property color vscGreen:     "#3fb950"
    readonly property color vscRed:       "#f85149"
    readonly property color vscOrange:    "#d29922"
    readonly property color vscPurple:    "#bc8cff"
    readonly property color vscTextPri:   "#f0f6fc"
    readonly property color vscTextSec:   "#8b949e"
    readonly property color vscTextDim:   "#484f58"

    // ── 状态 ───────────────────────────────────────────────────────────────────
    property int  activeTab:       0
    property bool sidebarVisible:  true
    readonly property bool portConnected: serial.portOpen
    property bool cmdSectionOpen:  true
    property int  selectedCmdItem: -1
    property bool timerRunning:    false
    property int _prevKeyEvent: 0
    readonly property var baudRates: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]

    function portIndex(name) {
        for (var i = 0; i < serial.availablePorts.length; i++) {
            if (serial.availablePorts[i] === name)
                return i
        }
        return -1
    }

    function baudIndex(rate) {
        for (var i = 0; i < root.baudRates.length; i++) {
            if (root.baudRates[i] === rate)
                return i
        }
        return 4
    }

    function parityText(parity) {
        return parity === 1 ? "Even" :
               parity === 2 ? "Odd" :
               parity === 3 ? "Space" :
               parity === 4 ? "Mark" : "None"
    }

    function parityCode(parity) {
        return parity === 1 ? "E" :
               parity === 2 ? "O" :
               parity === 3 ? "S" :
               parity === 4 ? "M" : "N"
    }

    function flowText(flowControl) {
        return flowControl === 1 ? "RTS/CTS" :
               flowControl === 2 ? "XON/XOFF" : "None"
    }

    function frameFormatText() {
        return serial.dataBits + root.parityCode(serial.parity) + serial.stopBits
    }

    function parseIntegerField(text, fallbackValue) {
        var match = String(text).match(/-?\d+/)
        return match === null ? fallbackValue : parseInt(match[0])
    }

    Component.onCompleted: serial.refreshPorts()

    // keyEventFlags 上升沿检测 → 右下角通知（状态位不需要，只有事件位才检测跳变）
    Connections {
        target: report
        function onUpdated() {
            var keNames = ["POWER_OFF","DOCK","ESTOP","START","UNLOCK","NETCFG"]
            var ke = report.keyEventFlags
            var keDiff = ke ^ root._prevKeyEvent
            if (keDiff !== 0) {
                for (var j = 0; j < keNames.length; j++) {
                    var kbit = 1 << j
                    if ((keDiff & kbit) && (ke & kbit)) {   // 上升沿
                        notifModel.append({
                            msg:       "⌨  KEY: " + keNames[j],
                            kind:      "key",
                            createdAt: Date.now()
                        })
                    }
                }
                root._prevKeyEvent = ke
            }
        }
    }

    ListModel { id: notifModel }

    // 通知自动消退（4 秒）
    Timer {
        interval: 800; running: true; repeat: true
        onTriggered: {
            var now = Date.now()
            for (var i = notifModel.count - 1; i >= 0; i--)
                if (now - notifModel.get(i).createdAt > 4000)
                    notifModel.remove(i)
        }
    }

    // ── 右下角通知覆盖层 ────────────────────────────────────────────────────────
    Column {
        anchors { right: parent.right; bottom: parent.bottom; rightMargin: 14; bottomMargin: 14 }
        spacing: 6
        z: 200

        Repeater {
            model: notifModel
            delegate: Rectangle {
                required property string msg
                required property string kind
                required property var    createdAt
                required property int    index

                width: notifText.width + 42; height: 36; radius: 6
                color: kind === "fault"  ? Qt.rgba(0.97, 0.32, 0.29, 0.18) :
                       kind === "clear"  ? Qt.rgba(0.24, 0.85, 0.44, 0.15) :
                                           Qt.rgba(0.35, 0.65, 1.00, 0.15)
                border.color: kind === "fault"  ? vscRed   :
                              kind === "clear"  ? vscGreen :
                                                  vscAccent
                border.width: 1

                Rectangle {
                    anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
                    width: 3; radius: 1
                    color: kind === "fault"  ? vscRed   :
                           kind === "clear"  ? vscGreen :
                                               vscAccent
                }

                Row {
                    anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                    spacing: 8
                    Text {
                        id: notifText
                        text: msg
                        font.pixelSize: 12; font.family: "monospace"; font.bold: true
                        color: kind === "fault"  ? vscRed   :
                               kind === "clear"  ? vscGreen :
                                                   vscAccent
                    }
                }

                // 右侧关闭按钮
                Text {
                    anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                    text: "✕"; font.pixelSize: 9; color: vscTextDim
                    MouseArea { anchors.fill: parent; onClicked: notifModel.remove(index) }
                }

                opacity: 0
                NumberAnimation on opacity { to: 1; duration: 200 }
            }
        }
    }

    // ── 顶部导航栏 ─────────────────────────────────────────────────────────────
    Rectangle {
        id: topNav
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 44
        color: vscTabBar

        // 底部分隔线
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 1; color: vscBorder
        }

        // 左侧：品牌 + 折叠 + 标签
        Row {
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }

            // 品牌标识
            Rectangle {
                width: 52; height: parent.height
                color: "transparent"
                Row {
                    anchors.centerIn: parent; spacing: 4
                    Rectangle {
                        width: 6; height: 22; radius: 1
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: vscAccent }
                            GradientStop { position: 1.0; color: vscPurple }
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "JC"; font.pixelSize: 13; font.bold: true
                        color: vscTextPri; anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // 折叠按钮
            Rectangle {
                width: 36; height: parent.height
                color: sideToggleMA.containsMouse ? vscTabHover : "transparent"
                Behavior on color { ColorAnimation { duration: 80 } }
                Text {
                    anchors.centerIn: parent; text: root.sidebarVisible ? "⇤" : "⇥"
                    font.pixelSize: 15; color: sideToggleMA.containsMouse ? vscTextPri : vscTextSec
                    Behavior on color { ColorAnimation { duration: 80 } }
                }
                MouseArea {
                    id: sideToggleMA; anchors.fill: parent; hoverEnabled: true
                    onClicked: root.sidebarVisible = !root.sidebarVisible
                }
            }

            Rectangle { width: 1; height: 24; color: vscBorder; anchors.verticalCenter: parent.verticalCenter }

            NavTab { icon: "◈"; label: "协议解析"; active: root.activeTab === 0; accent: vscAccent;  onTabClicked: root.activeTab = 0 }
            NavTab { icon: "⇅"; label: "文件传输"; active: root.activeTab === 1; accent: vscGreen;   onTabClicked: root.activeTab = 1 }
            NavTab { icon: "❯"; label: "终端模式"; active: root.activeTab === 2; accent: vscOrange;  onTabClicked: root.activeTab = 2 }
            NavTab { icon: "⚙"; label: "串口设置"; active: root.activeTab === 3; accent: vscPurple;  onTabClicked: root.activeTab = 3 }
        }

        // 右侧：端口状态 pill
        Row {
            anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
            spacing: 8

            // 端口状态 pill
            Rectangle {
                id: portChip
                height: 26; radius: 13; width: portChipRow.width + 20
                color: root.portConnected ? Qt.rgba(0.24, 0.73, 0.31, 0.15) : Qt.rgba(1,1,1,0.05)
                border.color: root.portConnected ? vscGreen : vscBorder; border.width: 1
                anchors.verticalCenter: parent.verticalCenter
                Behavior on color { ColorAnimation { duration: 200 } }

                Row {
                    id: portChipRow
                    anchors.centerIn: parent; spacing: 6

                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.portConnected ? vscGreen : vscTextDim
                        Behavior on color { ColorAnimation { duration: 300 } }
                        // 连接时的脉冲动画
                        SequentialAnimation on opacity {
                            running: root.portConnected; loops: Animation.Infinite
                            NumberAnimation { to: 0.4; duration: 800 }
                            NumberAnimation { to: 1.0; duration: 800 }
                        }
                    }
                    Text {
                        text: root.portConnected ? (serial.selectedPort + "  " + serial.baudRate) : "未连接"
                        font.pixelSize: 11; font.bold: root.portConnected
                        color: root.portConnected ? vscGreen : vscTextSec
                        anchors.verticalCenter: parent.verticalCenter
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.activeTab = 3
                    onEntered: portChip.border.width = 2
                    onExited:  portChip.border.width = 1
                }
            }
        }
    }

    // ── Body ───────────────────────────────────────────────────────────────────
    Item {
        id: body
        anchors { top: topNav.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }

        // ── 左侧栏（可折叠）───────────────────────────────────────────────────
        Rectangle {
            id: sidebar
            anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
            width: root.sidebarVisible ? 220 : 0
            clip: true
            color: vscSidebar
            Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

            Rectangle {
                anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
                width: 1; color: vscBorder
            }

            // 协议解析 - 左侧栏内容
            Item {
                anchors.fill: parent
                visible: root.activeTab === 0
                clip: true

                Flickable {
                    anchors.fill: parent
                    contentHeight: protoSideCol.height
                    clip: true

                    Column {
                        id: protoSideCol
                        width: 220

                        // ── 定时下发（固定，不可折叠）────────────────────────
                        SideSection { label: "定时下发"; open: true; canCollapse: false; width: parent.width }

                        TimerItem {
                            label: "VW 控制"
                            vValue: serial.controlV
                            wValue: serial.controlW
                            periodMs: serial.controlPeriodMs
                            running: serial.timedVwRunning
                            width: protoSideCol.width
                            onVCommitted: function(value) { serial.controlV = value }
                            onWCommitted: function(value) { serial.controlW = value }
                            onPeriodCommitted: function(value) { serial.controlPeriodMs = value }
                            onStartRequested: function() { serial.startTimedVwControl() }
                            onStopRequested: function() { serial.stopTimedVwControl() }
                        }

                        SideDivider { width: parent.width }

                        // ── 单次下发（可折叠）────────────────────────────────
                        SideSection {
                            label: "单次下发"
                            open: root.cmdSectionOpen
                            width: parent.width
                            onToggled: root.cmdSectionOpen = !root.cmdSectionOpen
                        }

                        Column {
                            width: parent.width
                            visible: root.cmdSectionOpen
                            clip: true

                            Repeater {
                                model: ["set_time_sync", "set_config", "set_mode", "reset_device"]
                                delegate: CmdItem {
                                    required property string modelData
                                    required property int    index
                                    label: modelData
                                    sendEnabled: modelData === "set_time_sync"
                                    expanded: root.selectedCmdItem === index
                                    width: protoSideCol.width
                                    onItemClicked: function() {
                                        root.selectedCmdItem = (root.selectedCmdItem === index ? -1 : index)
                                    }
                                    onSendRequested: function() {
                                        if (modelData === "set_time_sync")
                                            serial.sendCurrentTimestamp()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 其他 tab 的左侧栏（暂空）
            Item {
                anchors.fill: parent
                visible: root.activeTab !== 0
            }
        }

        // ── 主界面（跟随左侧栏自动展开）───────────────────────────────────────
        Item {
            anchors { top: parent.top; bottom: parent.bottom; left: sidebar.right; right: parent.right }

            // ── 协议解析主界面 ─────────────────────────────────────────────────
            Item {
                anchors.fill: parent
                visible: root.activeTab === 0

                // 接收数据区
                Rectangle {
                    anchors { top: parent.top; bottom: parent.bottom; left: parent.left; right: parent.right }
                    color: vscBg

                    // 结构化数据展示（gnu_soc_proto_mcu_report_t）
                    Flickable {
                        anchors.fill: parent
                        contentWidth: width
                        contentHeight: dashCol.implicitHeight + 20
                        clip: true
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        Column {
                            id: dashCol
                            width: parent.width - 24
                            x: 12; y: 12
                            spacing: 10

                            // ── 第一行：运动 / 电池 / 统计 ───────────────────────
                            Row {
                                width: parent.width; spacing: 10

                                DashCard {
                                    cw: (parent.width - 20) / 3
                                    accent: vscAccent
                                    title: "运动"
                                    Column {
                                        width: parent.width; spacing: 10
                                        Column {
                                            width: parent.width; spacing: 4
                                            Row {
                                                width: parent.width
                                                Text { text: "v_real"; font.pixelSize: 11; color: vscTextSec; width: parent.width - vIcon.width }
                                                Text { id: vIcon; text: report.vReal >= 0 ? "↑" : "↓"; font.pixelSize: 13; color: report.vReal >= 0 ? vscGreen : "#e8a55a"; Behavior on color { ColorAnimation { duration: 100 } } }
                                            }
                                            Row {
                                                spacing: 4
                                                Text { text: report.vReal; font.pixelSize: 22; font.bold: true; font.family: "monospace"; color: vscTextPri }
                                                Text { text: "mm/s"; font.pixelSize: 10; color: vscTextDim; anchors.bottom: parent.bottom; anchors.bottomMargin: 3 }
                                            }
                                        }
                                        Rectangle { width: parent.width; height: 1; color: vscBorder; opacity: 0.4 }
                                        Column {
                                            width: parent.width; spacing: 4
                                            Row {
                                                width: parent.width
                                                Text { text: "w_real"; font.pixelSize: 11; color: vscTextSec; width: parent.width - wIcon.width }
                                                Text { id: wIcon; text: report.wReal >= 0 ? "↻" : "↺"; font.pixelSize: 13; color: report.wReal >= 0 ? vscGreen : "#e8a55a"; Behavior on color { ColorAnimation { duration: 100 } } }
                                            }
                                            Row {
                                                spacing: 4
                                                Text { text: report.wReal; font.pixelSize: 22; font.bold: true; font.family: "monospace"; color: vscTextPri }
                                                Text { text: "mrad/s"; font.pixelSize: 10; color: vscTextDim; anchors.bottom: parent.bottom; anchors.bottomMargin: 3 }
                                            }
                                        }
                                    }
                                }

                                DashCard {
                                    cw: (parent.width - 20) / 3
                                    accent: vscGreen
                                    title: "电池"
                                    Column {
                                        width: parent.width; spacing: 10
                                        Column {
                                            width: parent.width; spacing: 6
                                            Row {
                                                spacing: 5
                                                Text { text: report.batterySoc; font.pixelSize: 36; font.bold: true; font.family: "monospace"; color: report.batterySoc > 50 ? vscGreen : report.batterySoc > 20 ? "#e8c55a" : vscRed; Behavior on color { ColorAnimation { duration: 300 } } }
                                                Text { text: "%"; font.pixelSize: 16; color: vscTextDim; anchors.bottom: parent.bottom; anchors.bottomMargin: 5 }
                                            }
                                            MiniBar { width: parent.width; height: 10; radius: 5; ratio: report.batterySoc / 100.0; barColor: report.batterySoc > 50 ? vscGreen : report.batterySoc > 20 ? "#e8c55a" : vscRed }
                                        }
                                        Rectangle { width: parent.width; height: 1; color: vscBorder; opacity: 0.4 }
                                        Row {
                                            spacing: 6
                                            Text { text: "Temp"; font.pixelSize: 11; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: report.batteryTemp; font.pixelSize: 18; font.bold: true; font.family: "monospace"; color: vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: "°C"; font.pixelSize: 11; color: vscTextDim; anchors.verticalCenter: parent.verticalCenter }
                                        }
                                    }
                                }

                                DashCard {
                                    cw: (parent.width - 20) / 3
                                    accent: "#9b59b6"
                                    title: "统计"
                                    Column {
                                        width: parent.width; spacing: 6
                                        Row {
                                            spacing: 6
                                            Text { text: "总帧数"; font.pixelSize: 11; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: serial.parsedFrames; font.pixelSize: 22; font.bold: true; font.family: "monospace"; color: vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                                        }
                                        Rectangle { width: parent.width; height: 1; color: vscBorder; opacity: 0.4 }
                                        Row {
                                            spacing: 6
                                            Text { text: "RX字节"; font.pixelSize: 11; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: serial.rxBytes; font.pixelSize: 14; font.bold: true; font.family: "monospace"; color: vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: "TX帧"; font.pixelSize: 11; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: serial.txFrames; font.pixelSize: 14; font.bold: true; font.family: "monospace"; color: vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: "坏帧"; font.pixelSize: 11; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: serial.badFrames; font.pixelSize: 14; font.bold: true; font.family: "monospace"; color: serial.badFrames > 0 ? vscRed : vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                                        }
                                        Rectangle { width: parent.width; height: 1; color: vscBorder; opacity: 0.4 }
                                        Row {
                                            spacing: 6
                                            Text { text: "模块故障"; font.pixelSize: 11; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                                            Text {
                                                text: report.moduleFaultFlags === 0 ? "正常" : "0x" + report.moduleFaultFlags.toString(16).toUpperCase()
                                                font.pixelSize: 14; font.bold: true; font.family: "monospace"
                                                color: report.moduleFaultFlags !== 0 ? vscRed : vscGreen
                                                anchors.verticalCenter: parent.verticalCenter
                                                Behavior on color { ColorAnimation { duration: 200 } }
                                            }
                                        }
                                    }
                                }
                            }

                            // ── 第二行：三电机 ───────────────────────────────────
                            Row {
                                width: parent.width; spacing: 10
                                MotorCard { title: "左电机  LEFT";  cw: (parent.width - 20) / 3; accent: "#9b59b6"; spd: report.leftSpeed;  cur: report.leftCurrent;  enc: report.leftEncoder;  flt: report.leftFault;  maxSpd: 500;  maxCur: 1000 }
                                MotorCard { title: "右电机  RIGHT"; cw: (parent.width - 20) / 3; accent: "#9b59b6"; spd: report.rightSpeed; cur: report.rightCurrent; enc: report.rightEncoder; flt: report.rightFault; maxSpd: 500;  maxCur: 1000 }
                                MotorCard { title: "切割电机  CUT"; cw: (parent.width - 20) / 3; accent: "#e67e22"; spd: report.cutSpeed;   cur: report.cutCurrent;   enc: report.cutEncoder;   flt: report.cutFault;   maxSpd: 1500; maxCur: 1500 }
                            }

                            // ── 第三行：IMU ──────────────────────────────────────
                            DashCard {
                                cw: parent.width
                                accent: "#e67e22"
                                title: "IMU"
                                Row {
                                    width: parent.width; spacing: 40
                                    Column {
                                        spacing: 7
                                        Text { text: "— 加速度 —"; font.pixelSize: 10; color: vscTextSec }
                                        ImuField { lbl: "Accel X"; val: report.imuAccelX + ""; unt: "mg" }
                                        ImuField { lbl: "Accel Y"; val: report.imuAccelY + ""; unt: "mg" }
                                        ImuField { lbl: "Accel Z"; val: report.imuAccelZ + ""; unt: "mg" }
                                    }
                                    Column {
                                        spacing: 7
                                        Text { text: "— 陀螺仪 —"; font.pixelSize: 10; color: vscTextSec }
                                        ImuField { lbl: "Gyro X"; val: report.imuGyroX + ""; unt: "dps" }
                                        ImuField { lbl: "Gyro Y"; val: report.imuGyroY + ""; unt: "dps" }
                                        ImuField { lbl: "Gyro Z"; val: report.imuGyroZ + ""; unt: "dps" }
                                    }
                                    Column {
                                        spacing: 7
                                        Text { text: "— 姿态 —"; font.pixelSize: 10; color: vscTextSec }
                                        ImuField { lbl: "Yaw";   val: report.imuYaw + ""; unt: "°"  }
                                        ImuField { lbl: "Pitch"; val: report.imuPitch + ""; unt: "°"  }
                                        ImuField { lbl: "Roll";  val: report.imuRoll + ""; unt: "°"  }
                                        ImuField { lbl: "Temp";  val: (report.imuTemp  / 10.0 ).toFixed(1); unt: "°C" }
                                    }
                                }
                            }

                            // ── 第四行：系统标志 ─────────────────────────────────
                            DashCard {
                                cw: parent.width
                                accent: vscTextDim
                                title: "系统标志  SYSTEM FLAGS"
                                Flow {
                                    width: parent.width; spacing: 8
                                    FlagBadge { lbl: "READY";              act: !!(report.systemFlags & 0x001) }
                                    FlagBadge { lbl: "RUNNING";            act: !!(report.systemFlags & 0x002) }
                                    FlagBadge { lbl: "CHARGING";           act: !!(report.systemFlags & 0x004) }
                                    FlagBadge { lbl: "SLEEPING";           act: !!(report.systemFlags & 0x008) }
                                    FlagBadge { lbl: "FAULT_EXIST";        act: !!(report.systemFlags & 0x010); err: !!(report.systemFlags & 0x010) }
                                    FlagBadge { lbl: "MOTOR_FAULT_EXIST";  act: !!(report.systemFlags & 0x020); err: !!(report.systemFlags & 0x020) }
                                    FlagBadge { lbl: "TIME_VALID";         act: !!(report.systemFlags & 0x040) }
                                    FlagBadge { lbl: "REMOTE_CTL_TIMEOUT"; act: !!(report.systemFlags & 0x080); err: !!(report.systemFlags & 0x080) }
                                    FlagBadge { lbl: "CUTTER_ENABLED";     act: !!(report.systemFlags & 0x100) }
                                    FlagBadge { lbl: "LIFT_ENABLED";       act: !!(report.systemFlags & 0x200) }
                                    FlagBadge { lbl: "FILE_TX_REQ";        act: !!(report.systemFlags & 0x400) }
                                }
                            }

                            // ── 第五行：传感器标志 ───────────────────────────────
                            DashCard {
                                cw: parent.width
                                accent: vscTextDim
                                title: "传感器  SENSOR FLAGS"
                                Flow {
                                    width: parent.width; spacing: 8
                                    FlagBadge { lbl: "BUMPER_LEFT";  act: !!(report.sensorFlags & 0x01); err: !!(report.sensorFlags & 0x01) }
                                    FlagBadge { lbl: "ESTOP";        act: !!(report.sensorFlags & 0x02); err: !!(report.sensorFlags & 0x02) }
                                    FlagBadge { lbl: "BUMPER_RIGHT"; act: !!(report.sensorFlags & 0x04); err: !!(report.sensorFlags & 0x04) }
                                    FlagBadge { lbl: "LIFT_LEFT";    act: !!(report.sensorFlags & 0x08) }
                                    FlagBadge { lbl: "LIFT_RIGHT";   act: !!(report.sensorFlags & 0x10) }
                                    FlagBadge { lbl: "CUTTER_LIMIT"; act: !!(report.sensorFlags & 0x20) }
                                    FlagBadge { lbl: "DOCK_CONTACT"; act: !!(report.sensorFlags & 0x40) }
                                    FlagBadge { lbl: "RAIN";         act: !!(report.sensorFlags & 0x80) }
                                }
                            }

                            // ── 第六行：模块故障 ─────────────────────────────────
                            DashCard {
                                cw: parent.width
                                accent: vscRed
                                title: "模块故障  MODULE FAULT FLAGS"
                                Flow {
                                    width: parent.width; spacing: 8
                                    FlagBadge { lbl: "BATTERY_PACK"; act: !!(report.moduleFaultFlags & 0x01); err: !!(report.moduleFaultFlags & 0x01) }
                                    FlagBadge { lbl: "MOTOR_LEFT";   act: !!(report.moduleFaultFlags & 0x02); err: !!(report.moduleFaultFlags & 0x02) }
                                    FlagBadge { lbl: "MOTOR_RIGHT";  act: !!(report.moduleFaultFlags & 0x04); err: !!(report.moduleFaultFlags & 0x04) }
                                    FlagBadge { lbl: "MOTOR_CUT";    act: !!(report.moduleFaultFlags & 0x08); err: !!(report.moduleFaultFlags & 0x08) }
                                    FlagBadge { lbl: "MOTOR_LIFT";   act: !!(report.moduleFaultFlags & 0x10); err: !!(report.moduleFaultFlags & 0x10) }
                                    FlagBadge { lbl: "ESTOP";        act: !!(report.moduleFaultFlags & 0x20); err: !!(report.moduleFaultFlags & 0x20) }
                                }
                            }
                        }
                    }
                }

            }

            // ── 文件传输主界面 ─────────────────────────────────────────────────
            Item {
                anchors.fill: parent
                visible: root.activeTab === 1
                Column {
                    anchors.centerIn: parent; spacing: 10
                    Text { text: "⇅";      font.pixelSize: 36; color: vscTextDim; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "文件传输"; font.pixelSize: 14; color: vscTextSec; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "功能开发中"; font.pixelSize: 11; color: vscTextDim; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }


            // ── 终端模式主界面 ─────────────────────────────────────────────────
            Item {
                anchors.fill: parent
                visible: root.activeTab === 2
                Rectangle {
                    anchors.fill: parent; color: "#0c0c0c"
                    Text {
                        anchors { left: parent.left; leftMargin: 12; top: parent.top; topMargin: 10 }
                        text: "$ _"; font.pixelSize: 13; font.family: "monospace"; color: "#cccccc"
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "终端模式  功能开发中"; font.pixelSize: 12; color: "#4e4e4e"
                    }
                }
            }

            // ── 串口设置主界面 ─────────────────────────────────────────────────
            Item {
                anchors.fill: parent
                visible: root.activeTab === 3

                Column {
                    anchors.centerIn: parent
                    width: 380
                    spacing: 14

                    // 连接状态标题
                    Row {
                        spacing: 10; anchors.horizontalCenter: parent.horizontalCenter
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: root.portConnected ? vscGreen : vscTextDim
                            anchors.verticalCenter: parent.verticalCenter
                            Behavior on color { ColorAnimation { duration: 300 } }
                        }
                        Text {
                            text: root.portConnected ? "已连接" : "串口未连接"
                            font.pixelSize: 16; color: vscTextPri
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: serial.statusMessage
                        font.pixelSize: 12
                        color: serial.lastError.length > 0 ? vscRed : vscTextSec
                    }

                    Column {
                        width: parent.width; spacing: 10
                        visible: !root.portConnected
                        Row {
                            width: parent.width; spacing: 8
                            Column {
                                width: parent.width * 0.58 - 4; spacing: 4
                                Text { text: "端  口"; font.pixelSize: 12; color: vscTextSec }
                                ComboBox {
                                    id: portCombo
                                    width: parent.width; height: 32
                                    model: serial.availablePorts
                                    enabled: serial.availablePorts.length > 0
                                    currentIndex: root.portIndex(serial.selectedPort)
                                    displayText: serial.availablePorts.length > 0 ? currentText : "未检测到串口"
                                    onActivated: serial.selectedPort = currentText
                                }
                            }
                            Column {
                                width: parent.width * 0.42 - 4; spacing: 4
                                Text { text: "波特率"; font.pixelSize: 12; color: vscTextSec }
                                ComboBox {
                                    width: parent.width; height: 32
                                    model: root.baudRates
                                    currentIndex: root.baudIndex(serial.baudRate)
                                    onActivated: serial.baudRate = parseInt(currentText)
                                }
                            }
                        }
                        Row {
                            width: parent.width; spacing: 8
                            PortSettingRow { label: "数据位"; value: serial.dataBits; width: (parent.width - 16) / 3 }
                            PortSettingRow { label: "停止位"; value: serial.stopBits; width: (parent.width - 16) / 3 }
                            PortSettingRow { label: "校验位"; value: root.parityText(serial.parity); width: (parent.width - 16) / 3 }
                        }
                        PortSettingRow { label: "流  控"; value: root.flowText(serial.flowControl); width: parent.width }
                    }

                    // 连接 / 断开 按钮
                    Rectangle {
                        width: parent.width; height: 36; radius: 4
                        color: root.portConnected ? Qt.rgba(0.96, 0.53, 0.44, 0.14) : vscAccent
                        border.color: root.portConnected ? vscRed : "transparent"
                        border.width: 1
                        opacity: portActionMA.containsMouse ? 0.80 : 1
                        Behavior on color   { ColorAnimation { duration: 200 } }
                        Behavior on opacity { NumberAnimation { duration: 100 } }
                        Text {
                            anchors.centerIn: parent
                            text: root.portConnected ? "断  开" : "连  接"
                            font.pixelSize: 13; font.bold: true
                            color: root.portConnected ? vscRed : "white"
                        }
                        MouseArea {
                            id: portActionMA; anchors.fill: parent; hoverEnabled: true
                            onClicked: root.portConnected ? serial.closePort() : serial.openPort()
                        }
                    }

                    // 已连接时显示详情
                    Column {
                        width: parent.width; spacing: 6
                        visible: root.portConnected
                        Repeater {
                            model: [
                                { k: "端口",   v: serial.selectedPort },
                                { k: "波特率", v: serial.baudRate },
                                { k: "帧格式", v: root.frameFormatText() },
                                { k: "流控",   v: root.flowText(serial.flowControl) },
                                { k: "RX字节", v: serial.rxBytes },
                                { k: "解析帧", v: serial.parsedFrames },
                                { k: "坏帧",   v: serial.badFrames },
                            ]
                            delegate: Row {
                                required property var modelData
                                width: parent.width
                                Text { text: modelData.k; width: 70; font.pixelSize: 12; color: vscTextSec }
                                Text { text: modelData.v; font.pixelSize: 12; color: vscTextPri }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── 内联组件 ───────────────────────────────────────────────────────────────

    component NavTab: Item {
        id: ntRoot
        property string label:  ""
        property string icon:   ""
        property bool   active: false
        property color  accent: vscAccent
        signal tabClicked()

        height: parent ? parent.height : 0
        width:  ntIcon.width + ntLbl.width + 26

        Rectangle {
            anchors.fill: parent
            color: ntRoot.active ? Qt.rgba(1,1,1,0.04) : ntMA.containsMouse ? Qt.rgba(1,1,1,0.06) : "transparent"
            Behavior on color { ColorAnimation { duration: 80 } }

            // 激活底部彩色粗线
            Rectangle {
                visible: ntRoot.active
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 3; radius: 1.5
                color: ntRoot.accent
            }

            Row {
                anchors.centerIn: parent; spacing: 5

                Text {
                    id: ntIcon; text: ntRoot.icon
                    font.pixelSize: 14
                    color: ntRoot.active ? ntRoot.accent : ntMA.containsMouse ? vscTextPri : vscTextSec
                    anchors.verticalCenter: parent.verticalCenter
                    Behavior on color { ColorAnimation { duration: 80 } }
                }
                Text {
                    id: ntLbl; text: ntRoot.label
                    font.pixelSize: 13
                    font.bold: ntRoot.active
                    color: ntRoot.active ? vscTextPri : ntMA.containsMouse ? Qt.rgba(0.94,0.96,0.99,0.85) : vscTextSec
                    anchors.verticalCenter: parent.verticalCenter
                    Behavior on color { ColorAnimation { duration: 80 } }
                }
            }
            MouseArea { id: ntMA; anchors.fill: parent; hoverEnabled: true; onClicked: ntRoot.tabClicked() }
        }
    }

    component SideSection: Item {
        id: ssRoot
        property string label: ""
        property bool   open: true
        property bool   canCollapse: true
        signal toggled()

        width: parent ? parent.width : 0
        height: 26

        Rectangle {
            anchors.fill: parent
            color: ssMA.containsMouse && ssRoot.canCollapse ? vscHover : "transparent"
            Row {
                anchors { left: parent.left; leftMargin: 8; verticalCenter: parent.verticalCenter }
                spacing: 5
                Text {
                    visible: ssRoot.canCollapse
                    text: ssRoot.open ? "▾" : "▸"
                    font.pixelSize: 11; color: vscTextSec
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: ssRoot.label
                    font.pixelSize: 12; font.bold: true
                    font.letterSpacing: 1.5
                    color: vscTextSec
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: ssMA; anchors.fill: parent; hoverEnabled: true
                onClicked: if (ssRoot.canCollapse) ssRoot.toggled()
            }
        }
    }

    component SideDivider: Item {
        height: 1
        Rectangle { anchors.fill: parent; color: vscBorder }
    }

    component TimerItem: Item {
        id: tiRoot
        property string label: ""
        property int    vValue: 0
        property int    wValue: 0
        property int    periodMs: 100
        property bool   running: false
        property bool   expanded: false
        signal vCommitted(int value)
        signal wCommitted(int value)
        signal periodCommitted(int value)
        signal startRequested()
        signal stopRequested()

        height: tiRoot.expanded ? 136 : 32
        clip: true
        Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.fill: parent
            color: tiMA.containsMouse ? vscSelected : "transparent"
            // 列表行（点击这里展开/折叠）
            Item {
                id: tiRow
                anchors { left: parent.left; right: parent.right; top: parent.top }
                height: 32
                Row {
                    anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                    spacing: 6
                    Rectangle {
                        width: 5; height: 5; radius: 2.5
                        color: tiRoot.running ? vscGreen : vscTextDim
                        anchors.verticalCenter: parent.verticalCenter
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                    Text { text: tiRoot.label; font.pixelSize: 14; color: vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                }
                // 启动/停止按钮
                Rectangle {
                    width: 44; height: 18; radius: 3
                    z: 2
                    anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                    color: tiRoot.running ? Qt.rgba(0.31, 0.79, 0.69, 0.18) : Qt.rgba(0, 0.47, 0.80, 0.18)
                    border.color: tiRoot.running ? vscGreen : vscAccent; border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: tiRoot.running ? "停止" : "启动"
                        font.pixelSize: 11; color: tiRoot.running ? vscGreen : vscAccent
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: function(mouse) {
                            mouse.accepted = true
                            if (tiRoot.running)
                                tiRoot.stopRequested()
                            else
                                tiRoot.startRequested()
                        }
                    }
                }
                // 仅顶部行响应展开/折叠，不覆盖下方输入区
                MouseArea {
                    id: tiMA
                    anchors { left: parent.left; right: parent.right; rightMargin: 58; top: parent.top; bottom: parent.bottom }
                    hoverEnabled: true
                    onClicked: tiRoot.expanded = !tiRoot.expanded
                }
            }

            // 展开：VW 控制参数（TextInput 在此区域，不被 MouseArea 覆盖）
            Column {
                anchors { left: parent.left; right: parent.right; top: parent.top; topMargin: 36 }
                anchors.leftMargin: 18; anchors.rightMargin: 8
                spacing: 6
                Row {
                    width: parent.width; spacing: 6
                    MiniField {
                        fieldLabel: "v"
                        fieldValue: tiRoot.vValue + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { tiRoot.vCommitted(root.parseIntegerField(val, tiRoot.vValue)) }
                    }
                    MiniField {
                        fieldLabel: "w"
                        fieldValue: tiRoot.wValue + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { tiRoot.wCommitted(root.parseIntegerField(val, tiRoot.wValue)) }
                    }
                }
                MiniField {
                    fieldLabel: "周期"
                    fieldValue: tiRoot.periodMs + " ms"
                    width: parent.width
                    onCommitted: function(val) { tiRoot.periodCommitted(root.parseIntegerField(val, tiRoot.periodMs)) }
                }
            }
        }
    }

    component MiniField: Item {
        id: mfRoot
        property string fieldLabel: ""
        property string fieldValue: ""
        signal committed(string val)
        height: 44

        // 同步外部值到编辑框（仅在未聚焦时，避免打断输入）
        onFieldValueChanged: { if (!mfTi.activeFocus) mfTi.text = fieldValue }
        Component.onCompleted: mfTi.text = fieldValue

        Column {
            anchors.fill: parent; spacing: 2
            Row {
                spacing: 5
                Text { text: mfRoot.fieldLabel; font.pixelSize: 11; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                // 聚焦时显示提示
                Text {
                    visible: mfTi.activeFocus
                    text: "↵ 回车确认"
                    font.pixelSize: 9; color: vscAccent; opacity: 0.8
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Rectangle {
                width: parent.width; height: 24; radius: 3
                color: mfTi.activeFocus ? Qt.rgba(0.35,0.65,1,0.08) : vscInput
                border.color: mfTi.activeFocus ? vscAccent : vscBorder; border.width: 1
                Behavior on border.color { ColorAnimation { duration: 100 } }
                TextInput {
                    id: mfTi
                    anchors { left: parent.left; right: parent.right; leftMargin: 6; rightMargin: 6; verticalCenter: parent.verticalCenter }
                    font.pixelSize: 13; color: vscTextPri
                    selectByMouse: true; selectedTextColor: "white"; selectionColor: vscAccent
                    Keys.onReturnPressed:  { mfRoot.committed(text); focus = false }
                    Keys.onEnterPressed:   { mfRoot.committed(text); focus = false }
                    Keys.onEscapePressed:  { text = mfRoot.fieldValue; focus = false }  // Esc 取消
                }
            }
        }
    }

    component CmdItem: Item {
        id: ciRoot
        property string label: ""
        property bool   expanded: false
        property bool   sendEnabled: false
        signal itemClicked()
        signal sendRequested()

        height: ciRoot.expanded ? 90 : 32
        clip: true
        Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.fill: parent
            color: ciMA.containsMouse ? vscSelected : "transparent"

            MouseArea { id: ciMA; anchors.fill: parent; hoverEnabled: true; z: -1; onClicked: ciRoot.itemClicked() }

            // 列表行
            Item {
                anchors { left: parent.left; right: parent.right; top: parent.top }
                height: 32
                Row {
                    anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                    spacing: 5
                    Text {
                        text: ciRoot.expanded ? "▾" : "▸"
                        font.pixelSize: 11; color: vscTextSec
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text { text: ciRoot.label; font.pixelSize: 14; color: vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                }
                // 发送按钮（hover 时显示）
                Rectangle {
                    visible: ciRoot.sendEnabled && ciMA.containsMouse
                    width: 36; height: 18; radius: 3; color: vscAccent
                    anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                    Text { anchors.centerIn: parent; text: "发送"; font.pixelSize: 11; color: "white" }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: function(mouse) {
                            mouse.accepted = true
                            ciRoot.sendRequested()
                        }
                    }
                }
            }

            // 展开：时间戳设置
            Column {
                anchors { left: parent.left; right: parent.right; top: parent.top; topMargin: 36 }
                anchors.leftMargin: 18; anchors.rightMargin: 8
                spacing: 4
                Text { text: "时间戳"; font.pixelSize: 11; color: vscTextSec }
                Rectangle {
                    width: parent.width; height: 24; radius: 3
                    color: vscInput; border.color: vscBorder; border.width: 1
                    Row {
                        anchors { left: parent.left; leftMargin: 6; verticalCenter: parent.verticalCenter }
                        spacing: 6
                        Text { text: "自动"; font.pixelSize: 13; color: vscTextPri; anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { width: 1; height: 12; color: vscBorder; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "点击时读取当前系统时间"; font.pixelSize: 12; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }

        }
    }

    component ModeChip: Rectangle {
        id: mcRoot
        property string label: ""
        property bool   active: false
        height: 18; width: mcLbl.width + 10; radius: 2
        color:        mcRoot.active ? vscAccent : "transparent"
        border.color: mcRoot.active ? vscAccent : vscBorder; border.width: 1
        Text { id: mcLbl; anchors.centerIn: parent; text: mcRoot.label; font.pixelSize: 12; color: mcRoot.active ? "white" : vscTextSec }
    }

    component PortSettingRow: Rectangle {
        id: psrRoot
        property string label: ""
        property string value: ""
        height: 46; radius: 4
        color: psrMA.containsMouse ? vscSelected : vscInput
        border.color: vscBorder; border.width: 1
        Behavior on color { ColorAnimation { duration: 100 } }
        Column {
            anchors { left: parent.left; leftMargin: 8; verticalCenter: parent.verticalCenter }
            spacing: 2
            Text { text: psrRoot.label; font.pixelSize: 12; color: vscTextSec }
            Text { text: psrRoot.value; font.pixelSize: 14; color: vscTextPri }
        }
        Text {
            anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
            text: "▾"; font.pixelSize: 11; color: vscTextDim
        }
        MouseArea { id: psrMA; anchors.fill: parent; hoverEnabled: true }
    }

    // ── DataCard：数据卡片容器 ──────────────────────────────────────────────────
    // 用法：DataCard { title: "xxx"; cardWidth: 220; DataField { ... } DataField { ... } }
    component DataCard: Rectangle {
        id: dcRoot
        property string title: ""
        property int cardWidth: 220
        default property alias content: dcBody.data

        width: cardWidth
        height: dcTitle.height + dcBody.implicitHeight + 32
        radius: 6
        color: vscSidebar
        border.color: vscBorder
        border.width: 1

        Text {
            id: dcTitle
            anchors { top: parent.top; left: parent.left; topMargin: 10; leftMargin: 12 }
            text: dcRoot.title
            font.pixelSize: 12; font.bold: true; font.letterSpacing: 1
            color: vscAccent
        }

        Column {
            id: dcBody
            anchors { top: dcTitle.bottom; left: parent.left; right: parent.right; topMargin: 8; leftMargin: 12; rightMargin: 12 }
            spacing: 5
        }
    }

    // ── DataField：标签 + 值 一行 ──────────────────────────────────────────────
    component DataField: Row {
        property string label: ""
        property string value: ""
        property bool   isError: false
        spacing: 0
        Text {
            width: 80; text: label
            font.pixelSize: 13; color: vscTextSec
        }
        Text {
            text: value
            font.pixelSize: 13; font.family: "monospace"
            color: isError ? vscRed : vscTextPri
        }
    }

    // ── FlagDot：位标志指示器 ───────────────────────────────────────────────────
    component FlagDot: Row {
        property bool   active: false
        property string label: ""
        property bool   isError: false
        spacing: 5
        Rectangle {
            width: 7; height: 7; radius: 3.5
            anchors.verticalCenter: parent.verticalCenter
            color: active ? (isError ? vscRed : vscGreen) : vscTextDim
            Behavior on color { ColorAnimation { duration: 150 } }
        }
        Text {
            text: label
            font.pixelSize: 12; font.family: "monospace"
            color: active ? (isError ? vscRed : vscTextPri) : vscTextSec
            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }

    // ── DashCard：带彩色 accent 竖线的数据卡片 ────────────────────────────────
    component DashCard: Item {
        id: dcRoot
        property int    cw: 220
        property color  accent: vscAccent
        property string title: ""
        default property alias dcContent: dcBody.data

        width: cw
        height: dcCard.height

        // 阴影层（偏移矩形模拟）
        Rectangle {
            anchors { fill: dcCard; topMargin: -1; leftMargin: -1; rightMargin: -1; bottomMargin: -3 }
            radius: 7; color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.5); border.width: 1
            z: 0
        }
        Rectangle {
            anchors { fill: dcCard; topMargin: 2; leftMargin: 1; rightMargin: 1; bottomMargin: -4 }
            radius: 7; color: Qt.rgba(0, 0, 0, 0.25)
            z: 0
        }

        Rectangle {
            id: dcCard
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: dcTitleTxt.height + dcBody.implicitHeight + 38
            radius: 6; clip: true
            color: vscSidebar
            border.color: dcRoot.accent === vscAccent ? vscCardBorder : Qt.rgba(dcRoot.accent.r, dcRoot.accent.g, dcRoot.accent.b, 0.5)
            border.width: 1
            z: 1

            Rectangle {
                anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
                width: 3; color: dcRoot.accent
            }
            Text {
                id: dcTitleTxt
                anchors { top: parent.top; left: parent.left; topMargin: 10; leftMargin: 14 }
                text: dcRoot.title
                font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.2
                color: dcRoot.accent
            }
            Column {
                id: dcBody
                anchors { top: dcTitleTxt.bottom; left: parent.left; right: parent.right
                          topMargin: 10; leftMargin: 14; rightMargin: 14 }
                spacing: 0
            }
        }
    }

    // ── MiniBar：响应式进度条 ─────────────────────────────────────────────────
    component MiniBar: Rectangle {
        property real  ratio: 0.5
        property color barColor: vscGreen
        height: 4; radius: 2; color: vscInput; clip: true
        Rectangle {
            width: parent.width * Math.max(0.0, Math.min(1.0, ratio))
            height: parent.height; radius: 2
            color: parent.barColor
            Behavior on width { NumberAnimation { duration: 120 } }
            Behavior on color { ColorAnimation  { duration: 200 } }
        }
    }

    // ── MotorCard：单电机完整展示卡 ──────────────────────────────────────────
    component MotorCard: Item {
        id: mcRoot
        property int    cw: 200
        property color  accent: "#9b59b6"
        property string title: ""
        property int    spd: 0
        property int    cur: 0
        property int    enc: 0
        property int    flt: 0
        property int    maxSpd: 500
        property int    maxCur: 1000

        width: cw
        height: mcCard.height

        // 阴影层
        Rectangle {
            anchors { fill: mcCard; topMargin: 2; leftMargin: 1; rightMargin: 1; bottomMargin: -4 }
            radius: 7; color: Qt.rgba(0, 0, 0, 0.30); z: 0
        }

        Rectangle {
            id: mcCard
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: mcBody.implicitHeight + 48
            radius: 6; clip: true
            color: flt !== 0 ? Qt.rgba(0.96, 0.33, 0.24, 0.08) : vscSidebar
            border.color: flt !== 0 ? vscRed : Qt.rgba(mcRoot.accent.r, mcRoot.accent.g, mcRoot.accent.b, 0.55)
            border.width: 1
            Behavior on color { ColorAnimation { duration: 200 } }

        Rectangle {
            anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
            width: 3
            color: flt !== 0 ? vscRed : mcRoot.accent
            Behavior on color { ColorAnimation { duration: 200 } }
        }
        Text {
            id: mcTitle
            anchors { top: parent.top; left: parent.left; topMargin: 10; leftMargin: 14 }
            text: mcRoot.title
            font.pixelSize: 11; font.bold: true; font.letterSpacing: 1
            color: flt !== 0 ? vscRed : mcRoot.accent
            Behavior on color { ColorAnimation { duration: 200 } }
        }
        Column {
            id: mcBody
            anchors { top: mcTitle.bottom; left: parent.left; right: parent.right
                      topMargin: 10; leftMargin: 14; rightMargin: 14 }
            spacing: 8

            // Speed
            Column {
                width: parent.width; spacing: 4
                Row {
                    width: parent.width
                    Text { text: "Speed"; font.pixelSize: 10; color: vscTextSec; width: parent.width - spdDir.width }
                    Text {
                        id: spdDir
                        text: spd > 0 ? "↑" : spd < 0 ? "↓" : "—"
                        font.pixelSize: 13
                        color: spd > 0 ? vscGreen : spd < 0 ? "#e8a55a" : vscTextDim
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                }
                Row {
                    spacing: 4
                    Text { text: spd; font.pixelSize: 20; font.bold: true; font.family: "monospace"; color: vscTextPri }
                    Text { text: "rpm"; font.pixelSize: 10; color: vscTextDim; anchors.bottom: parent.bottom; anchors.bottomMargin: 2 }
                }
            }

            // Current
            Column {
                width: parent.width; spacing: 4
                Text { text: "Current"; font.pixelSize: 10; color: vscTextSec }
                Row {
                    spacing: 4
                    Text {
                        text: cur; font.pixelSize: 16; font.bold: true; font.family: "monospace"
                        color: cur > maxCur * 0.8 ? vscRed : vscTextPri
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    Text { text: "mA"; font.pixelSize: 10; color: vscTextDim; anchors.bottom: parent.bottom; anchors.bottomMargin: 2 }
                }
            }

            // Encoder
            Row {
                spacing: 8
                Text { text: "Enc"; font.pixelSize: 10; color: vscTextSec; anchors.verticalCenter: parent.verticalCenter }
                Text { text: enc; font.pixelSize: 13; font.family: "monospace"; color: vscTextSec }
            }

            // Fault badge
            Rectangle {
                width: parent.width; height: 24; radius: 4
                color: flt !== 0 ? Qt.rgba(0.96, 0.33, 0.24, 0.15) : Qt.rgba(0.31, 0.79, 0.69, 0.10)
                border.color: flt !== 0 ? vscRed : vscGreen; border.width: 1
                Behavior on color { ColorAnimation { duration: 200 } }
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Rectangle {
                        width: 6; height: 6; radius: 3; anchors.verticalCenter: parent.verticalCenter
                        color: flt !== 0 ? vscRed : vscGreen
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                    Text {
                        text: flt !== 0 ? "FAULT  0x" + flt.toString(16).toUpperCase() : "OK"
                        font.pixelSize: 11; font.bold: true
                        color: flt !== 0 ? vscRed : vscGreen
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                }
            }
            }
        }
    }

    // ── ImuField：IMU 数值行 ──────────────────────────────────────────────────
    component ImuField: Row {
        property string lbl: ""
        property string val: ""
        property string unt: ""
        spacing: 6
        Text { text: lbl; font.pixelSize: 11; color: vscTextSec; width: 62; font.family: "monospace" }
        Text { text: val; font.pixelSize: 15; font.bold: true; font.family: "monospace"; color: vscTextPri }
        Text { text: unt; font.pixelSize: 10; color: vscTextDim; anchors.bottom: parent.bottom; anchors.bottomMargin: 2 }
    }

    // ── FlagBadge：标志 badge 指示器 ─────────────────────────────────────────
    component FlagBadge: Rectangle {
        property string lbl: ""
        property bool   act: false
        property bool   err: false
        height: 26; radius: 4; width: fbLbl.width + 22
        color: act ? (err ? Qt.rgba(0.96, 0.33, 0.24, 0.15) : Qt.rgba(0.31, 0.79, 0.69, 0.12)) : vscInput
        border.color: act ? (err ? vscRed : vscGreen) : vscBorder; border.width: 1
        Behavior on color { ColorAnimation { duration: 150 } }
        Row {
            anchors.centerIn: parent; spacing: 6
            Rectangle {
                width: 7; height: 7; radius: 3.5; anchors.verticalCenter: parent.verticalCenter
                color: act ? (err ? vscRed : vscGreen) : vscTextDim
                Behavior on color { ColorAnimation { duration: 150 } }
            }
            Text {
                id: fbLbl; text: lbl
                font.pixelSize: 11; font.family: "monospace"
                color: act ? (err ? vscRed : vscTextPri) : vscTextSec
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }
    }
}
