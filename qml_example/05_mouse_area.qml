// 示例 05：MouseArea 鼠标交互
// 知识点：点击、悬停、按下状态，property 属性

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 440
    height: 340
    visible: true
    title: "05 - MouseArea"
    color: "#1e1e2e"

    // ── 点击改变颜色 ──────────────────────────────────────────

    Rectangle {
        id: clickBox
        x: 20; y: 20
        width: 120; height: 60; radius: 8

        // property：自定义属性，clicked 为 false 时显示蓝色，true 时显示绿色
        property bool clicked: false
        color: clicked ? "#a6e3a1" : "#89b4fa"

        Text {
            anchors.centerIn: parent
            text: clickBox.clicked ? "已点击 ✓" : "点我"
            color: "#1e1e2e"; font.pixelSize: 14
        }

        MouseArea {
            anchors.fill: parent
            // onClicked：鼠标点击时触发
            onClicked: clickBox.clicked = !clickBox.clicked
        }
    }

    // ── 悬停高亮（hover） ─────────────────────────────────────

    Rectangle {
        x: 160; y: 20
        width: 120; height: 60; radius: 8
        // hoverBox.color 取决于 hoverArea.containsMouse
        color: hoverArea.containsMouse ? "#f38ba8" : "#45475a"

        Text {
            anchors.centerIn: parent
            text: "悬停变色"
            color: "white"; font.pixelSize: 14
        }

        MouseArea {
            id: hoverArea
            anchors.fill: parent
            hoverEnabled: true      // 必须设为 true 才能检测 hover
        }
    }

    // ── 按下 / 松开 ───────────────────────────────────────────

    Rectangle {
        x: 300; y: 20
        width: 120; height: 60; radius: 8
        color: pressArea.pressed ? "#f9e2af" : "#585b70"
        // pressed 时缩小一点（scale < 1）
        scale: pressArea.pressed ? 0.93 : 1.0

        Text {
            anchors.centerIn: parent
            text: pressArea.pressed ? "按下中…" : "按住我"
            color: "#1e1e2e"; font.pixelSize: 14
        }

        MouseArea {
            id: pressArea
            anchors.fill: parent
            hoverEnabled: true
        }
    }

    // ── 计数器 ────────────────────────────────────────────────

    Rectangle {
        x: 20; y: 120
        width: 260; height: 70; radius: 8; color: "#313244"

        // 在 Rectangle 上定义一个计数属性
        property int count: 0

        Row {
            anchors.centerIn: parent
            spacing: 16

            // 减号按钮
            Rectangle {
                width: 40; height: 40; radius: 6
                color: minusArea.containsMouse ? "#f38ba8" : "#45475a"
                Text { anchors.centerIn: parent; text: "−"; font.pixelSize: 20; color: "white" }
                MouseArea {
                    id: minusArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: parent.parent.parent.parent.count -= 1   // 向上访问 Rectangle 的 count
                }
            }

            // 数字显示
            Text {
                width: 60
                text: parent.parent.parent.count
                font.pixelSize: 28; color: "white"
                horizontalAlignment: Text.AlignHCenter
                anchors.verticalCenter: parent.verticalCenter
            }

            // 加号按钮
            Rectangle {
                width: 40; height: 40; radius: 6
                color: plusArea.containsMouse ? "#a6e3a1" : "#45475a"
                Text { anchors.centerIn: parent; text: "+"; font.pixelSize: 20; color: "white" }
                MouseArea {
                    id: plusArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: parent.parent.parent.parent.count += 1
                }
            }
        }
    }

    // ── 鼠标坐标跟踪 ──────────────────────────────────────────

    Rectangle {
        x: 20; y: 220
        width: 400; height: 100; radius: 8; color: "#181825"

        Text {
            id: posText
            anchors.centerIn: parent
            text: "在此区域移动鼠标"
            font.pixelSize: 14; color: "#585b70"
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            // onPositionChanged：鼠标移动时触发，mouse 是事件对象
            onPositionChanged: (mouse) => {
                posText.text = "x: " + Math.round(mouse.x) + "  y: " + Math.round(mouse.y)
            }
            onExited: posText.text = "在此区域移动鼠标"
        }
    }
}
