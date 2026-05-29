import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    property var serialController
    property color terminalBackground: "#0c0c0c"
    property color terminalText: "#cccccc"
    property color terminalDim: "#4e4e4e"
    property color terminalBorder: "#30363d"
    property bool cursorBlinkOn: true

    focus: visible
    activeFocusOnTab: true

    function ensureTerminalFocus() {
        if (visible && serialController && serialController.terminalMode)
            forceActiveFocus()
    }

    function copySelectionToClipboard() {
        if (!serialController)
            return false
        if (terminalOutput.selectedText.length <= 0)
            return false
        serialController.copyTextToClipboard(terminalOutput.selectedText)
        return true
    }

    function pasteClipboardToTerminal() {
        if (!serialController || !serialController.terminalMode)
            return false
        return serialController.sendClipboardText()
    }

    function handleTerminalKey(event) {
        if (!serialController || !serialController.terminalMode)
            return

        var payload = ""
        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_C) {
            root.copySelectionToClipboard()
            event.accepted = true
            return
        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_V) {
            root.pasteClipboardToTerminal()
            event.accepted = true
            return
        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Left) {
            payload = "\x1b[1;5D"
        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Right) {
            payload = "\x1b[1;5C"
        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Backspace) {
            payload = "\x17"
        } else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Delete) {
            payload = "\x1b[3;5~"
        } else if ((event.modifiers & Qt.ControlModifier) && event.key >= Qt.Key_A && event.key <= Qt.Key_Z) {
            payload = String.fromCharCode(event.key - Qt.Key_A + 1)
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            payload = "\n"
        } else if (event.key === Qt.Key_Backspace) {
            payload = "\x7f"
        } else if (event.key === Qt.Key_Delete) {
            payload = "\x1b[3~"
        } else if (event.key === Qt.Key_Tab) {
            payload = "\t"
        } else if (event.key === Qt.Key_Escape) {
            payload = "\x1b"
        } else if (event.key === Qt.Key_Up) {
            payload = "\x1b[A"
        } else if (event.key === Qt.Key_Down) {
            payload = "\x1b[B"
        } else if (event.key === Qt.Key_Right) {
            payload = "\x1b[C"
        } else if (event.key === Qt.Key_Left) {
            payload = "\x1b[D"
        } else if (event.key === Qt.Key_Home) {
            payload = "\x1b[H"
        } else if (event.key === Qt.Key_End) {
            payload = "\x1b[F"
        } else {
            payload = event.text
        }

        if (payload.length > 0) {
            event.accepted = serialController.sendTerminalText(payload)
        }
    }

    onVisibleChanged: ensureTerminalFocus()

    Keys.onPressed: function(event) {
        root.handleTerminalKey(event)
    }

    Connections {
        target: root.serialController
        function onTerminalModeChanged() {
            root.cursorBlinkOn = true
            root.ensureTerminalFocus()
        }
        function onTerminalBufferChanged() {
            root.cursorBlinkOn = true
        }
    }

    Timer {
        interval: 530
        repeat: true
        running: root.visible && root.serialController && root.serialController.terminalMode
        onTriggered: root.cursorBlinkOn = !root.cursorBlinkOn
    }

    Rectangle {
        anchors.fill: parent
        color: root.terminalBackground

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: root.ensureTerminalFocus()
        }

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Item {
                width: parent.width
                height: 28

                Text {
                    id: terminalTitle
                    text: serialController && serialController.terminalMode
                          ? "终端透传模式"
                          : "终端未激活"
                    font.pixelSize: 13
                    color: root.terminalText
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    anchors {
                        left: terminalTitle.right
                        leftMargin: 8
                        verticalCenter: parent.verticalCenter
                    }
                    text: serialController && serialController.terminalMode
                          ? "键盘输入实时透传到串口，点击退出按钮离开终端"
                          : "请从顶栏进入终端模式"
                    font.pixelSize: 11
                    color: root.terminalDim
                }

                Row {
                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    spacing: 8

                    Button {
                        text: "清空"
                        enabled: serialController
                        onClicked: {
                            serialController.clearTerminalBuffer()
                            root.ensureTerminalFocus()
                        }
                    }

                    Button {
                        text: "退出终端"
                        enabled: serialController && serialController.terminalMode
                        onClicked: serialController.closePort()
                    }
                }
            }

            ScrollView {
                id: terminalScroll
                width: parent.width
                height: parent.height - 36
                clip: true

                TextArea {
                    id: terminalOutput
                    property rect terminalCursorRect: serialController
                                                       ? positionToRectangle(Math.max(0, Math.min(serialController.terminalCursorPosition, length)))
                                                       : Qt.rect(0, 0, 0, 0)
                    readOnly: true
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.NoWrap
                    text: serialController ? serialController.terminalHtml : ""
                    color: root.terminalText
                    font.family: "monospace"
                    font.pixelSize: 13
                    cursorVisible: false
                    persistentSelection: true
                    selectByMouse: true
                    background: Rectangle {
                        color: root.terminalBackground
                        border.color: root.terminalBorder
                        border.width: 1
                    }
                    onTextChanged: cursorPosition = length

                    Keys.onPressed: function(event) {
                        root.handleTerminalKey(event)
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: root.pasteClipboardToTerminal()
                    }

                    Rectangle {
                        x: terminalOutput.terminalCursorRect.x
                        y: terminalOutput.terminalCursorRect.y + 1
                        width: 2
                        height: Math.max(terminalOutput.font.pixelSize + 2, terminalOutput.terminalCursorRect.height - 2)
                        color: root.terminalText
                        visible: root.visible &&
                                 root.cursorBlinkOn &&
                                 serialController &&
                                 serialController.terminalMode
                    }
                }
            }
        }
    }
}
