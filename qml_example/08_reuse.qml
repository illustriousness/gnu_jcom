// 示例 08：QML 的复用方式
// 对应 C 语言的：#define / 函数 / 结构体复用
//
// QML 有三种主要复用方式：
//   1. property  → 相当于常量 #define
//   2. component → 相当于"模板"，在同一文件内复用
//   3. 单独 .qml 文件 → 相当于独立组件（最推荐，下一个示例讲）

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 500
    height: 500
    visible: true
    title: "08 - 复用"
    color: "#1e1e2e"

    // ─────────────────────────────────────────────────────────
    // 方式一：property 定义常量
    // 相当于 C 的 #define RED  0xf38ba8
    //              #define RADIUS 8
    // ─────────────────────────────────────────────────────────

    readonly property color cRed:    "#f38ba8"
    readonly property color cGreen:  "#a6e3a1"
    readonly property color cBlue:   "#89b4fa"
    readonly property color cYellow: "#f9e2af"
    readonly property color cBg:     "#313244"
    readonly property int   kRadius: 8
    readonly property int   kPad:    16

    // 用常量：改一处（上面的 property），所有用到的地方全部更新
    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: kPad
        spacing: 10

        Rectangle { width: 60; height: 60; radius: kRadius; color: cRed   }
        Rectangle { width: 60; height: 60; radius: kRadius; color: cGreen }
        Rectangle { width: 60; height: 60; radius: kRadius; color: cBlue  }
        Rectangle { width: 60; height: 60; radius: kRadius; color: cYellow}
    }

    // ─────────────────────────────────────────────────────────
    // 方式二：component 内联组件
    // 相当于 C 的 struct / 函数——定义一次，重复使用
    // ─────────────────────────────────────────────────────────

    // 定义一个"标签徽章"组件，有两个可配置属性
    component Badge: Rectangle {
        property string label: "默认"
        property color  bgColor: cBlue

        width:  badgeText.width + 20
        height: 28
        radius: 14          // 完全圆角（胶囊形）
        color:  bgColor

        Text {
            id: badgeText
            anchors.centerIn: parent
            text:           parent.label
            font.pixelSize: 13
            color:          "#1e1e2e"
        }
    }

    // 用 Badge 组件：像 HTML 标签一样直接写
    Row {
        anchors.top: parent.top
        anchors.topMargin: 100
        anchors.left: parent.left
        anchors.leftMargin: kPad
        spacing: 8

        Badge { label: "已连接";  bgColor: cGreen  }
        Badge { label: "115200";  bgColor: cBlue   }
        Badge { label: "8N1";     bgColor: cYellow }
        Badge { label: "错误";    bgColor: cRed    }
    }

    // ─────────────────────────────────────────────────────────
    // 方式三：component 做"卡片"布局，内部可以放任意内容
    // ─────────────────────────────────────────────────────────

    component Card: Rectangle {
        property string title: "标题"
        // default property：让外部直接在 Card {} 里写子元素
        default property alias content: body.data

        width:  220
        height: 120
        radius: kRadius
        color:  cBg

        // 卡片标题栏
        Rectangle {
            id: titleBar
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 32
            color:  Qt.darker(parent.color, 1.3)
            radius: kRadius
            // 只让上面两个角有圆角（遮住下面两角）
            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: kRadius; color: parent.color
            }
            Text {
                anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                text: parent.parent.title
                font.pixelSize: 13; color: "#cdd6f4"
            }
        }

        // 卡片内容区：外部塞进来的子元素放在这里
        Item {
            id: body
            anchors { top: titleBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.margins: 10
        }
    }

    // 使用 Card，直接在里面放内容
    Row {
        anchors.top: parent.top
        anchors.topMargin: 160
        anchors.left: parent.left
        anchors.leftMargin: kPad
        spacing: 12

        Card {
            title: "串口状态"
            Column {
                spacing: 6
                Row { spacing: 6
                    Rectangle { width: 8; height: 8; radius: 4; color: cGreen; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "/dev/ttyUSB0"; color: "#cdd6f4"; font.pixelSize: 12 }
                }
                Text { text: "波特率：115200"; color: "#6c7086"; font.pixelSize: 12 }
                Text { text: "已运行：00:03:21"; color: "#6c7086"; font.pixelSize: 12 }
            }
        }

        Card {
            title: "数据统计"
            Column {
                spacing: 6
                Text { text: "TX：1,024 字节"; color: "#cdd6f4"; font.pixelSize: 12 }
                Text { text: "RX：8,192 字节"; color: "#cdd6f4"; font.pixelSize: 12 }
                Text { text: "错误：0";        color: cGreen;    font.pixelSize: 12 }
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 方式四：JavaScript 函数复用逻辑
    // 相当于 C 的工具函数
    // ─────────────────────────────────────────────────────────

    // 定义一个格式化字节数的函数
    function formatBytes(n) {
        if (n < 1024)     return n + " B"
        if (n < 1048576)  return (n / 1024).toFixed(1) + " KB"
        return (n / 1048576).toFixed(2) + " MB"
    }

    // 定义一个根据值返回颜色的函数
    function levelColor(value) {
        if (value > 80) return cRed
        if (value > 50) return cYellow
        return cGreen
    }

    Row {
        anchors.top: parent.top
        anchors.topMargin: 320
        anchors.left: parent.left
        anchors.leftMargin: kPad
        spacing: 20

        // 用 formatBytes 函数
        Column {
            spacing: 4
            Text { text: "函数复用示例";  color: "#6c7086"; font.pixelSize: 11 }
            Text { text: formatBytes(512);     color: "#cdd6f4"; font.pixelSize: 14 }
            Text { text: formatBytes(204800);  color: "#cdd6f4"; font.pixelSize: 14 }
            Text { text: formatBytes(3145728); color: "#cdd6f4"; font.pixelSize: 14 }
        }

        // 用 levelColor 函数
        Column {
            spacing: 8
            Text { text: "颜色函数示例"; color: "#6c7086"; font.pixelSize: 11 }
            Repeater {
                model: [20, 60, 90]   // 三个不同的值
                Row {
                    spacing: 8
                    required property var modelData
                    Rectangle {
                        width: 12; height: 12; radius: 6
                        color: levelColor(modelData)        // 调用函数
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: modelData + "%"
                        color: levelColor(modelData)        // 同一个函数用两次
                        font.pixelSize: 13
                    }
                }
            }
        }
    }
}
