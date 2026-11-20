// uQ's theme tokens (~/.local/state/uq/theme/tokens.json, written by uqd) for the QML Theme
// singleton. Without uQ, `present` is false and the window uses its own dark palette.
#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

namespace reelo {

/// $REELO_UQ_TOKENS, else $XDG_STATE_HOME/uq/theme/tokens.json, else ~/.local/state/uq/...
QString tokensPath();

/// {present, colors{bg, surface, ...}, radius, fontUi, fontMono, fontSize}; {present: false}
/// when the file is missing, too big, or has no colours.
QVariantMap loadTokens(const QString &path);

/// Follows the tokens file (it is replaced, not rewritten, so the folder is watched too).
class ThemeWatcher : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap tokens READ tokens NOTIFY tokensChanged)

public:
    explicit ThemeWatcher(QString path = tokensPath(), QObject *parent = nullptr);
    QVariantMap tokens() const { return m_tokens; }

signals:
    void tokensChanged();

private:
    void reload();
    void rewatch();
    QString m_path;
    QVariantMap m_tokens;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};

} // namespace reelo
