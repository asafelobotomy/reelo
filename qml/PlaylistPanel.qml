import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Reelo

// The queue, beside the video: click to play, × to take out.
Rectangle {
    id: panel
    signal closeRequested()
    color: Qt.alpha(Theme.surface, 0.96)
    border.color: Theme.border
    border.width: 1
    radius: Theme.radius

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Queue") + "  ·  " + App.playlist.count
                color: Theme.text
                font.family: Theme.fontUi
                font.pointSize: Theme.bodySize
                font.weight: Font.DemiBold
            }
            IconButton {
                iconName: "close"
                tip: qsTr("Close the queue")
                size: 30
                onClicked: panel.closeRequested()
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: App.playlist
            spacing: 2
            currentIndex: App.playlist.currentIndex
            ScrollBar.vertical: ScrollBar {}
            Component.onCompleted: positionViewAtIndex(currentIndex, ListView.Center)

            delegate: ItemDelegate {
                id: row
                required property int index
                required property string title
                required property string kind
                required property bool isCurrent
                width: ListView.view.width
                height: 40
                hoverEnabled: true
                onClicked: App.playAt(index)
                Accessible.name: title

                background: Rectangle {
                    radius: Theme.radiusControl
                    color: row.isCurrent ? Qt.alpha(Theme.accent, 0.22)
                         : row.hovered ? Theme.surfaceAlt : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 8
                    Label {
                        text: row.index + 1
                        color: row.isCurrent ? Theme.accent : Theme.textMuted
                        font.family: Theme.fontUi
                        font.pointSize: Theme.smallSize
                        font.features: { "tnum": 1 }
                        Layout.preferredWidth: 22
                        horizontalAlignment: Text.AlignRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: row.title
                        elide: Text.ElideMiddle
                        color: row.isCurrent ? Theme.accent : Theme.text
                        font.family: Theme.fontUi
                        font.pointSize: Theme.bodySize
                        font.weight: row.isCurrent ? Font.DemiBold : Font.Normal
                    }
                    IconButton {
                        iconName: "close"
                        tip: qsTr("Take out of the queue")
                        size: 26
                        visible: row.hovered
                        onClicked: App.playlist.remove(row.index)
                    }
                }
            }
        }
    }
}
