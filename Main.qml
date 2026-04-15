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
            value: "https://demotiles.maplibre.org/style.json"
        }
    }

    MapView {
        id: mapView
        anchors.fill: parent
        map.plugin: mapPlugin
        map.center: QtPositioning.coordinate(39.9042, 116.4074)
        map.zoomLevel: 1

        MapLibre.style: Style {
        }

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
                value: 0
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
                value: 0
                orientation: Qt.Vertical
                height: (parent.height - 60) / 2 - 10

                onMoved: mapView.map.tilt = value

                ToolTip.visible: pressed
                ToolTip.text: qsTr("倾斜: %1°").arg(Math.round(value))
            }
        }
    }
}
