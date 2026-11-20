import QtQuick
import QtQuick.Controls
import "logic.js" as Logic

// A menu of the subtitle or audio tracks, with "Off" for subtitles.
Menu {
    id: menu
    property string type: "sub"
    property var tracks: []
    signal pick(int id)

    readonly property var list: Logic.tracksOf(tracks, type)
    readonly property int current: Logic.selectedId(tracks, type)

    Instantiator {
        model: menu.type === "sub" ? 1 : 0
        delegate: MenuItem {
            text: qsTr("Off")
            checkable: true
            checked: menu.current === 0
            onTriggered: menu.pick(0)
        }
        onObjectAdded: (i, o) => menu.insertItem(0, o)
        onObjectRemoved: (i, o) => menu.removeItem(o)
    }
    Instantiator {
        model: menu.list
        delegate: MenuItem {
            required property var modelData
            text: Logic.trackLabel(modelData)
            checkable: true
            checked: modelData.selected === true
            onTriggered: menu.pick(modelData.id)
        }
        onObjectAdded: (i, o) => menu.addItem(o)
        onObjectRemoved: (i, o) => menu.removeItem(o)
    }
    MenuItem {
        text: menu.type === "sub" ? qsTr("No subtitles in this file") : qsTr("No audio tracks")
        enabled: false
        visible: menu.list.length === 0
        height: visible ? implicitHeight : 0
    }
}
