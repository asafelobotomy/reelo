import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Reelo
import "logic.js" as Logic

// The bar over the bottom of the video: timeline, then transport, volume, tracks and view.
Item {
    id: controls
    required property MpvObject player
    property bool playlistOpen: false
    signal togglePlaylist()
    signal toggleFullscreen()

    implicitHeight: column.implicitHeight + 24

    // Fades the video under the controls so they read on any picture.
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: -40
        gradient: Gradient {
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: 1; color: Qt.alpha(Theme.scrim, 0.78) }
        }
    }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        anchors.bottomMargin: 10
        spacing: 2

        SeekBar {
            Layout.fillWidth: true
            position: controls.player.position
            duration: controls.player.duration
            cached: controls.player.cached
            seekable: controls.player.seekable
            onSeek: s => controls.player.seekTo(s)
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            IconButton {
                iconName: "previous"
                tip: qsTr("Previous (P)")
                tint: Theme.onVideo
                onClicked: App.previous()
            }
            IconButton {
                iconName: controls.player.paused || controls.player.eof ? "play" : "pause"
                tip: controls.player.paused ? qsTr("Play (Space)") : qsTr("Pause (Space)")
                tint: Theme.onVideo
                size: 46
                onClicked: {
                    if (controls.player.eof)
                        controls.player.seekTo(0);
                    controls.player.paused = !controls.player.paused;
                }
            }
            IconButton {
                iconName: "next"
                tip: qsTr("Next (N)")
                tint: Theme.onVideo
                enabled: App.playlist.hasNext
                onClicked: App.next()
            }

            IconButton {
                iconName: controls.player.muted || controls.player.volume === 0 ? "muted"
                        : controls.player.volume < 50 ? "volume-low" : "volume"
                tip: controls.player.muted ? qsTr("Unmute (M)") : qsTr("Mute (M)")
                tint: Theme.onVideo
                Layout.leftMargin: 8
                onClicked: controls.player.muted = !controls.player.muted
            }
            Slider {
                id: volume
                Layout.preferredWidth: 96
                from: 0
                to: 130
                stepSize: 1
                value: controls.player.volume
                focusPolicy: Qt.NoFocus
                onMoved: {
                    controls.player.volume = value;
                    if (controls.player.muted)
                        controls.player.muted = false;
                }
                Accessible.name: qsTr("Volume")
                background: Rectangle {
                    x: volume.leftPadding
                    y: volume.topPadding + volume.availableHeight / 2 - height / 2
                    width: volume.availableWidth
                    height: 4
                    radius: 2
                    color: Qt.alpha(Theme.onVideo, 0.25)
                    Rectangle {
                        width: volume.visualPosition * parent.width
                        height: parent.height
                        radius: 2
                        color: controls.player.muted ? Qt.alpha(Theme.onVideo, 0.5) : Theme.onVideo
                    }
                }
                handle: Rectangle {
                    x: volume.leftPadding + volume.visualPosition * (volume.availableWidth - width)
                    y: volume.topPadding + volume.availableHeight / 2 - height / 2
                    width: 12
                    height: 12
                    radius: 6
                    color: Theme.onVideo
                    scale: volume.hovered || volume.pressed ? 1.15 : 1
                }
                ToolTip.visible: hovered || pressed
                ToolTip.text: Math.round(value) + "%"
            }

            Label {
                Layout.leftMargin: 8
                color: Theme.onVideo
                font.family: Theme.fontUi
                font.pointSize: Theme.smallSize
                font.features: { "tnum": 1 }
                text: Logic.formatTime(controls.player.position) + "  /  "
                      + Logic.formatTime(controls.player.duration)
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: Math.abs(controls.player.speed - 1) > 0.001
                color: Theme.onVideo
                font.family: Theme.fontUi
                font.pointSize: Theme.smallSize
                text: Logic.speedLabel(controls.player.speed)
                Layout.rightMargin: 6
            }
            IconButton {
                iconName: "subtitles"
                tip: qsTr("Subtitles (V)")
                tint: Theme.onVideo
                active: Logic.selectedId(controls.player.tracks, "sub") > 0
                onClicked: subMenu.popup(this, 0, -subMenu.implicitHeight)
                TrackMenu {
                    id: subMenu
                    type: "sub"
                    tracks: controls.player.tracks
                    onPick: id => controls.player.selectTrack("sub", id)
                }
            }
            IconButton {
                iconName: "audio"
                tip: qsTr("Audio track (A)")
                tint: Theme.onVideo
                visible: Logic.tracksOf(controls.player.tracks, "audio").length > 1
                onClicked: audioMenu.popup(this, 0, -audioMenu.implicitHeight)
                TrackMenu {
                    id: audioMenu
                    type: "audio"
                    tracks: controls.player.tracks
                    onPick: id => controls.player.selectTrack("audio", id)
                }
            }
            IconButton {
                iconName: "playlist"
                tip: qsTr("Queue (Q)")
                tint: Theme.onVideo
                active: controls.playlistOpen
                onClicked: controls.togglePlaylist()
            }
            IconButton {
                iconName: App.fullscreen ? "fullscreen-exit" : "fullscreen"
                tip: App.fullscreen ? qsTr("Leave full screen (F)") : qsTr("Full screen (F)")
                tint: Theme.onVideo
                onClicked: controls.toggleFullscreen()
            }
        }
    }
}
