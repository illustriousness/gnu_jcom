# QML 学习示例

每个文件都是独立可运行的示例，把文件内容复制到 `qml/Main.qml` 即可看效果。

## 学习顺序

| 文件 | 知识点 |
|------|--------|
| `01_rectangle.qml` | Rectangle：颜色、圆角、边框 |
| `02_text.qml` | Text：字体、大小、换行、省略号 |
| `03_anchors.qml` | anchors：自适应布局，贴边、居中、互相锚定 |
| `04_row_column.qml` | Row / Column / Repeater：自动排列 |
| `05_mouse_area.qml` | MouseArea：点击、悬停、按下、坐标跟踪 |
| `06_animation.qml` | Animation：Behavior、NumberAnimation、SequentialAnimation |
| `07_property_binding.qml` | property / alias / 数据绑定 |

## 使用方法

```bash
# 1. 复制某个示例到 Main.qml
cp qml_example/01_rectangle.qml qml/Main.qml

# 2. 重新构建并运行
cmake --build build && ./build/qcom
```

## 建议学习路径

1. 先跑 01～03，理解"怎么摆东西"
2. 跑 04，理解"怎么自动排列"
3. 跑 05，理解"怎么响应鼠标"
4. 跑 06，加上动画让界面更生动
5. 跑 07，理解"数据驱动 UI"——这是 QML 最重要的思想
