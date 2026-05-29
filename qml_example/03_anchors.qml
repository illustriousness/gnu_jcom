// 示例 03：anchors 布局
// 知识点：用 anchors 做自适应布局，比 x/y 固定坐标更灵活

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 500
    height: 400
    visible: true
    title: "03 - Anchors"
    color: "#1e1e2e"

    // ── 贴四个角 ──────────────────────────────────────────────

    Rectangle {
        width: 80; height: 50; color: "#f38ba8"; radius: 6
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 12       // 四边同时留 12px 间距
        Text { anchors.centerIn: parent; text: "左上"; color: "white" }
    }

    Rectangle {
        width: 80; height: 50; color: "#fab387"; radius: 6
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 12
        Text { anchors.centerIn: parent; text: "右上"; color: "white" }
    }

    Rectangle {
        width: 80; height: 50; color: "#a6e3a1"; radius: 6
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 12
        Text { anchors.centerIn: parent; text: "左下"; color: "white" }
    }

    Rectangle {
        width: 80; height: 50; color: "#89b4fa"; radius: 6
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 12
        Text { anchors.centerIn: parent; text: "右下"; color: "white" }
    }

    // ── 绝对居中 ──────────────────────────────────────────────

    Rectangle {
        width: 120; height: 60; color: "#cba6f7"; radius: 8
        anchors.centerIn: parent    // 水平 + 垂直都居中
        Text { anchors.centerIn: parent; text: "正中央"; color: "white"; font.pixelSize: 16 }
    }

    // ── 靠上居中（水平居中，距顶部固定） ──────────────────────

    Rectangle {
        width: 180; height: 36; color: "#f9e2af"; radius: 6
        anchors.horizontalCenter: parent.horizontalCenter   // 只水平居中
        anchors.top: parent.top
        anchors.topMargin: 14
        Text { anchors.centerIn: parent; text: "顶部水平居中"; color: "#1e1e2e" }
    }

    // ── 两个矩形互相锚定 ──────────────────────────────────────

    Rectangle {
        id: leftBox
        width: 90; height: 44; color: "#45475a"; radius: 6
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 12
        Text { anchors.centerIn: parent; text: "左侧盒"; color: "white" }
    }

    Rectangle {
        width: 90; height: 44; color: "#585b70"; radius: 6
        // 紧贴 leftBox 的右侧，间距 8px
        anchors.left: leftBox.right
        anchors.leftMargin: 8
        anchors.verticalCenter: leftBox.verticalCenter   // 和 leftBox 垂直对齐
        Text { anchors.centerIn: parent; text: "右侧盒"; color: "white" }
    }
}
