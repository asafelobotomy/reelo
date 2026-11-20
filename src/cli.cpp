#include "cli.h"
#include "config.h"
#include "defaultapp.h"
#include "library.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace reelo {

static const char *kUsage =
    "reelo " REELO_VERSION " - uQ's media player\n"
    "\n"
    "Usage:\n"
    "  reelo [FILE|URL]...           play files, folders' media or streams\n"
    "  reelo config show [--json]    every setting with its value and default\n"
    "  reelo config set KEY VALUE    change one setting\n"
    "  reelo config reset KEY        back to the default\n"
    "  reelo default status [--json] is reelo the default video and audio player?\n"
    "  reelo default set             make it the default for video and audio\n"
    "  reelo library [--json]        the newest media in the library folder\n"
    "  reelo --version | --help\n";

bool isCliCommand(const QStringList &argsIn)
{
    QStringList args = argsIn;
    args.removeAll(QStringLiteral("--json"));
    if (args.isEmpty())
        return false;
    static const QStringList cmds = {"config", "default", "library", "--version", "-V",
                                     "--help", "-h"};
    return cmds.contains(args.first());
}

static void json(QTextStream &out, const QJsonObject &o)
{
    out << QJsonDocument(o).toJson(QJsonDocument::Compact) << '\n';
}

static int fail(QTextStream &err, const QString &msg, int code = 1)
{
    err << "reelo: " << msg << '\n';
    return code;
}

int runCli(const QStringList &argsIn, QTextStream &out, QTextStream &err)
{
    QStringList args = argsIn;
    const bool asJson = args.removeAll("--json") > 0;
    const QString cmd = args.value(0);
    const QString sub = args.value(1);

    if (cmd == "--version" || cmd == "-V") {
        out << "reelo " << REELO_VERSION << '\n';
        return 0;
    }
    if (cmd == "--help" || cmd == "-h") {
        out << kUsage;
        return 0;
    }

    if (cmd == "config") {
        Config cfg;
        if (sub == "show" && args.size() == 2) {
            if (asJson) {
                json(out, cfg.describe());
            } else {
                for (const Key &k : keys())
                    out << k.name << " = " << cfg.value(k.name).toString() << '\n';
            }
            return 0;
        }
        if (sub == "set" && args.size() == 4) {
            const QString e = cfg.set(args.at(2), args.at(3));
            if (!e.isEmpty())
                return fail(err, e, 2);
            out << args.at(2) << " = " << cfg.value(args.at(2)).toString() << '\n';
            return 0;
        }
        if (sub == "reset" && args.size() == 3) {
            const QString e = cfg.reset(args.at(2));
            if (!e.isEmpty())
                return fail(err, e, 2);
            out << args.at(2) << " = " << cfg.value(args.at(2)).toString() << " (default)\n";
            return 0;
        }
        return fail(err, "usage: reelo config show|set KEY VALUE|reset KEY", 2);
    }

    if (cmd == "default") {
        if (sub == "status" && args.size() == 2) {
            const QJsonObject s = defaultStatus();
            if (asJson) {
                json(out, s);
            } else {
                out << (s.value("default").toBool() ? "reelo is the default player\n"
                                                    : "reelo is not the default for every type\n");
                out << "video: " << (s.value("video").toString().isEmpty() ? "none" : s.value("video").toString())
                    << "\naudio: " << (s.value("audio").toString().isEmpty() ? "none" : s.value("audio").toString())
                    << '\n';
            }
            return 0;
        }
        if (sub == "set" && args.size() == 2) {
            const QString e = makeDefault();
            if (!e.isEmpty())
                return fail(err, e);
            if (asJson)
                json(out, defaultStatus());
            else
                out << "reelo is now the default for video and audio\n";
            return 0;
        }
        return fail(err, "usage: reelo default status|set", 2);
    }

    if (cmd == "library" && args.size() == 1) {
        Config cfg;
        const QJsonArray items = scanLibrary(cfg.text("library"));
        if (asJson) {
            json(out, {{"folder", cfg.text("library")}, {"items", items}});
        } else {
            for (const auto &v : items)
                out << v.toObject().value("path").toString() << '\n';
        }
        return 0;
    }

    return fail(err, "unknown command; see reelo --help", 2);
}

} // namespace reelo
