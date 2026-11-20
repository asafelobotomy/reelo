import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl as Impl
import QtQuick.Layouts
import Reelo

// What shows for music without cover art: the title and artist on the theme's background.
Rectangle {
    id: view
    property string title
    property string artist
    property string album
    color: Theme.bg

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 560)
        spacing: 10

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10
            width: 168
            height: 168
            radius: Theme.radius * 2
            color: Theme.surface
            border.color: Theme.border
            border.width: 1
            Impl.IconImage {
                anchors.centerIn: parent
                source: "qrc:/qt/qml/Reelo/icons/music.svg"
                sourceSize: Qt.size(84, 84)
                color: Theme.accent
            }
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: view.title
            color: Theme.text
            elide: Text.ElideRight
            font.family: Theme.fontUi
            font.pointSize: Theme.headingSize
            font.weight: Font.DemiBold
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            visible: text.length > 0
            text: [view.artist, view.album].filter(s => s && s.length).join("  ·  ")
            color: Theme.textMuted
            elide: Text.ElideRight
            font.family: Theme.fontUi
            font.pointSize: Theme.bodySize + 1
        }
    }
}
