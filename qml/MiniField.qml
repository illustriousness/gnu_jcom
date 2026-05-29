import QtQuick 2.15

Item {
    id: root

    property string fieldLabel: ""
    property string fieldValue: ""
    property color textPrimary: "#f0f6fc"
    property color textSecondary: "#8b949e"
    property color inputColor: "#21262d"
    property color borderColor: "#30363d"
    property color accentColor: "#58a6ff"

    signal committed(string val)

    height: 44

    onFieldValueChanged: {
        if (!fieldInput.activeFocus)
            fieldInput.text = fieldValue
    }
    Component.onCompleted: fieldInput.text = fieldValue

    Column {
        anchors.fill: parent
        spacing: 2

        Row {
            spacing: 5
            Text {
                text: root.fieldLabel
                font.pixelSize: 11
                color: root.textSecondary
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                visible: fieldInput.activeFocus
                text: "↵ 回车确认"
                font.pixelSize: 9
                color: root.accentColor
                opacity: 0.8
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Rectangle {
            width: parent.width
            height: 24
            radius: 3
            color: fieldInput.activeFocus ? Qt.rgba(0.35, 0.65, 1, 0.08) : root.inputColor
            border.color: fieldInput.activeFocus ? root.accentColor : root.borderColor
            border.width: 1

            Behavior on border.color { ColorAnimation { duration: 100 } }

            TextInput {
                id: fieldInput
                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: 6
                    rightMargin: 6
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: 13
                color: root.textPrimary
                selectByMouse: true
                selectedTextColor: "white"
                selectionColor: root.accentColor
                Keys.onReturnPressed: {
                    root.committed(text)
                    focus = false
                }
                Keys.onEnterPressed: {
                    root.committed(text)
                    focus = false
                }
                Keys.onEscapePressed: {
                    text = root.fieldValue
                    focus = false
                }
            }
        }
    }
}
