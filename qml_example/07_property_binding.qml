// 示例 07：property 属性与数据绑定
// 知识点：自定义属性、属性绑定（binding）、property alias

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 460
    height: 360
    visible: true
    title: "07 - Property & Binding"
    color: "#1e1e2e"

    // ── 属性绑定：一个值变，依赖它的值自动更新 ───────────────

    Text {
        text: "属性绑定"
        color: "#6c7086"; font.pixelSize: 12; x: 16; y: 14
    }

    // 这个滑块模拟一个 0~100 的值
    Rectangle {
        id: sliderTrack
        x: 16; y: 36
        width: 300; height: 12; radius: 6; color: "#313244"

        Rectangle {
            id: sliderFill
            height: parent.height; radius: parent.radius
            // 宽度绑定到 root.sliderValue，值一变宽度自动变
            width: root.sliderValue / 100 * parent.width
            color: "#89b4fa"
            Behavior on width { NumberAnimation { duration: 80 } }
        }

        MouseArea {
            anchors.fill: parent
            // 点击或拖动改变值
            onClicked:      (mouse) => root.sliderValue = Math.round(mouse.x / width * 100)
            onPositionChanged: (mouse) => {
                if (pressed)
                    root.sliderValue = Math.max(0, Math.min(100, Math.round(mouse.x / width * 100)))
            }
        }
    }

    // 在 Window 上定义一个属性，让多个子元素都能引用
    property int sliderValue: 40

    // 数字显示：自动跟随 sliderValue
    Text {
        x: 324; y: 30
        text: root.sliderValue + " %"
        font.pixelSize: 20; font.bold: true; color: "#cdd6f4"
    }

    // 颜色也绑定到同一个值——三个地方都在用 sliderValue，改一处全部更新
    Rectangle {
        x: 16; y: 68
        width: 300; height: 30; radius: 6
        color: Qt.rgba(root.sliderValue / 100, 0.4, 1 - root.sliderValue / 100, 1)
        Text {
            anchors.centerIn: parent
            text: "颜色也跟着变"
            font.pixelSize: 13; color: "white"
        }
    }

    // ── property alias：把内部属性"透出"给外部 ───────────────

    Text {
        text: "property alias（组件透传）"
        color: "#6c7086"; font.pixelSize: 12; x: 16; y: 126
    }

    // 用 alias 把内部 Text 的 text / color 暴露出来
    Item {
        id: badge
        x: 16; y: 148
        width: badgeRect.width; height: badgeRect.height

        // 对外暴露两个属性，外部改这两个就等于改内部 Text
        property alias label: badgeText.text
        property alias badgeColor: badgeRect.color

        Rectangle {
            id: badgeRect
            width: badgeText.width + 24; height: 32; radius: 16; color: "#a6e3a1"
            Text {
                id: badgeText
                anchors.centerIn: parent
                text: "默认文字"; font.pixelSize: 14; color: "#1e1e2e"
            }
        }
    }

    // 外部只需要设 label 和 badgeColor，不用关心内部结构
    Component.onCompleted: {
        badge.label = "已连接 ✓"
        badge.badgeColor = "#a6e3a1"
    }

    // ── 用同一套 property 驱动多个组件 ───────────────────────

    Text {
        text: "同一属性驱动多个元素"
        color: "#6c7086"; font.pixelSize: 12; x: 16; y: 206
    }

    property bool isOnline: false

    Rectangle {
        x: 16; y: 228
        width: 16; height: 16; radius: 8
        color: root.isOnline ? "#a6e3a1" : "#f38ba8"
        Behavior on color { ColorAnimation { duration: 300 } }
    }

    Text {
        x: 40; y: 228
        text: root.isOnline ? "在线" : "离线"
        font.pixelSize: 14
        color: root.isOnline ? "#a6e3a1" : "#f38ba8"
        Behavior on color { ColorAnimation { duration: 300 } }
    }

    Rectangle {
        x: 120; y: 222
        width: 80; height: 28; radius: 6
        color: toggleArea.containsMouse ? "#585b70" : "#45475a"
        Text { anchors.centerIn: parent; text: "切换状态"; color: "#cdd6f4"; font.pixelSize: 12 }
        MouseArea {
            id: toggleArea
            anchors.fill: parent; hoverEnabled: true
            onClicked: root.isOnline = !root.isOnline
        }
    }

    // 第三个依赖同一属性的元素
    Rectangle {
        x: 16; y: 272
        width: 200; height: 60; radius: 8
        color: root.isOnline ? "#1e3a2f" : "#3a1e1e"
        Behavior on color { ColorAnimation { duration: 300 } }
        Text {
            anchors.centerIn: parent
            text: root.isOnline ? "设备已连接，正在接收数据…" : "等待连接…"
            font.pixelSize: 12; color: "#6c7086"; wrapMode: Text.Wrap
            width: parent.width - 16
        }
    }
}
