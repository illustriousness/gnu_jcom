// 示例 04：Row 和 Column
// 知识点：自动排列子元素，不需要手动设 x/y

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 460
    height: 360
    visible: true
    title: "04 - Row & Column"
    color: "#1e1e2e"

    // ── Row：横向排列 ─────────────────────────────────────────

    Text {
        text: "Row（横向）"
        color: "#cdd6f4"; font.pixelSize: 13
        anchors.top: parent.top; anchors.left: parent.left
        anchors.margins: 16
    }

    Row {
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.left: parent.left
        anchors.leftMargin: 16
        spacing: 10          // 子元素之间的间距

        Rectangle { width: 60; height: 60; color: "#f38ba8"; radius: 6 }
        Rectangle { width: 60; height: 60; color: "#fab387"; radius: 6 }
        Rectangle { width: 60; height: 60; color: "#f9e2af"; radius: 6 }
        Rectangle { width: 60; height: 60; color: "#a6e3a1"; radius: 6 }
    }

    // ── Column：纵向排列 ──────────────────────────────────────

    Text {
        text: "Column（纵向）"
        color: "#cdd6f4"; font.pixelSize: 13
        anchors.top: parent.top; anchors.topMargin: 120
        anchors.left: parent.left; anchors.leftMargin: 16
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 146
        anchors.left: parent.left
        anchors.leftMargin: 16
        spacing: 8

        Rectangle { width: 200; height: 36; color: "#89b4fa"; radius: 6
            Text { anchors.centerIn: parent; text: "第一行"; color: "#1e1e2e" } }
        Rectangle { width: 200; height: 36; color: "#74c7ec"; radius: 6
            Text { anchors.centerIn: parent; text: "第二行"; color: "#1e1e2e" } }
        Rectangle { width: 200; height: 36; color: "#89dceb"; radius: 6
            Text { anchors.centerIn: parent; text: "第三行"; color: "#1e1e2e" } }
    }

    // ── 嵌套：Column 里放 Row ─────────────────────────────────

    Text {
        text: "Column 嵌套 Row"
        color: "#cdd6f4"; font.pixelSize: 13
        anchors.top: parent.top; anchors.topMargin: 120
        anchors.left: parent.left; anchors.leftMargin: 250
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 146
        anchors.left: parent.left
        anchors.leftMargin: 250
        spacing: 8

        Repeater {
            // Repeater：重复生成 3 个子元素，不用写三遍
            model: 3
            Row {
                spacing: 6
                Repeater {
                    model: 3
                    Rectangle {
                        width: 32; height: 32; radius: 4
                        color: Qt.rgba(0.3 + index * 0.1, 0.5, 0.8, 1)
                    }
                }
            }
        }
    }
}
