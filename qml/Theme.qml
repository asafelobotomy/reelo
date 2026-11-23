pragma Singleton
import QtQuick
import Reelo

// Every colour, radius, font size and speed the window uses: uQ's shared theme base
// (UqThemeBase.qml, vendored unchanged from uQ's native/uq-consts). On uQ it follows uQ's theme
// and Reduce motion live; elsewhere it is uQ's Graphite palette.
UqThemeBase {
    tokens: App.theme.tokens

    // Controls over a playing video sit on a dark scrim whatever the theme, so their text and
    // icons stay white there; the start screen and panels use the theme's own colours.
    readonly property color scrim: "#000000"
    readonly property color onVideo: "#ffffff"
}
