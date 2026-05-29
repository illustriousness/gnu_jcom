import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: dialog

    property var serialController
    property string entryPassword: "123456"
    property string passwordInput: ""
    property string errorText: ""
    property color backgroundColor: "#1c2128"
    property color borderColor: "#d29922"
    property color textPrimary: "#f0f6fc"
    property color textSecondary: "#8b949e"
    property color errorColor: "#f85149"
    property int topOffset: 72

    signal activated()

    modal: true
    focus: true
    width: 360
    height: contentColumn.implicitHeight + 28
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: topOffset
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    function openForTerminal() {
        passwordInput = ""
        errorText = ""
        open()
    }

    function confirmPassword() {
        if (passwordInput !== entryPassword) {
            errorText = "密码错误"
            passwordField.forceActiveFocus()
            passwordField.selectAll()
            return
        }

        if (serialController && serialController.activateTerminal()) {
            activated()
            close()
            return
        }

        errorText = serialController && serialController.lastError.length > 0
                    ? serialController.lastError
                    : "终端激活发送失败"
        passwordField.forceActiveFocus()
    }

    onOpened: passwordField.forceActiveFocus()

    background: Rectangle {
        radius: 8
        color: dialog.backgroundColor
        border.color: dialog.borderColor
        border.width: 1
    }

    contentItem: Column {
        id: contentColumn
        width: parent.width
        spacing: 12

        Text {
            width: parent.width
            text: "进入终端模式"
            font.pixelSize: 16
            font.bold: true
            color: dialog.textPrimary
        }

        Text {
            width: parent.width
            text: "输入密码后才会发送终端激活命令。"
            font.pixelSize: 12
            color: dialog.textSecondary
            wrapMode: Text.WordWrap
        }

        TextField {
            id: passwordField
            width: parent.width
            height: 34
            text: dialog.passwordInput
            echoMode: TextInput.Password
            placeholderText: "密码"
            selectByMouse: true
            onTextChanged: dialog.passwordInput = text
            Keys.onReturnPressed: dialog.confirmPassword()
            Keys.onEnterPressed: dialog.confirmPassword()
        }

        Text {
            width: parent.width
            visible: dialog.errorText.length > 0
            text: dialog.errorText
            font.pixelSize: 12
            color: dialog.errorColor
            wrapMode: Text.WordWrap
        }

        Row {
            anchors.right: parent.right
            spacing: 8

            Button {
                text: "取消"
                onClicked: dialog.close()
            }

            Button {
                text: "进入"
                onClicked: dialog.confirmPassword()
            }
        }
    }
}
