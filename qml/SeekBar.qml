import QtQuick
import QtQuick.Controls
import "logic.js" as Logic

// The timeline: played part in the accent colour, buffered part lighter, and the time under the
// pointer while hovering. Dragging seeks once on release (and live every 150 ms while dragging).
Item {
    id: bar
    property real position: 0
    property real duration: 0
    property real cached: 0
    property bool seekable: true
    property color tint: Theme.onVideo
    signal seek(real seconds)

    readonly property real shown: drag.pressed ? dragTo : position
    property real dragTo: 0

    implicitHeight: 24
    enabled: seekable && duration > 0
    Accessible.role: Accessible.Slider
    Accessible.name: qsTr("Position")

    function at(x) {
        return Math.max(0, Math.min(1, x / track.width)) * duration;
    }

    Rectangle {
        id: track
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: drag.containsMouse || drag.pressed ? 6 : 4
        radius: height / 2
        color: Qt.alpha(bar.tint, 0.25)
        Behavior on height { NumberAnimation { duration: 100 } }

        Rectangle {
            height: parent.height
            radius: parent.radius
            color: Qt.alpha(bar.tint, 0.35)
            width: bar.duration > 0 ? parent.width * Math.min(1, bar.cached / bar.duration) : 0
        }
        Rectangle {
            height: parent.height
            radius: parent.radius
            color: Theme.accent
            width: bar.duration > 0 ? parent.width * Math.min(1, bar.shown / bar.duration) : 0
        }
    }
    Rectangle {
        id: knob
        width: 14
        height: 14
        radius: 7
        color: Theme.accent
        border.color: Qt.alpha("#000000", 0.25)
        anchors.verticalCenter: parent.verticalCenter
        x: (bar.duration > 0 ? track.width * Math.min(1, bar.shown / bar.duration) : 0) - width / 2
        visible: bar.enabled
        scale: drag.containsMouse || drag.pressed ? 1 : 0
        Behavior on scale { NumberAnimation { duration: 100 } }
    }

    // The time under the pointer.
    Rectangle {
        visible: drag.containsMouse && bar.enabled
        y: -height - 6
        x: Math.max(0, Math.min(bar.width - width, drag.mouseX - width / 2))
        width: hoverLabel.implicitWidth + 12
        height: hoverLabel.implicitHeight + 6
        radius: 4
        color: Qt.alpha(Theme.scrim, 0.75)
        Label {
            id: hoverLabel
            anchors.centerIn: parent
            text: Logic.formatTime(bar.at(drag.mouseX))
            color: Theme.onVideo
            font.family: Theme.fontUi
            font.pointSize: Theme.smallSize
            font.features: { "tnum": 1 }
        }
    }

    Timer {
        id: live
        interval: 150
        repeat: true
        running: drag.pressed
        onTriggered: bar.seek(bar.dragTo)
    }

    MouseArea {
        id: drag
        anchors.fill: parent
        anchors.topMargin: -6
        anchors.bottomMargin: -6
        hoverEnabled: true
        cursorShape: bar.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onPressed: mouse => bar.dragTo = bar.at(mouse.x)
        onPositionChanged: mouse => { if (pressed) bar.dragTo = bar.at(mouse.x); }
        onReleased: bar.seek(bar.dragTo)
    }
}
