import QtQuick
import QtQuick.Controls
import Reelo

// A short message over the video ("Volume 80%", "Subtitles: English") that fades by itself.
Rectangle {
    id: osd
    function show(text) {
        label.text = text;
        osd.opacity = 1;
        hide.restart();
    }

    width: label.implicitWidth + 28
    height: label.implicitHeight + 14
    radius: height / 2
    color: Qt.alpha(Theme.scrim, 0.7)
    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 180 } }

    Label {
        id: label
        anchors.centerIn: parent
        color: Theme.onVideo
        font.family: Theme.fontUi
        font.pointSize: Theme.bodySize + 1
    }
    Timer {
        id: hide
        interval: 1400
        onTriggered: osd.opacity = 0
    }
}
