#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>

namespace reelo {

const QList<Key> &keys()
{
    static const QList<Key> all = {
        {"resume", "bool", true, {}, {},
         "Start a file where you stopped it last time."},
        {"autoplay_next", "bool", true, {}, {},
         "When a file ends, play the next one in its folder."},
        {"keep_awake", "bool", true, {}, {},
         "Keep the screen on while a video plays."},
        {"hwdec", "choice", QStringLiteral("auto"), {"auto", "off"}, {},
         "Decode video on the graphics card when it can."},
        {"subtitles", "choice", QStringLiteral("auto"), {"auto", "off"}, {},
         "Load and show subtitles that match the file."},
        {"sub_lang", "text", QString(), {}, QStringLiteral("^([a-z]{2,3}(,[a-z]{2,3}){0,7})?$"),
         "Preferred subtitle languages, e.g. en,eng."},
        {"audio_lang", "text", QString(), {}, QStringLiteral("^([a-z]{2,3}(,[a-z]{2,3}){0,7})?$"),
         "Preferred audio languages, e.g. ja,jpn."},
        {"library", "text", QStringLiteral("~/Downloads"), {},
         QStringLiteral("^~?/[^\\x00-\\x1f]{0,511}$"),
         "Folder whose newest videos and music the start screen lists (AV Morning Star saves here)."},
    };
    return all;
}

const Key *findKey(const QString &name)
{
    for (const Key &k : keys())
        if (k.name == name)
            return &k;
    return nullptr;
}

static QString xdg(const char *var, const QString &fallback)
{
    const QString v = qEnvironmentVariable(var);
    return QDir::isAbsolutePath(v) ? v : QDir::homePath() + fallback;
}

QString configPath()
{
    const QString env = qEnvironmentVariable("REELO_CONFIG");
    if (!env.isEmpty())
        return env;
    return xdg("XDG_CONFIG_HOME", "/.config") + "/reelo/config.json";
}

QString stateDir()
{
    return xdg("XDG_STATE_HOME", "/.local/state") + "/reelo";
}

Config::Config(QString path) : m_path(std::move(path)) { load(); }

QVariant Config::parse(const Key &key, const QString &raw, QString *error)
{
    const QString v = raw.trimmed();
    if (key.type == "bool") {
        static const QStringList yes = {"true", "on", "yes", "1"};
        static const QStringList no = {"false", "off", "no", "0"};
        if (yes.contains(v.toLower()))
            return true;
        if (no.contains(v.toLower()))
            return false;
        *error = QStringLiteral("%1 is on or off (true/false), not \"%2\"").arg(key.name, raw);
        return {};
    }
    if (key.type == "choice") {
        if (key.choices.contains(v))
            return v;
        *error = QStringLiteral("%1 is one of %2").arg(key.name, key.choices.join(", "));
        return {};
    }
    if (!QRegularExpression(key.pattern).match(v).hasMatch()) {
        *error = QStringLiteral("\"%1\" is not a valid %2").arg(raw, key.name);
        return {};
    }
    return v;
}

void Config::load()
{
    m_values = {};
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly) || f.size() > 64 * 1024)
        return;
    const QJsonObject in = QJsonDocument::fromJson(f.readAll()).object();
    for (const Key &k : keys()) {
        const QJsonValue v = in.value(k.name);
        if (v.isUndefined())
            continue;
        QString err;
        const QString raw = v.isBool() ? (v.toBool() ? "true" : "false") : v.toString();
        if (!v.isBool() && !v.isString())
            continue;
        const QVariant ok = parse(k, raw, &err);
        if (ok.isValid())
            m_values.insert(k.name, QJsonValue::fromVariant(ok));
    }
}

QVariant Config::value(const QString &key) const
{
    const Key *k = findKey(key);
    if (!k)
        return {};
    const QJsonValue v = m_values.value(key);
    return v.isUndefined() ? k->fallback : v.toVariant();
}

QString Config::save() const
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly))
        return QStringLiteral("can't write %1: %2").arg(m_path, f.errorString());
    f.write(QJsonDocument(m_values).toJson(QJsonDocument::Indented));
    if (!f.commit())
        return QStringLiteral("can't write %1: %2").arg(m_path, f.errorString());
    return {};
}

QString Config::set(const QString &key, const QString &raw)
{
    const Key *k = findKey(key);
    if (!k)
        return QStringLiteral("unknown setting \"%1\"").arg(key);
    load();
    QString err;
    const QVariant v = parse(*k, raw, &err);
    if (!v.isValid())
        return err;
    m_values.insert(key, QJsonValue::fromVariant(v));
    return save();
}

QString Config::reset(const QString &key)
{
    if (!findKey(key))
        return QStringLiteral("unknown setting \"%1\"").arg(key);
    load();
    m_values.remove(key);
    return save();
}

QJsonObject Config::describe() const
{
    QJsonArray list;
    for (const Key &k : keys()) {
        QJsonObject o{{"key", k.name},
                      {"type", k.type},
                      {"default", QJsonValue::fromVariant(k.fallback)},
                      {"value", QJsonValue::fromVariant(value(k.name))},
                      {"set", m_values.contains(k.name)},
                      {"help", k.help}};
        if (!k.choices.isEmpty())
            o.insert("choices", QJsonArray::fromStringList(k.choices));
        list.append(o);
    }
    return {{"settings", list}, {"path", m_path}};
}

} // namespace reelo
