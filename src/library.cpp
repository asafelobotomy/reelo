#include "library.h"
#include "media.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <algorithm>

namespace reelo {

static void collect(const QDir &dir, QList<QFileInfo> &out)
{
    const auto entries = dir.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : entries)
        if (isMedia(fi.fileName()) && fi.size() > 0)
            out.append(fi);
}

QJsonArray scanLibrary(const QString &folder, int limit)
{
    const QDir root(expandHome(folder));
    if (folder.isEmpty() || !root.exists())
        return {};
    QList<QFileInfo> files;
    collect(root, files);
    // One level down (a channel or album folder), but not into hidden or huge trees.
    const auto subdirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    int visited = 0;
    for (const QFileInfo &d : subdirs) {
        if (++visited > 200)
            break;
        if (!d.isSymLink())
            collect(QDir(d.absoluteFilePath()), files);
    }
    std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.lastModified() > b.lastModified();
    });
    QJsonArray out;
    for (const QFileInfo &fi : files) {
        if (out.size() >= limit)
            break;
        const QString rel = root.relativeFilePath(fi.absolutePath());
        out.append(QJsonObject{
            {"path", fi.absoluteFilePath()},
            {"name", fi.completeBaseName()},
            {"folder", rel == "." ? QString() : rel},
            {"kind", kindOf(fi.fileName()) == Kind::Audio ? "audio" : "video"},
            {"size", double(fi.size())},
            {"modified", double(fi.lastModified().toMSecsSinceEpoch())},
        });
    }
    return out;
}

} // namespace reelo
