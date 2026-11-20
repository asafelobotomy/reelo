#include "playlist.h"
#include "media.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace reelo {

Playlist::Playlist(QObject *parent) : QAbstractListModel(parent) {}

int Playlist::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

static bool isStream(const QString &s)
{
    return s.contains(QStringLiteral("://")) && !s.startsWith(QStringLiteral("file://"));
}

QVariant Playlist::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const QString &item = m_items.at(index.row());
    switch (role) {
    case PathRole:
        return item;
    case TitleRole:
    case Qt::DisplayRole:
        return isStream(item) ? item : QFileInfo(item).completeBaseName();
    case CurrentRole:
        return index.row() == m_current;
    case KindRole:
        if (isStream(item))
            return QStringLiteral("stream");
        return kindOf(item) == Kind::Audio ? QStringLiteral("audio") : QStringLiteral("video");
    }
    return {};
}

QHash<int, QByteArray> Playlist::roleNames() const
{
    return {{PathRole, "path"}, {TitleRole, "title"}, {CurrentRole, "isCurrent"}, {KindRole, "kind"}};
}

QString Playlist::current() const
{
    return m_current >= 0 && m_current < m_items.size() ? m_items.at(m_current) : QString();
}

QString Playlist::normalise(const QString &input)
{
    QString s = input.trimmed();
    if (s.isEmpty())
        return {};
    if (s.startsWith(QStringLiteral("file://")))
        s = QUrl(s).toLocalFile();
    if (isStream(s)) {
        const QUrl url(s);
        static const QStringList schemes = {"http", "https", "rtsp", "rtmp", "mms", "ytdl"};
        return url.isValid() && schemes.contains(url.scheme().toLower()) ? s : QString();
    }
    const QFileInfo fi(expandHome(s));
    if (!fi.isFile() || !fi.isReadable())
        return {};
    return fi.absoluteFilePath();
}

QStringList Playlist::siblings(const QString &file)
{
    const QFileInfo fi(file);
    QStringList names = fi.dir().entryList(QDir::Files | QDir::Readable);
    names.erase(std::remove_if(names.begin(), names.end(),
                               [](const QString &n) { return !isMedia(n); }),
                names.end());
    if (!names.contains(fi.fileName()))
        names.append(fi.fileName());
    naturalSort(names);
    QStringList out;
    for (const QString &n : names)
        out.append(fi.dir().absoluteFilePath(n));
    return out;
}

QStringList Playlist::accept(const QStringList &inputs) const
{
    QStringList out;
    for (const QString &in : inputs) {
        const QString n = normalise(in);
        if (!n.isEmpty())
            out.append(n);
    }
    return out;
}

bool Playlist::open(const QStringList &inputs, bool withFolder)
{
    QStringList items = accept(inputs);
    if (items.isEmpty())
        return false;
    int start = 0;
    if (withFolder && items.size() == 1 && !isStream(items.first()) && isMedia(items.first())) {
        const QString chosen = items.first();
        items = siblings(chosen);
        start = std::max<int>(0, items.indexOf(chosen));
    }
    beginResetModel();
    m_items = items;
    m_current = -1;
    endResetModel();
    emit countChanged();
    setCurrent(start);
    return true;
}

void Playlist::append(const QStringList &inputs)
{
    const QStringList items = accept(inputs);
    if (items.isEmpty())
        return;
    const bool wasEmpty = m_items.isEmpty();
    beginInsertRows({}, m_items.size(), m_items.size() + items.size() - 1);
    m_items.append(items);
    endInsertRows();
    emit countChanged();
    if (wasEmpty)
        setCurrent(0);
    else
        emit currentChanged(); // hasNext may have changed
}

bool Playlist::next()
{
    if (!hasNext())
        return false;
    setCurrent(m_current + 1);
    return true;
}

bool Playlist::previous()
{
    if (!hasPrevious())
        return false;
    setCurrent(m_current - 1);
    return true;
}

void Playlist::playAt(int index)
{
    if (index >= 0 && index < m_items.size())
        setCurrent(index);
}

void Playlist::remove(int index)
{
    if (index < 0 || index >= m_items.size())
        return;
    beginRemoveRows({}, index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit countChanged();
    if (index < m_current) {
        --m_current;
        emit currentChanged();
    } else if (index == m_current) {
        // The one playing went: play what took its place, or stop at the end.
        const int next = m_current < m_items.size() ? m_current : -1;
        m_current = -2; // force a change
        setCurrent(next);
    } else {
        emit currentChanged();
    }
}

void Playlist::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
    m_current = -2;
    setCurrent(-1);
}

void Playlist::setCurrent(int index)
{
    if (index == m_current)
        return;
    const int old = m_current;
    m_current = index;
    if (old >= 0 && old < m_items.size())
        emit dataChanged(this->index(old), this->index(old), {CurrentRole});
    if (index >= 0)
        emit dataChanged(this->index(index), this->index(index), {CurrentRole});
    emit currentChanged();
}

} // namespace reelo
