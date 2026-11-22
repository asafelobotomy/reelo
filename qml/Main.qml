import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Reelo
import "logic.js" as Logic

ApplicationWindow {
    id: win
    width: 1100
    height: 680
    minimumWidth: 480
    minimumHeight: 300
    visible: true
    title: Logic.windowTitle(player.mediaTitle, player.idle || App.playlist.count === 0)
    color: playing ? "#000000" : Theme.bg
    visibility: App.fullscreen ? Window.FullScreen : Window.AutomaticVisibility

    readonly property bool playing: App.playlist.count > 0
    property bool playlistOpen: false
    // Controls show while the pointer moves, while paused, and while a menu is open.
    property bool pointerActive: true
    readonly property bool controlsShown: !playing || pointerActive || player.paused || player.eof
                                          || controlsHover.hovered

    palette.window: Theme.surface
    palette.windowText: Theme.text
    palette.base: Theme.bg
    palette.text: Theme.text
    palette.button: Theme.surfaceAlt
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    palette.light: Theme.overlay
    palette.mid: Theme.border
    palette.dark: Theme.border
    palette.toolTipBase: Theme.overlay
    palette.toolTipText: Theme.text
    palette.placeholderText: Theme.textMuted
    font.family: Theme.fontUi

    Component.onCompleted: App.player = player

    function osd(text) { osdBox.show(text); }

    function nudgeVolume(by) {
        player.volume = Math.max(0, Math.min(130, player.volume + by));
        if (player.muted)
            player.muted = false;
        osd(qsTr("Volume %1%").arg(player.volume));
    }

    function cycleTrack(type) {
        const list = Logic.tracksOf(player.tracks, type);
        if (list.length === 0)
            return osd(type === "sub" ? qsTr("No subtitles") : qsTr("No audio tracks"));
        const ids = (type === "sub" ? [0] : []).concat(list.map(t => t.id));
        const next = ids[(ids.indexOf(Logic.selectedId(player.tracks, type)) + 1) % ids.length];
        player.selectTrack(type, next);
        const t = list.find(x => x.id === next);
        osd((type === "sub" ? qsTr("Subtitles: ") : qsTr("Audio: ")) + (t ? Logic.trackLabel(t) : qsTr("off")));
    }

    function togglePause() {
        if (!playing)
            return;
        if (player.eof)
            player.seekTo(0);
        player.paused = !player.paused;
    }

    function setSpeed(s) {
        player.speed = s;
        osd(qsTr("Speed %1").arg(Logic.speedLabel(s)));
    }

    // Lift subtitles above the controls while they show.
    onControlsShownChanged: player.set("sub-margin-y", controlsShown && playing ? 110 : 34)

    Connections {
        target: App
        function onNotice(text) { win.osd(text); }
    }

    // ---- the picture
    // Always in the scene: mpv's video output can only start once the item has rendered once
    // (its OpenGL context comes from the first frame), and it learns about tracks after that.
    MpvObject {
        id: player
        anchors.fill: parent
    }

    AudioView {
        anchors.fill: parent
        visible: win.playing && !player.hasVideo && !player.idle
        title: player.metadata.title || player.metadata.TITLE || player.mediaTitle
        artist: player.metadata.artist || player.metadata.ARTIST || ""
        album: player.metadata.album || player.metadata.ALBUM || ""
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: win.playing && (player.buffering || (player.idle && !player.error))
        visible: running
    }

    // ---- pointer: hide the cursor and controls after a moment of stillness while playing
    MouseArea {
        id: surface
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: win.controlsShown || !win.playing ? Qt.ArrowCursor : Qt.BlankCursor
        enabled: win.playing
        onPositionChanged: { win.pointerActive = true; idleTimer.restart(); }
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton)
                win.togglePause();
            else
                clickTimer.restart();
        }
        onDoubleClicked: { clickTimer.stop(); App.fullscreen = !App.fullscreen; }
        onWheel: wheel => win.nudgeVolume(wheel.angleDelta.y > 0 ? 5 : -5)
    }
    // A single click pauses, unless it turns out to be a double click.
    Timer { id: clickTimer; interval: 250; onTriggered: win.togglePause() }
    Timer { id: idleTimer; interval: 2200; running: true; onTriggered: win.pointerActive = false }

    // ---- top: title and close
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        visible: win.playing
        opacity: win.controlsShown ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.motionSlow; easing.type: Theme.easing } }
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.alpha(Theme.scrim, 0.65) }
            GradientStop { position: 1; color: "transparent" }
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 10
            anchors.bottomMargin: 14
            Label {
                Layout.fillWidth: true
                text: player.mediaTitle
                elide: Text.ElideRight
                color: Theme.onVideo
                font.pointSize: Theme.bodySize + 1
                font.weight: Font.DemiBold
            }
            IconButton {
                iconName: "open"
                tip: qsTr("Open files (Ctrl+O)")
                tint: Theme.onVideo
                onClicked: fileDialog.open()
            }
            IconButton {
                iconName: "close"
                tip: qsTr("Close and go back (Ctrl+W)")
                tint: Theme.onVideo
                onClicked: { App.fullscreen = false; App.close(); }
            }
        }
    }

    Controls {
        id: controls
        player: player
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: win.playlistOpen ? playlistPanel.width + 16 : 0
        visible: win.playing
        opacity: win.controlsShown ? 1 : 0
        enabled: opacity > 0.5
        Behavior on opacity { NumberAnimation { duration: Theme.motionSlow; easing.type: Theme.easing } }
        playlistOpen: win.playlistOpen
        onTogglePlaylist: win.playlistOpen = !win.playlistOpen
        onToggleFullscreen: App.fullscreen = !App.fullscreen
        HoverHandler { id: controlsHover }
    }

    PlaylistPanel {
        id: playlistPanel
        visible: win.playing && win.playlistOpen
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 12
        anchors.topMargin: 60
        width: Math.min(340, win.width * 0.4)
        onCloseRequested: win.playlistOpen = false
    }

    Osd {
        id: osdBox
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 72
        z: 10
    }

    // ---- start screen
    Home {
        anchors.fill: parent
        visible: !win.playing
        onOpenFiles: fileDialog.open()
        onOpenUrl: urlDialog.open()
    }

    DropArea {
        anchors.fill: parent
        onDropped: drop => {
            const items = Logic.dropped(drop.urls);
            if (items.length === 0)
                return;
            if (win.playing && drop.modifiers & Qt.ShiftModifier)
                App.enqueue(items);
            else if (!App.open(items))
                win.osd(qsTr("Nothing playable in what was dropped"));
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Open videos or music")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Videos and music (*.mkv *.mp4 *.webm *.avi *.mov *.m4v *.ts *.mp3 *.flac *.ogg *.opus *.m4a *.wav)"),
                      qsTr("All files (*)")]
        onAccepted: {
            if (!App.open(Logic.dropped(selectedFiles)))
                win.osd(qsTr("Nothing playable in what was opened"));
        }
    }

    UrlDialog {
        id: urlDialog
        onAccepted: url => App.open([url])
    }

    // ---- keys
    Shortcut { sequences: ["Space", "K"]; onActivated: win.togglePause() }
    Shortcut { sequence: "Left"; enabled: win.playing; onActivated: player.seekBy(-5) }
    Shortcut { sequence: "Right"; enabled: win.playing; onActivated: player.seekBy(5) }
    Shortcut { sequence: "J"; enabled: win.playing; onActivated: player.seekBy(-10) }
    Shortcut { sequence: "L"; enabled: win.playing; onActivated: player.seekBy(10) }
    Shortcut { sequence: "Shift+Left"; enabled: win.playing; onActivated: player.seekBy(-60) }
    Shortcut { sequence: "Shift+Right"; enabled: win.playing; onActivated: player.seekBy(60) }
    Shortcut { sequence: "Up"; onActivated: win.nudgeVolume(5) }
    Shortcut { sequence: "Down"; onActivated: win.nudgeVolume(-5) }
    Shortcut {
        sequence: "M"
        onActivated: { player.muted = !player.muted; win.osd(player.muted ? qsTr("Muted") : qsTr("Sound on")); }
    }
    Shortcut { sequences: ["F", "F11"]; onActivated: App.fullscreen = !App.fullscreen }
    Shortcut { sequence: "Escape"; enabled: App.fullscreen; onActivated: App.fullscreen = false }
    Shortcut { sequences: ["N", "Media Next"]; enabled: win.playing; onActivated: App.next() }
    Shortcut { sequences: ["P", "Media Previous"]; enabled: win.playing; onActivated: App.previous() }
    Shortcut { sequence: "V"; enabled: win.playing; onActivated: win.cycleTrack("sub") }
    Shortcut { sequence: "A"; enabled: win.playing; onActivated: win.cycleTrack("audio") }
    Shortcut { sequence: "Q"; enabled: win.playing; onActivated: win.playlistOpen = !win.playlistOpen }
    Shortcut { sequence: "]"; enabled: win.playing; onActivated: win.setSpeed(Logic.stepSpeed(player.speed, true)) }
    Shortcut { sequence: "["; enabled: win.playing; onActivated: win.setSpeed(Logic.stepSpeed(player.speed, false)) }
    Shortcut { sequence: "Backspace"; enabled: win.playing; onActivated: win.setSpeed(1) }
    Shortcut { sequence: "."; enabled: win.playing && player.paused; onActivated: player.command(["frame-step"]) }
    Shortcut { sequence: ","; enabled: win.playing && player.paused; onActivated: player.command(["frame-back-step"]) }
    Shortcut { sequences: [StandardKey.Open]; onActivated: fileDialog.open() }
    Shortcut { sequence: "Ctrl+L"; onActivated: urlDialog.open() }
    Shortcut { sequence: "Ctrl+W"; enabled: win.playing; onActivated: { App.fullscreen = false; App.close(); } }
    Shortcut { sequence: "Ctrl+Q"; onActivated: Qt.quit() }
    Shortcut { sequence: "Media Play"; onActivated: win.togglePause() }
    Shortcut { sequence: "Media Pause"; onActivated: player.paused = true }
}
