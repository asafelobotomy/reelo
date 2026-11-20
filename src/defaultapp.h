// Whether reelo is the default app for video and audio (mimeapps.list), and making it so.
#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace reelo {

inline constexpr auto kDesktopId = "io.github.asafelobotomy.reelo.desktop";

/// $XDG_CONFIG_HOME/mimeapps.list (or ~/.config/mimeapps.list).
QString userMimeappsPath();
/// The files after the user's: $XDG_CONFIG_DIRS/mimeapps.list, then the system's.
QStringList systemMimeappsPaths();

/// The default app for one type: the first `[Default Applications]` entry naming it, user
/// file first. Empty when none is set.
QString defaultFor(const QString &mime, const QString &user, const QStringList &system);

/// `{desktop_id, types, ours, default (all ours), video, audio, others: {id: count}}`.
QJsonObject defaultStatus(const QString &user = userMimeappsPath(),
                          const QStringList &system = systemMimeappsPaths());

/// Makes reelo the default for every type in mimeTypes(), in the user's file only, keeping
/// every other line. Returns an error message or an empty string.
QString makeDefault(const QString &user = userMimeappsPath());

/// The text of a mimeapps.list with `types` set to `id` under [Default Applications].
QString withDefaults(const QString &text, const QStringList &types, const QString &id);

} // namespace reelo
