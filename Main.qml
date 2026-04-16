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

    Plugin {
        id: mapPlugin
        name: "maplibre"

        PluginParameter {
            name: "maplibre.map.styles"
            value: "http://127.0.0.1:4943/styles/day/style.json?schema=hxmap"
        }
    }

    MapView {
        id: mapView
        anchors.fill: parent
        map.plugin: mapPlugin
        map.center: QtPositioning.coordinate(36.75, 3.05)
        map.zoomLevel: 8

        MapLibre.style: Style {
        }

        // ── 触摸手势：双指缩放 / 旋转 / 倾斜 ─────────────────────
        PinchHandler {
            id: pinchHandler
            target: null

            property real startBearing: 0
            property real startTilt: 0
            property real startZoom: 0
            property real startCentroidY: 0

            onActiveChanged: {
                if (active) {
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
                    mapView.map.bearing += event.angleDelta.y / 15
                } else if (event.modifiers & Qt.ControlModifier) {
                    mapView.map.tilt = Math.max(0, Math.min(60, mapView.map.tilt + event.angleDelta.y / 15))
                }
            }
        }
    }

    // ── 左下角比例尺 ──────────────────────────────────────────
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

        // 取一个"好看"的整数值，使比例尺宽度在 50~150 px 之间
        readonly property var niceDistances: [
            1, 2, 5, 10, 20, 50, 100, 200, 500,
            1000, 2000, 5000, 10000, 20000, 50000, 100000,
            200000, 500000, 1000000
        ]

        readonly property real targetWidth: 100          // 目标杆长 px
        readonly property real targetMeters: metersPerPixel * targetWidth

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

        readonly property real barWidth: niceDistance / metersPerPixel
        readonly property string distanceText: {
            if (niceDistance >= 1000)
                return (niceDistance / 1000) + " km"
            else
                return niceDistance + " m"
        }

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

            Label {
                text: qsTr("旋转 %1°").arg(Math.round(mapView.map.bearing))
                color: "white"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
            }

            Slider {
                id: bearingSlider
                width: parent.width
                from: 0
                to: 360
                value: mapView.map.bearing
                orientation: Qt.Vertical
                height: (parent.height - 60) / 2 - 10

                onMoved: mapView.map.bearing = value

                ToolTip.visible: pressed
                ToolTip.text: qsTr("旋转: %1°").arg(Math.round(value))
            }

            Label {
                text: qsTr("倾斜 %1°").arg(Math.round(mapView.map.tilt))
                color: "white"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
            }

            Slider {
                id: tiltSlider
                width: parent.width
                from: 0
                to: 60
                value: mapView.map.tilt
                orientation: Qt.Vertical
                height: (parent.height - 60) / 2 - 10

                onMoved: mapView.map.tilt = value

                ToolTip.visible: pressed
                ToolTip.text: qsTr("倾斜: %1°").arg(Math.round(value))
            }
        }
    }
}
