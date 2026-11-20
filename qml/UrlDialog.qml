import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Reelo

// Asks for a stream or video page address (YouTube and the like play through yt-dlp).
Popup {
    id: dialog
    signal accepted(string url)
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(520, parent ? parent.width - 40 : 520)
    padding: 18
    onOpened: { field.text = ""; field.forceActiveFocus(); }

    background: Rectangle {
        color: Theme.surface
        radius: Theme.radius
        border.color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label {
            text: qsTr("Play from an address")
            color: Theme.text
            font.family: Theme.fontUi
            font.pointSize: Theme.bodySize + 2
            font.weight: Font.DemiBold
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("A stream (https://…/live.m3u8) or a video page; pages need yt-dlp installed.")
            color: Theme.textMuted
            font.family: Theme.fontUi
            font.pointSize: Theme.smallSize
        }
        TextField {
            id: field
            Layout.fillWidth: true
            placeholderText: "https://"
            color: Theme.text
            font.family: Theme.fontUi
            selectByMouse: true
            background: Rectangle {
                color: Theme.bg
                radius: Theme.radius - 2
                border.color: field.activeFocus ? Theme.accent : Theme.border
            }
            onAccepted: go.clicked()
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            PillButton {
                text: qsTr("Cancel")
                onClicked: dialog.close()
            }
            PillButton {
                id: go
                text: qsTr("Play")
                primary: true
                enabled: /^(https?|rtsp|rtmp):\/\/\S+$/.test(field.text.trim())
                onClicked: {
                    dialog.accepted(field.text.trim());
                    dialog.close();
                }
            }
        }
    }
}
