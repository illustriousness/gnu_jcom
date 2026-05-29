// 示例 06：Animation 动画
// 知识点：NumberAnimation、ColorAnimation、Behavior、SequentialAnimation

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 460
    height: 380
    visible: true
    title: "06 - Animation"
    color: "#1e1e2e"

    // ── Behavior：属性变化时自动加动画 ───────────────────────
    // 这是最常用的方式——只要属性一变，动画自动播放

    Text {
        text: "Behavior（点击色块）"
        color: "#6c7086"; font.pixelSize: 12
        x: 16; y: 14
    }

    Rectangle {
        id: behaviorBox
        x: 16; y: 36
        width: 100; height: 60; radius: 8
        property bool on: false
        color: on ? "#a6e3a1" : "#f38ba8"

        // Behavior on color 表示：每次 color 属性变化都用 ColorAnimation 过渡
        Behavior on color {
            ColorAnimation { duration: 400 }
        }

        Text { anchors.centerIn: parent; text: "颜色"; color: "#1e1e2e"; font.pixelSize: 14 }
        MouseArea { anchors.fill: parent; onClicked: behaviorBox.on = !behaviorBox.on }
    }

    Rectangle {
        id: moveBox
        x: 130; y: 36
        width: 100; height: 60; radius: 8; color: "#89b4fa"
        property bool moved: false

        // Behavior on x：x 值变化时自动弹性动画
        Behavior on x {
            NumberAnimation { duration: 400; easing.type: Easing.OutBounce }
        }

        Text { anchors.centerIn: parent; text: "弹跳"; color: "#1e1e2e"; font.pixelSize: 14 }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                moveBox.moved = !moveBox.moved
                moveBox.x = moveBox.moved ? 260 : 130
            }
        }
    }

    // ── NumberAnimation：直接控制的动画 ──────────────────────

    Text {
        text: "NumberAnimation（点击启动）"
        color: "#6c7086"; font.pixelSize: 12
        x: 16; y: 120
    }

    Rectangle {
        id: spinBox
        x: 16; y: 142
        width: 60; height: 60; radius: 8; color: "#cba6f7"
        Text { anchors.centerIn: parent; text: "转"; color: "#1e1e2e"; font.pixelSize: 16; font.bold: true }
        transform: Rotation {
            id: spinRotation
            origin.x: 30; origin.y: 30   // 旋转中心
            angle: 0
        }
        MouseArea {
            anchors.fill: parent
            onClicked: rotAnim.start()
        }
    }

    NumberAnimation {
        id: rotAnim
        target: spinRotation
        property: "angle"
        from: 0; to: 360
        duration: 800
        easing.type: Easing.InOutQuad
    }

    // ── SequentialAnimation：按顺序播放多个动画 ──────────────

    Text {
        text: "SequentialAnimation（自动循环）"
        color: "#6c7086"; font.pixelSize: 12
        x: 16; y: 226
    }

    Rectangle {
        id: seqBox
        x: 16; y: 248
        width: 50; height: 50; radius: 25; color: "#f9e2af"
    }

    SequentialAnimation {
        running: true      // 页面一加载就开始
        loops: Animation.Infinite   // 无限循环

        // 向右移动
        NumberAnimation {
            target: seqBox; property: "x"
            to: 360; duration: 1200
            easing.type: Easing.InOutSine
        }
        // 缩小
        NumberAnimation {
            target: seqBox; property: "width"
            to: 20; duration: 300
        }
        NumberAnimation {
            target: seqBox; property: "height"
            to: 20; duration: 300
        }
        // 向左移动回来
        NumberAnimation {
            target: seqBox; property: "x"
            to: 16; duration: 1200
            easing.type: Easing.InOutSine
        }
        // 恢复大小
        NumberAnimation {
            target: seqBox; property: "width"
            to: 50; duration: 300
        }
        NumberAnimation {
            target: seqBox; property: "height"
            to: 50; duration: 300
        }
    }
}
