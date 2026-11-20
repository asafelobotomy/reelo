// What counts as a playable file, and the MIME types reelo opens.
#pragma once

#include <QString>
#include <QStringList>

namespace reelo {

enum class Kind { None, Video, Audio };

/// By file extension (case-insensitive). Subtitles, images and the rest are None.
Kind kindOf(const QString &path);
inline bool isMedia(const QString &path) { return kindOf(path) != Kind::None; }

/// The MIME types the desktop entry lists, and `reelo default set` claims.
const QStringList &mimeTypes();

/// "~/x" -> "$HOME/x"; anything else unchanged.
QString expandHome(const QString &path);

/// <0, 0 or >0: "ep2" before "ep10", ignoring case.
int naturalCompare(const QString &a, const QString &b);
/// Sorts like a person would: "ep2" before "ep10", ignoring case.
void naturalSort(QStringList &names);

} // namespace reelo
