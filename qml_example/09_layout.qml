// 示例 09：不手动计算位置的布局方式
// 核心思想：让 QML 自动算坐标，你只管描述"关系"

import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 560
    height: 620
    visible: true
    title: "09 - 相对布局"
    color: "#1e1e2e"

    // ─────────────────────────────────────────────────────────
    // 1. Row / Column：最简单，子元素自动排列，完全不用写 x/y
    // ─────────────────────────────────────────────────────────

    SectionTitle { y: 12; text: "① Row / Column：自动排列" }

    Row {
        y: 36; x: 16
        spacing: 8   // 只需要设间距，位置全自动

        Repeater {
            model: 5
            Rectangle {
                width: 60; height: 40; radius: 6
                color: Qt.hsla(index / 5, 0.6, 0.6, 1)
                Text { anchors.centerIn: parent; text: index; color: "white"; font.pixelSize: 14 }
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 2. anchors：相对某个元素的边/中心对齐
    //    "我的左边 = 它的右边 + 8px 间距"
    // ─────────────────────────────────────────────────────────

    SectionTitle { y: 100; text: "② anchors：相对某元素对齐" }

    // 参考元素（左边那个）
    Rectangle {
        id: refBox
        x: 16; y: 124
        width: 80; height: 60; radius: 6; color: "#45475a"
        Text { anchors.centerIn: parent; text: "参考"; color: "#cdd6f4"; font.pixelSize: 13 }
    }

    // "我的左边 = refBox 的右边 + 12px"
    Rectangle {
        id: box2
        anchors.left:           refBox.right
        anchors.leftMargin:     12
        anchors.verticalCenter: refBox.verticalCenter   // 垂直与 refBox 对齐
        width: 80; height: 60; radius: 6; color: "#89b4fa"
        Text { anchors.centerIn: parent; text: "紧跟"; color: "#1e1e2e"; font.pixelSize: 13 }
    }

    // "我的左边 = box2 的右边 + 12px"，链式追加
    Rectangle {
        anchors.left:           box2.right
        anchors.leftMargin:     12
        anchors.verticalCenter: refBox.verticalCenter
        width: 80; height: 60; radius: 6; color: "#a6e3a1"
        Text { anchors.centerIn: parent; text: "再跟"; color: "#1e1e2e"; font.pixelSize: 13 }
    }

    // ─────────────────────────────────────────────────────────
    // 3. anchors.fill + margins：撑满父容器并留边距
    //    不需要算 width/height，自动跟随父容器尺寸变化
    // ─────────────────────────────────────────────────────────

    SectionTitle { y: 210; text: "③ fill + margins：撑满容器" }

    Rectangle {
        x: 16; y: 234
        width: 300; height: 80; radius: 8; color: "#313244"

        // 子元素撑满父容器，四边各留 12px
        Rectangle {
            anchors.fill: parent
            anchors.margins: 12
            radius: 6; color: "#89b4fa"
            Text { anchors.centerIn: parent; text: "自动撑满（留 12px 边距）"; color: "#1e1e2e"; font.pixelSize: 13 }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 4. Grid：二维网格，自动换行，不用算行列坐标
    // ─────────────────────────────────────────────────────────

    SectionTitle { y: 338; text: "④ Grid：自动网格" }

    Grid {
        x: 16; y: 362
        columns: 4      // 每行放 4 个，自动换行
        spacing: 8

        Repeater {
            model: 8
            Rectangle {
                width: 56; height: 40; radius: 6
                color: index % 2 === 0 ? "#45475a" : "#585b70"
                Text { anchors.centerIn: parent; text: "G" + index; color: "#cdd6f4"; font.pixelSize: 13 }
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 5. Flow：像文字一样自动换行，宽度不够就换到下一行
    // ─────────────────────────────────────────────────────────

    SectionTitle { y: 464; text: "⑤ Flow：自动换行" }

    Flow {
        x: 16; y: 488
        width: 400      // 只需要设宽度，超出自动换行
        spacing: 6

        Repeater {
            model: ["串口设置", "波特率", "数据位", "停止位", "校验位", "流控", "发送", "接收", "解析", "日志"]
            Rectangle {
                width:  tag.width + 16
                height: 26; radius: 13
                color: "#313244"
                Text {
                    id: tag
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 12; color: "#cdd6f4"
                }
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 辅助组件：章节标题
    // ─────────────────────────────────────────────────────────
    component SectionTitle: Text {
        x: 16
        font.pixelSize: 12; color: "#6c7086"
    }
}
