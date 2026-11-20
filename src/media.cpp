#include "media.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <algorithm>

namespace reelo {

Kind kindOf(const QString &path)
{
    static const QSet<QString> video = {
        "mkv", "mp4", "m4v", "webm", "avi", "mov", "wmv", "flv", "mpg", "mpeg", "ts", "m2ts",
        "mts", "ogv", "3gp", "vob", "divx", "rmvb", "asf", "mxf", "y4m"};
    static const QSet<QString> audio = {
        "mp3", "flac", "ogg", "oga", "opus", "m4a", "aac", "wav", "wma", "alac", "ape", "mka",
        "aiff", "aif", "wv", "mpc", "dsf", "dff", "mid", "midi", "spx", "tta"};
    const QString ext = QFileInfo(path).suffix().toLower();
    if (video.contains(ext))
        return Kind::Video;
    if (audio.contains(ext))
        return Kind::Audio;
    return Kind::None;
}

const QStringList &mimeTypes()
{
    static const QStringList all = {
        "video/mp4", "video/x-matroska", "video/webm", "video/x-msvideo", "video/quicktime",
        "video/x-ms-wmv", "video/x-flv", "video/mpeg", "video/mp2t", "video/ogg", "video/3gpp",
        "video/x-m4v", "video/vnd.avi", "video/x-ogm+ogg", "video/x-theora+ogg",
        "audio/mpeg", "audio/flac", "audio/x-flac", "audio/ogg", "audio/x-vorbis+ogg",
        "audio/opus", "audio/mp4", "audio/x-m4a", "audio/aac", "audio/x-wav", "audio/wav",
        "audio/x-ms-wma", "audio/x-matroska", "audio/x-ape", "audio/x-aiff", "audio/x-wavpack",
        "audio/webm", "audio/x-mpegurl", "audio/x-scpls"};
    return all;
}

QString expandHome(const QString &path)
{
    if (path == "~")
        return QDir::homePath();
    if (path.startsWith("~/"))
        return QDir::homePath() + path.mid(1);
    return path;
}

int naturalCompare(const QString &a, const QString &b)
{
    // Runs of digits compare as numbers, the rest case-insensitively. Done by hand rather than
    // with QCollator, whose numeric mode does nothing under the C locale.
    qsizetype i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a.at(i).isDigit() && b.at(j).isDigit()) {
            qsizetype ei = i, ej = j;
            while (ei < a.size() && a.at(ei).isDigit())
                ++ei;
            while (ej < b.size() && b.at(ej).isDigit())
                ++ej;
            QStringView na = QStringView(a).mid(i, ei - i), nb = QStringView(b).mid(j, ej - j);
            while (na.size() > 1 && na.front() == u'0')
                na = na.mid(1);
            while (nb.size() > 1 && nb.front() == u'0')
                nb = nb.mid(1);
            if (na.size() != nb.size())
                return na.size() < nb.size() ? -1 : 1;
            if (const int c = na.compare(nb))
                return c < 0 ? -1 : 1;
            i = ei;
            j = ej;
            continue;
        }
        const QChar ca = a.at(i).toCaseFolded(), cb = b.at(j).toCaseFolded();
        if (ca != cb)
            return ca < cb ? -1 : 1;
        ++i;
        ++j;
    }
    if (i < a.size() || j < b.size())
        return i < a.size() ? 1 : -1;
    return QString::compare(a, b); // equal ignoring case and zeros: still a stable order
}

void naturalSort(QStringList &names)
{
    std::sort(names.begin(), names.end(),
              [](const QString &a, const QString &b) { return naturalCompare(a, b) < 0; });
}

} // namespace reelo
