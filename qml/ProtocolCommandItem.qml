import QtQuick 2.15

Item {
    id: root

    property string label: ""
    property string commandType: ""
    property bool expanded: false
    property bool sendEnabled: true
    property int expandedHeight: 90

    property int motorId: 0
    property int motorPayload: 0
    property int ledMode: 0
    property int ledId: 0
    property int ledR: 255
    property int ledG: 255
    property int ledB: 255
    property int ledInterval: 500
    property int powerAction: 1
    property int buzzerId: 0
    property int buzzerRepeat: 1
    property int buzzerOnMs: 100
    property int buzzerOffMs: 100

    property color selectedColor: "#2d333b"
    property color textPrimary: "#f0f6fc"
    property color textSecondary: "#8b949e"
    property color textDim: "#484f58"
    property color inputColor: "#21262d"
    property color borderColor: "#30363d"
    property color accentColor: "#58a6ff"

    signal itemClicked()
    signal sendRequested()

    height: root.expanded ? root.expandedHeight : 32
    clip: true

    function parseIntegerField(text, fallbackValue) {
        var match = String(text).match(/-?\d+/)
        return match === null ? fallbackValue : parseInt(match[0])
    }

    Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

    Rectangle {
        anchors.fill: parent
        color: rowMouse.containsMouse ? root.selectedColor : "transparent"

        Item {
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: 32

            Row {
                anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                spacing: 5

                Text {
                    text: root.expanded ? "▾" : "▸"
                    font.pixelSize: 11
                    color: root.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: root.label
                    font.pixelSize: 14
                    color: root.textPrimary
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: rowMouse
                anchors { left: parent.left; right: parent.right; rightMargin: 52; top: parent.top }
                height: parent.height
                hoverEnabled: true
                onClicked: root.itemClicked()
            }

            Rectangle {
                visible: root.sendEnabled
                width: 36
                height: 18
                radius: 3
                color: root.accentColor
                anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }

                Text {
                    anchors.centerIn: parent
                    text: "发送"
                    font.pixelSize: 11
                    color: "white"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: function(mouse) {
                        mouse.accepted = true
                        root.sendRequested()
                    }
                }
            }
        }

        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; topMargin: 36 }
            anchors.leftMargin: 18
            anchors.rightMargin: 8
            spacing: 6

            Text {
                visible: root.commandType === "time"
                width: parent.width
                text: "点击发送时读取当前系统时间戳"
                font.pixelSize: 12
                color: root.textSecondary
                wrapMode: Text.WordWrap
            }

            Column {
                visible: root.commandType === "motor"
                width: parent.width
                spacing: 6

                Text {
                    width: parent.width
                    text: "motor_id: 0=割草转速, 1=抬升草高"
                    font.pixelSize: 11
                    color: root.textSecondary
                    wrapMode: Text.WordWrap
                }

                Row {
                    width: parent.width
                    spacing: 6

                    MiniField {
                        fieldLabel: "motor_id"
                        fieldValue: root.motorId + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.motorId = root.parseIntegerField(val, root.motorId) }
                    }

                    MiniField {
                        fieldLabel: "payload"
                        fieldValue: root.motorPayload + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.motorPayload = root.parseIntegerField(val, root.motorPayload) }
                    }
                }
            }

            Column {
                visible: root.commandType === "led"
                width: parent.width
                spacing: 6

                Text {
                    width: parent.width
                    text: "mode: 0=闪烁, 1=流水, 2=呼吸; id: 0=灯带, 1=LED0"
                    font.pixelSize: 11
                    color: root.textSecondary
                    wrapMode: Text.WordWrap
                }

                Row {
                    width: parent.width
                    spacing: 6

                    MiniField {
                        fieldLabel: "mode"
                        fieldValue: root.ledMode + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.ledMode = root.parseIntegerField(val, root.ledMode) }
                    }

                    MiniField {
                        fieldLabel: "id"
                        fieldValue: root.ledId + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.ledId = root.parseIntegerField(val, root.ledId) }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 6

                    MiniField {
                        fieldLabel: "R"
                        fieldValue: root.ledR + ""
                        width: (parent.width - 12) / 3
                        onCommitted: function(val) { root.ledR = root.parseIntegerField(val, root.ledR) }
                    }

                    MiniField {
                        fieldLabel: "G"
                        fieldValue: root.ledG + ""
                        width: (parent.width - 12) / 3
                        onCommitted: function(val) { root.ledG = root.parseIntegerField(val, root.ledG) }
                    }

                    MiniField {
                        fieldLabel: "B"
                        fieldValue: root.ledB + ""
                        width: (parent.width - 12) / 3
                        onCommitted: function(val) { root.ledB = root.parseIntegerField(val, root.ledB) }
                    }
                }

                MiniField {
                    fieldLabel: "interval"
                    fieldValue: root.ledInterval + " ms"
                    width: parent.width
                    onCommitted: function(val) { root.ledInterval = root.parseIntegerField(val, root.ledInterval) }
                }
            }

            Column {
                visible: root.commandType === "power"
                width: parent.width
                spacing: 6

                Text {
                    width: parent.width
                    text: "action: 0=关机, 1=休眠"
                    font.pixelSize: 11
                    color: root.textSecondary
                    wrapMode: Text.WordWrap
                }

                MiniField {
                    fieldLabel: "action"
                    fieldValue: root.powerAction + ""
                    width: parent.width
                    onCommitted: function(val) { root.powerAction = root.parseIntegerField(val, root.powerAction) }
                }
            }

            Column {
                visible: root.commandType === "buzzer"
                width: parent.width
                spacing: 6

                Row {
                    width: parent.width
                    spacing: 6

                    MiniField {
                        fieldLabel: "id"
                        fieldValue: root.buzzerId + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.buzzerId = root.parseIntegerField(val, root.buzzerId) }
                    }

                    MiniField {
                        fieldLabel: "repeat"
                        fieldValue: root.buzzerRepeat + ""
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.buzzerRepeat = root.parseIntegerField(val, root.buzzerRepeat) }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 6

                    MiniField {
                        fieldLabel: "on"
                        fieldValue: root.buzzerOnMs + " ms"
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.buzzerOnMs = root.parseIntegerField(val, root.buzzerOnMs) }
                    }

                    MiniField {
                        fieldLabel: "off"
                        fieldValue: root.buzzerOffMs + " ms"
                        width: (parent.width - 6) / 2
                        onCommitted: function(val) { root.buzzerOffMs = root.parseIntegerField(val, root.buzzerOffMs) }
                    }
                }
            }
        }
    }
}
