#include "theme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace reelo {

QString tokensPath()
{
    const QString env = qEnvironmentVariable("REELO_UQ_TOKENS");
    if (!env.isEmpty())
        return env;
    QString state = qEnvironmentVariable("XDG_STATE_HOME");
    if (!QDir::isAbsolutePath(state))
        state = QDir::homePath() + "/.local/state";
    return state + "/uq/theme/tokens.json";
}

QVariantMap loadTokens(const QString &path)
{
    const QVariantMap absent{{"present", false}};
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly) || f.size() > 256 * 1024)
        return absent;
    const QJsonObject v = QJsonDocument::fromJson(f.readAll()).object();
    static const QStringList names = {"bg", "surface", "surface_alt", "overlay", "text",
                                      "text_muted", "accent", "accent_text", "border",
                                      "success", "warning", "error"};
    static const QRegularExpression colour(QStringLiteral("^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$"));
    const QJsonObject in = v.value("colors").toObject();
    QVariantMap colors;
    for (const QString &k : names) {
        const QString c = in.value(k).toString();
        if (colour.match(c).hasMatch())
            colors.insert(k, c);
    }
    if (colors.isEmpty())
        return absent;
    const QJsonObject fonts = v.value("fonts").toObject();
    const int radius = v.value("ui").toObject().value("radius").toInt(8);
    return {
        {"present", true},
        {"colors", colors},
        {"radius", std::clamp(radius, 0, 32)},
        {"fontUi", fonts.value("ui").toString()},
        {"fontMono", fonts.value("mono").toString()},
        {"fontSize", std::clamp(fonts.value("size").toInt(0), 0, 32)},
    };
}

ThemeWatcher::ThemeWatcher(QString path, QObject *parent)
    : QObject(parent), m_path(std::move(path)), m_tokens(loadTokens(m_path))
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(150);
    connect(&m_debounce, &QTimer::timeout, this, &ThemeWatcher::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce,
            qOverload<>(&QTimer::start));
    rewatch();
}

void ThemeWatcher::rewatch()
{
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    // Watch the nearest folder that exists, so a theme written for the first time is seen.
    QString dir = QFileInfo(m_path).absolutePath();
    while (!QFileInfo::exists(dir) && dir != "/")
        dir = QFileInfo(dir).absolutePath();
    m_watcher.addPath(dir);
    if (QFileInfo::exists(m_path))
        m_watcher.addPath(m_path);
}

void ThemeWatcher::reload()
{
    rewatch();
    const QVariantMap next = loadTokens(m_path);
    if (next != m_tokens) {
        m_tokens = next;
        emit tokensChanged();
    }
}

} // namespace reelo
