// 示例 02：Text 文字
// 知识点：Text 的字体、大小、颜色、对齐、换行

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 400
    height: 320
    visible: true
    title: "02 - Text"
    color: "#1e1e2e"

    // 最简单的文字
    Text {
        x: 20; y: 20
        text: "Hello, QML!"
        font.pixelSize: 24      // 字号（像素）
        color: "white"
    }

    // 加粗 + 斜体
    Text {
        x: 20; y: 70
        text: "加粗 + 斜体"
        font.pixelSize: 20
        font.bold: true
        font.italic: true
        color: "#f9e2af"
    }

    // 字间距
    Text {
        x: 20; y: 115
        text: "字  间  距"
        font.pixelSize: 20
        font.letterSpacing: 6   // 字母/字符间距（像素）
        color: "#cba6f7"
    }

    // 固定宽度 + 自动换行
    Text {
        x: 20; y: 160
        width: 200              // 限制宽度后会自动换行
        text: "这是一段很长很长很长的文字，超出宽度后会自动换行显示。"
        font.pixelSize: 14
        color: "#a6e3a1"
        wrapMode: Text.Wrap     // 换行模式
    }

    // 超出显示省略号
    Text {
        x: 20; y: 270
        width: 200
        text: "这段文字只显示一行，超出部分用省略号代替..."
        font.pixelSize: 14
        color: "#89dceb"
        elide: Text.ElideRight  // 超出时在末尾加 ...
    }
}
