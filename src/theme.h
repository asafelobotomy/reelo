// uQ's theme tokens (~/.local/state/uq/theme/tokens.json, written by uqd) for the QML Theme
// singleton, from the vendored uq-theme module (uqtheme.h/.cpp, copied unchanged from uQ's
// native/uq-theme; CI checks them with scripts/check-uq-theme.sh). Without uQ, `present` is false
// and the window uses its own dark palette. The file is followed with a file watch, not polled.
#pragma once

#include "uqtheme.h"

namespace reelo {

/// $REELO_UQ_TOKENS, else $XDG_STATE_HOME/uq/theme/tokens.json, else ~/.local/state/uq/...
inline QString tokensPath() { return uqtheme::tokensPath("REELO_UQ_TOKENS"); }

/// {present, colors{bg, surface, ...}, radius, fontUi, fontMono, fontSize, ...}; {present: false}
/// when the file is missing, too big, or not a whole theme.
inline QVariantMap loadTokens(const QString &path) { return uqtheme::load(path); }

} // namespace reelo
