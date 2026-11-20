// The play queue: local files and URLs, with one current entry. Opening a single local file
// queues the playable files of its folder around it (in natural order), so "next" goes on.
#pragma once

#include <QAbstractListModel>
#include <QStringList>

namespace reelo {

class Playlist : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentChanged)
    Q_PROPERTY(QString current READ current NOTIFY currentChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY currentChanged)
    Q_PROPERTY(bool hasPrevious READ hasPrevious NOTIFY currentChanged)

public:
    enum Roles { PathRole = Qt::UserRole + 1, TitleRole, CurrentRole, KindRole };
    explicit Playlist(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_items.size(); }
    int currentIndex() const { return m_current; }
    QString current() const;
    bool hasNext() const { return m_current + 1 < m_items.size(); }
    bool hasPrevious() const { return m_current > 0; }
    QStringList items() const { return m_items; }

    /// Replaces the queue. One local file with `withFolder` queues its folder's media too.
    /// Returns false (and changes nothing) when nothing in `inputs` is playable.
    Q_INVOKABLE bool open(const QStringList &inputs, bool withFolder = true);
    /// Adds to the end; starts playing when the queue was empty.
    Q_INVOKABLE void append(const QStringList &inputs);
    Q_INVOKABLE bool next();
    Q_INVOKABLE bool previous();
    Q_INVOKABLE void playAt(int index);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

    /// A path, file:// URL or stream URL as reelo plays it: local files absolute, the rest as is.
    /// Empty when it is neither a playable local file nor a URL mpv can open.
    static QString normalise(const QString &input);
    /// The playable files of `file`'s folder, `file` included, in natural order.
    static QStringList siblings(const QString &file);

signals:
    void countChanged();
    void currentChanged();

private:
    void setCurrent(int index);
    QStringList accept(const QStringList &inputs) const;
    QStringList m_items;
    int m_current = -1;
};

} // namespace reelo
