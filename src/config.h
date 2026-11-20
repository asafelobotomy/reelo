// reelo's settings: one JSON file (~/.config/reelo/config.json) that the window, the CLI
// (`reelo config ...`) and uQ Settings' page all read and write. Only known keys with valid
// values are ever written; a bad or missing file means the defaults.
#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace reelo {

struct Key {
    QString name;
    QString type;        // "bool", "choice" or "text"
    QVariant fallback;   // the default
    QStringList choices; // for "choice"
    QString pattern;     // for "text": the whole value must match
    QString help;
};

/// Every setting, in the order the page and `config show` list them.
const QList<Key> &keys();
const Key *findKey(const QString &name);

/// $REELO_CONFIG, else $XDG_CONFIG_HOME/reelo/config.json, else ~/.config/reelo/config.json.
QString configPath();
/// $XDG_STATE_HOME/reelo (resume positions and the like), else ~/.local/state/reelo.
QString stateDir();

class Config {
public:
    explicit Config(QString path = configPath());

    /// Re-reads the file. Unknown keys and bad values are dropped (their defaults apply).
    void load();
    QVariant value(const QString &key) const;
    bool boolean(const QString &key) const { return value(key).toBool(); }
    QString text(const QString &key) const { return value(key).toString(); }

    /// Checks and stores one value (as typed on the command line: "true"/"false" for a bool).
    /// Returns an error message, or an empty string when it was saved.
    QString set(const QString &key, const QString &raw);
    /// Takes the key out of the file, so its default applies again.
    QString reset(const QString &key);

    /// `{settings: [{key, type, default, value, set, choices?, help}], path}` for `config show`.
    QJsonObject describe() const;
    QString path() const { return m_path; }

    /// The value parsed and checked for `key`, or an invalid QVariant with `error` set.
    static QVariant parse(const Key &key, const QString &raw, QString *error);

private:
    QString save() const;
    QString m_path;
    QJsonObject m_values;
};

} // namespace reelo
