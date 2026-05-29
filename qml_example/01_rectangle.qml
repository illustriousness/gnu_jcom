// 示例 01：Rectangle 基础
// 知识点：窗口、矩形、颜色、圆角、边框

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 400
    height: 300
    visible: true
    title: "01 - Rectangle"

    // 窗口背景色
    color: "#f0f0f0"

    // 最基础的矩形
    Rectangle {
        width: 120
        height: 80
        color: "steelblue"   // 颜色可以用名字
    }

    // 带圆角的矩形
    Rectangle {
        x: 150                // x / y 是相对父对象左上角的坐标
        y: 20
        width: 120
        height: 80
        color: "#e74c3c"     // 也可以用十六进制
        radius: 16           // 圆角半径
    }

    // 只有边框、没有填充色的矩形
    Rectangle {
        x: 40
        y: 140
        width: 120
        height: 80
        color: "transparent"      // 透明背景
        border.color: "#2ecc71"   // 边框颜色
        border.width: 3           // 边框宽度
        radius: 8
    }

    // 小练习：再加一个矩形，放到右下角
}
