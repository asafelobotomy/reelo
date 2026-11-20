// The start screen's list: the newest videos and music in the library folder (~/Downloads by
// default, where AV Morning Star and hauler save) and one level of folders below it.
#pragma once

#include <QJsonArray>
#include <QString>

namespace reelo {

/// Newest first, at most `limit`: [{path, name, folder, kind, size, modified (ms since epoch)}].
/// Hidden files and folders are skipped; a missing folder gives an empty list.
QJsonArray scanLibrary(const QString &folder, int limit = 60);

} // namespace reelo
