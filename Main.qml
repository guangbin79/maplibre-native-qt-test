/**
 * Main.qml — MapLibre 地图主界面
 *
 * 本文件定义了完整的地图交互界面，包含：
 *   1. MapLibre 地图视图（通过本地 GIS Server 加载矢量瓦片样式）
 *   2. 触摸手势支持（双指缩放、旋转、倾斜）
 *   3. 桌面端鼠标交互（右键旋转、中键倾斜、修饰键+滚轮）
 *   4. 左下角自适应比例尺（根据缩放级别自动调整刻度）
 *   5. 右上角控制面板（旋转/倾斜滑块）
 *
 * 数据流：
 *   HXGISServer (C++, 127.0.0.1:4943)  →  style.json  →  MapLibre 渲染
 */

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtLocation
import QtPositioning
import MapLibre 3.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("MapLibre Map")

    /* ── MapLibre 插件配置 ──────────────────────────────────────
     * 通过 Qt Location 的 Plugin 机制加载 MapLibre 后端。
     * maplibre.map.styles 指向本地 GIS Server 提供的样式端点，
     * schema=hxmap 表示使用 HXGIS 自定义瓦片方案。
     */
    Plugin {
        id: mapPlugin
        name: "maplibre"

        PluginParameter {
            name: "maplibre.map.styles"
            value: "http://127.0.0.1:4943/styles/day/style.json?schema=hxmap"
        }
    }

    /* ── 地图视图 ───────────────────────────────────────────────
     * MapView 是 MapLibre Qt 的主组件，包含 Map 和相关手势处理器。
     * 初始中心点设为 (36.75, 3.05)（阿尔及尔），缩放级别 8。
     */
    MapView {
        id: mapView
        anchors.fill: parent
        map.plugin: mapPlugin
        map.center: QtPositioning.coordinate(36.75, 3.05)
        map.zoomLevel: 8

        // MapLibre 样式容器（当前为空，样式由 style.json 端点驱动）
        MapLibre.style: Style {
        }

        // ── 触摸手势：双指缩放 / 旋转 / 倾斜 ─────────────────────
        //
        // PinchHandler 同时处理三种手势：
        //   - scale  → 缩放级别 (zoomLevel = startZoom + log₂(scale))
        //   - rotation → 地图旋转角度 (bearing)
        //   - 双指纵向位移 → 地图倾斜角度 (tilt, 范围 0°~60°)
        //
        // onScaleChanged 和 onRotationChanged 都同步更新 tilt，
        // 确保无论用户先触发哪种手势变化，倾斜都能即时响应。
        PinchHandler {
            id: pinchHandler
            target: null  // 不移动任何 Item，只读取手势参数

            property real startBearing: 0
            property real startTilt: 0
            property real startZoom: 0
            property real startCentroidY: 0  // 手势开始时双指中心的 Y 坐标

            onActiveChanged: {
                if (active) {
                    // 记录手势开始时的地图状态，作为后续计算的基准值
                    startBearing = mapView.map.bearing
                    startTilt = mapView.map.tilt
                    startZoom = mapView.map.zoomLevel
                    startCentroidY = centroid.position.y
                }
            }

            // 缩放：双指捏合 → zoomLevel = startZoom + log₂(scale)
            onScaleChanged: {
                if (active) {
                    var newZoom = startZoom + Math.log2(scale)
                    mapView.map.zoomLevel = newZoom
                    // 同步更新倾斜（scale 变化频繁，可覆盖大部分帧）
                    var dy = centroid.position.y - startCentroidY
                    mapView.map.tilt = Math.max(0, Math.min(60, startTilt + dy * 0.15))
                }
            }

            // 旋转：双指旋转角度
            onRotationChanged: {
                if (active) {
                    mapView.map.bearing = (startBearing + rotation + 360) % 360
                    // 同步更新倾斜（rotation 变化时补充更新）
                    var dy = centroid.position.y - startCentroidY
                    mapView.map.tilt = Math.max(0, Math.min(60, startTilt + dy * 0.15))
                }
            }
        }

        // ── 桌面端：右键旋转、中键倾斜 ─────────────────────────
        //
        // 旋转算法使用叉积判断旋转方向：
        //   向量1: 鼠标位置到屏幕中心 (rx, ry)
        //   向量2: 鼠标移动增量 (dx, dy)
        //   cross = dx * ry - dy * rx
        //   cross > 0 → 顺时针，cross < 0 → 逆时针
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton | Qt.MiddleButton

            property real lastX: 0
            property real lastY: 0

            onPressed: function(mouse) {
                if (mouse.button === Qt.RightButton || mouse.button === Qt.MiddleButton) {
                    lastX = mouse.x
                    lastY = mouse.y
                    mouse.accepted = true
                } else {
                    mouse.accepted = false
                }
            }

            onPositionChanged: function(mouse) {
                if (mouse.buttons & Qt.RightButton) {
                    // 叉积旋转：鼠标绕屏幕中心的切向移动量决定旋转方向和幅度
                    var cx = width / 2
                    var cy = height / 2
                    var rx = mouse.x - cx
                    var ry = mouse.y - cy
                    var dx = mouse.x - lastX
                    var dy = mouse.y - lastY
                    var cross = dx * ry - dy * rx
                    mapView.map.bearing += cross * 0.005
                    lastX = mouse.x
                    lastY = mouse.y
                    mouse.accepted = true
                } else if (mouse.buttons & Qt.MiddleButton) {
                    // 中键上下拖拽 → 倾斜角度 (0°~60°)
                    var dy = mouse.y - lastY
                    mapView.map.tilt = Math.max(0, Math.min(60, mapView.map.tilt + dy * 0.3))
                    lastX = mouse.x
                    lastY = mouse.y
                    mouse.accepted = true
                } else {
                    mouse.accepted = false
                }
            }

            onReleased: function(mouse) {
                mouse.accepted = (mouse.button === Qt.RightButton || mouse.button === Qt.MiddleButton)
            }
        }

        // ── 桌面端：Shift+滚轮旋转、Ctrl+滚轮倾斜 ─────────────
        WheelHandler {
            acceptedModifiers: Qt.ShiftModifier | Qt.ControlModifier
            onWheel: function(event) {
                if (event.modifiers & Qt.ShiftModifier) {
                    // Shift + 滚轮 → 旋转（每格约 1°，angleDelta 单位为 1/8°）
                    mapView.map.bearing += event.angleDelta.y / 15
                } else if (event.modifiers & Qt.ControlModifier) {
                    // Ctrl + 滚轮 → 倾斜（每格约 1°，范围 0°~60°）
                    mapView.map.tilt = Math.max(0, Math.min(60, mapView.map.tilt + event.angleDelta.y / 15))
                }
            }
        }
    }

    // ── 左下角比例尺 ──────────────────────────────────────────
    //
    // 自适应比例尺算法：
    //   1. 根据 Web Mercator 投影公式计算当前缩放级别下每像素代表的地面米数
    //   2. 从预定义的"美观"距离序列中选择最接近的整数值
    //   3. 将该距离转换为像素宽度绘制比例尺条
    //
    // Web Mercator 每像素米数公式：
    //   metersPerPixel = 156543.03392 * cos(lat°) / 2^zoom
    //   其中 156543.03392 是赤道处 zoom=0 时每像素对应的地面距离
    Item {
        id: scaleBar
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.bottomMargin: 24
        width: scaleBarRect.width
        height: scaleBarLabel.height + scaleBarRect.height

        // 每像素对应的地面米数（Web Mercator 公式）
        readonly property real metersPerPixel: {
            var lat = mapView.map.center.latitude
            var zoom = mapView.map.zoomLevel
            return 156543.03392 * Math.cos(lat * Math.PI / 180) / Math.pow(2, zoom)
        }

        // 预定义的"美观"距离序列（1m ~ 1000km）
        // 比例尺会从中选择最合适的值，使显示宽度在 50~150px 之间
        readonly property var niceDistances: [
            1, 2, 5, 10, 20, 50, 100, 200, 500,
            1000, 2000, 5000, 10000, 20000, 50000, 100000,
            200000, 500000, 1000000
        ]

        readonly property real targetWidth: 100          // 目标杆长 px
        readonly property real targetMeters: metersPerPixel * targetWidth

        // 从序列中选择第一个 >= 目标距离 40% 的值（确保比例尺不会太短）
        readonly property real niceDistance: {
            var best = niceDistances[niceDistances.length - 1]
            for (var i = 0; i < niceDistances.length; i++) {
                if (niceDistances[i] >= targetMeters * 0.4) {
                    best = niceDistances[i]
                    break
                }
            }
            return best
        }

        // 将选中的美观距离转换为实际像素宽度
        readonly property real barWidth: niceDistance / metersPerPixel

        // 格式化距离文本（>=1km 用 km，否则用 m）
        readonly property string distanceText: {
            if (niceDistance >= 1000)
                return (niceDistance / 1000) + " km"
            else
                return niceDistance + " m"
        }

        // 距离标签（如 "5 km"）
        Label {
            id: scaleBarLabel
            anchors.left: parent.left
            anchors.top: parent.top
            text: scaleBar.distanceText
            color: "black"
            font.pixelSize: 11
            font.bold: true
            leftPadding: 0
        }

        // 比例尺主体：横线 + 两端竖线
        Rectangle {
            id: scaleBarRect
            anchors.left: parent.left
            anchors.top: scaleBarLabel.bottom
            anchors.topMargin: 2
            width: scaleBar.barWidth
            height: 4
            radius: 1
            color: "black"

            // 两端竖线
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                width: 2
                height: parent.height + 4
                anchors.topMargin: -2
                color: "black"
            }
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                width: 2
                height: parent.height + 4
                anchors.topMargin: -2
                color: "black"
            }
        }
    }

    /* ── 右上角控制面板 ─────────────────────────────────────────
     * 半透明浮动面板，包含旋转 (bearing) 和倾斜 (tilt) 两个垂直滑块。
     * 滑块值与地图属性双向绑定：
     *   - 显示当前值（Label 实时更新）
     *   - 用户拖拽滑块时实时改变地图状态 (onMoved)
     *   - 地图手势改变状态时滑块自动跟随 (value: mapView.map.bearing/tilt)
     */
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        width: 70
        height: parent.height / 3 + 60
        color: "#80000000"
        radius: 8

        Column {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            // 旋转角度标签
            Label {
                text: qsTr("旋转 %1°").arg(Math.round(mapView.map.bearing))
                color: "white"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
            }

            // 旋转滑块 (0° ~ 360°)
            Slider {
                id: bearingSlider
                width: parent.width
                from: 0
                to: 360
                value: mapView.map.bearing  // 地图状态 → 滑块位置
                orientation: Qt.Vertical
                height: (parent.height - 60) / 2 - 10

                onMoved: mapView.map.bearing = value  // 滑块拖拽 → 地图状态

                ToolTip.visible: pressed
                ToolTip.text: qsTr("旋转: %1°").arg(Math.round(value))
            }

            // 倾斜角度标签
            Label {
                text: qsTr("倾斜 %1°").arg(Math.round(mapView.map.tilt))
                color: "white"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
            }

            // 倾斜滑块 (0° ~ 60°)
            Slider {
                id: tiltSlider
                width: parent.width
                from: 0
                to: 60
                value: mapView.map.tilt  // 地图状态 → 滑块位置
                orientation: Qt.Vertical
                height: (parent.height - 60) / 2 - 10

                onMoved: mapView.map.tilt = value  // 滑块拖拽 → 地图状态

                ToolTip.visible: pressed
                ToolTip.text: qsTr("倾斜: %1°").arg(Math.round(value))
            }
        }
    }
}
