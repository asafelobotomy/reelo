import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl as Impl
import QtQuick.Layouts
import Reelo
import "logic.js" as Logic

// The start screen: open a file or an address, or pick from the newest downloads.
Rectangle {
    id: home
    signal openFiles()
    signal openUrl()
    color: Theme.bg

    property real now: Date.now()
    Timer { interval: 60000; running: home.visible; repeat: true; onTriggered: home.now = Date.now() }
    onVisibleChanged: if (visible) { now = Date.now(); App.refreshLibrary(); }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        anchors.topMargin: 24
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Image {
                source: "qrc:/qt/qml/Reelo/icons/reelo.svg"
                sourceSize: Qt.size(40, 40)
            }
            Label {
                text: "reelo"
                color: Theme.text
                font.family: Theme.fontUi
                font.pointSize: Theme.headingSize
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            PillButton {
                text: qsTr("Make default player")
                visible: !App.isDefault
                onClicked: App.makeDefault()
                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: qsTr("Open videos and music with reelo")
            }
        }

        RowLayout {
            spacing: 10
            PillButton {
                text: qsTr("Open files…")
                primary: true
                iconName: "open"
                onClicked: home.openFiles()
            }
            PillButton {
                text: qsTr("Open address…")
                iconName: "link"
                onClicked: home.openUrl()
            }
            Label {
                Layout.leftMargin: 6
                text: qsTr("or drop files here")
                color: Theme.textMuted
                font.family: Theme.fontUi
                font.pointSize: Theme.smallSize
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Label {
                text: qsTr("Newest in %1").arg(App.libraryFolder)
                color: Theme.text
                font.family: Theme.fontUi
                font.pointSize: Theme.bodySize + 1
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                elide: Text.ElideMiddle
            }
            PillButton {
                text: qsTr("Play all")
                iconName: "play"
                visible: App.library.length > 1
                onClicked: App.open(App.library.map(i => i.path).reverse())
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            radius: Theme.radius
            border.color: Qt.alpha(Theme.border, 0.6)

            ListView {
                id: list
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: App.library
                spacing: 2
                ScrollBar.vertical: ScrollBar {}

                delegate: ItemDelegate {
                    id: row
                    required property var modelData
                    width: ListView.view.width
                    height: 52
                    hoverEnabled: true
                    onClicked: App.open([modelData.path])
                    Accessible.name: modelData.name

                    background: Rectangle {
                        radius: Theme.radius - 3
                        color: row.hovered ? Theme.surfaceAlt : "transparent"
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }
                    contentItem: RowLayout {
                        spacing: 12
                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: Theme.radius - 3
                            color: Qt.alpha(Theme.accent, 0.16)
                            Impl.IconImage {
                                anchors.centerIn: parent
                                source: "qrc:/qt/qml/Reelo/icons/" + (row.modelData.kind === "audio" ? "music" : "video") + ".svg"
                                sourceSize: Qt.size(20, 20)
                                color: Theme.accent
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Label {
                                Layout.fillWidth: true
                                text: row.modelData.name
                                elide: Text.ElideRight
                                color: Theme.text
                                font.family: Theme.fontUi
                                font.pointSize: Theme.bodySize
                            }
                            Label {
                                Layout.fillWidth: true
                                text: [row.modelData.folder, Logic.formatSize(row.modelData.size),
                                       Logic.ago(row.modelData.modified, home.now)].filter(s => s).join("  ·  ")
                                elide: Text.ElideRight
                                color: Theme.textMuted
                                font.family: Theme.fontUi
                                font.pointSize: Theme.smallSize
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                visible: App.library.length === 0
                spacing: 6
                width: parent.width - 48
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("No videos or music here yet")
                    color: Theme.text
                    font.family: Theme.fontUi
                    font.pointSize: Theme.bodySize + 1
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("What AV Morning Star and hauler download shows up here. The folder is set in uQ Settings › Apps › Media player, or with: reelo config set library ~/Videos")
                    color: Theme.textMuted
                    font.family: Theme.fontUi
                    font.pointSize: Theme.smallSize
                }
            }
        }
    }
}
