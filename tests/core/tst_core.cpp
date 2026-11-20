// Core tests: settings, queue, library, theme tokens, default app. No window, no mpv.
#include "config.h"
#include "defaultapp.h"
#include "library.h"
#include "media.h"
#include "playlist.h"
#include "theme.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>

using namespace reelo;

static void touch(const QString &path, const QByteArray &data = "x")
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

class TstCore : public QObject {
    Q_OBJECT
private slots:
    void kinds()
    {
        QCOMPARE(kindOf("/a/b.MKV"), Kind::Video);
        QCOMPARE(kindOf("song.flac"), Kind::Audio);
        QCOMPARE(kindOf("subs.srt"), Kind::None);
        QCOMPARE(kindOf("noext"), Kind::None);
    }

    void naturalOrder()
    {
        QStringList n{"ep10.mkv", "Ep2.mkv", "ep1.mkv"};
        naturalSort(n);
        QCOMPARE(n, QStringList({"ep1.mkv", "Ep2.mkv", "ep10.mkv"}));
    }

    void configDefaultsAndValidation()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("c/config.json");
        Config c(path);
        QCOMPARE(c.boolean("resume"), true);
        QCOMPARE(c.text("hwdec"), QString("auto"));
        QCOMPARE(c.text("library"), QString("~/Downloads"));
        QVERIFY(c.set("resume", "off").isEmpty());
        QVERIFY(c.set("hwdec", "off").isEmpty());
        QVERIFY(!c.set("hwdec", "vaapi").isEmpty());
        QVERIFY(!c.set("sub_lang", "en;rm -rf").isEmpty());
        QVERIFY(c.set("sub_lang", "en,eng").isEmpty());
        QVERIFY(!c.set("nope", "1").isEmpty());
        QVERIFY(!c.set("library", "relative/dir").isEmpty());
        Config again(path);
        QCOMPARE(again.boolean("resume"), false);
        QCOMPARE(again.text("hwdec"), QString("off"));
        QCOMPARE(again.text("sub_lang"), QString("en,eng"));
        QVERIFY(again.reset("resume").isEmpty());
        Config third(path);
        QCOMPARE(third.boolean("resume"), true);
        const QJsonArray list = third.describe().value("settings").toArray();
        QCOMPARE(list.size(), keys().size());
    }

    void configIgnoresBadFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("config.json");
        touch(path, "{\"resume\": \"maybe\", \"hwdec\": \"off\", \"junk\": 1, \"keep_awake\": false}");
        Config c(path);
        QCOMPARE(c.boolean("resume"), true);
        QCOMPARE(c.text("hwdec"), QString("off"));
        QCOMPARE(c.boolean("keep_awake"), false);
        touch(path, "not json");
        c.load();
        QCOMPARE(c.text("hwdec"), QString("auto"));
    }

    void playlistFolderAndNavigation()
    {
        QTemporaryDir dir;
        for (const char *n : {"b 10.mkv", "b 2.mkv", "b 1.mp4", "notes.txt", "b 3.srt", "song.mp3"})
            touch(dir.filePath(n));
        Playlist p;
        QSignalSpy cur(&p, &Playlist::currentChanged);
        QVERIFY(p.open({dir.filePath("b 2.mkv")}));
        QCOMPARE(p.count(), 4); // the media of the folder, not .txt or .srt
        QCOMPARE(p.current(), dir.filePath("b 2.mkv"));
        QCOMPARE(p.currentIndex(), 1);
        QVERIFY(cur.count() >= 1);
        QVERIFY(p.next());
        QCOMPARE(p.current(), dir.filePath("b 10.mkv"));
        QVERIFY(p.next());
        QCOMPARE(p.current(), dir.filePath("song.mp3"));
        QVERIFY(!p.next());
        QVERIFY(p.previous());
        p.remove(2); // the current one: what took its place plays
        QCOMPARE(p.current(), dir.filePath("song.mp3"));
        p.clear();
        QCOMPARE(p.currentIndex(), -1);
        QVERIFY(!p.open({dir.filePath("missing.mkv"), dir.path()}));
        QVERIFY(p.open({dir.filePath("b 2.mkv")}, false));
        QCOMPARE(p.count(), 1);
    }

    void playlistUrls()
    {
        QCOMPARE(Playlist::normalise("https://example.org/v.m3u8"), QString("https://example.org/v.m3u8"));
        QVERIFY(Playlist::normalise("javascript://x").isEmpty());
        QVERIFY(Playlist::normalise("file:///nonexistent/x.mkv").isEmpty());
        Playlist p;
        p.append({"https://example.org/a", "ftp://nope"});
        QCOMPARE(p.count(), 1);
        QCOMPARE(p.currentIndex(), 0);
    }

    void library()
    {
        QTemporaryDir dir;
        touch(dir.filePath("old.mkv"));
        QDir(dir.path()).mkpath("Channel");
        QDir(dir.path()).mkpath(".hidden");
        touch(dir.filePath("Channel/new.opus"));
        touch(dir.filePath(".hidden/secret.mkv"));
        touch(dir.filePath("doc.pdf"));
        touch(dir.filePath("empty.mp4"), "");
        {
            QFile old(dir.filePath("old.mkv"));
            QVERIFY(old.open(QIODevice::ReadWrite));
            QVERIFY(old.setFileTime(QDateTime::currentDateTime().addDays(-3),
                                    QFileDevice::FileModificationTime));
        }
        const QJsonArray items = scanLibrary(dir.path());
        QCOMPARE(items.size(), 2);
        QCOMPARE(items.at(0).toObject().value("name").toString(), QString("new"));
        QCOMPARE(items.at(0).toObject().value("folder").toString(), QString("Channel"));
        QCOMPARE(items.at(0).toObject().value("kind").toString(), QString("audio"));
        QCOMPARE(items.at(1).toObject().value("kind").toString(), QString("video"));
        QVERIFY(scanLibrary(dir.filePath("missing")).isEmpty());
    }

    void tokens()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("tokens.json");
        QCOMPARE(loadTokens(path).value("present").toBool(), false);
        touch(path, "{\"colors\":{\"bg\":\"#1f1a2b\",\"accent\":\"#b48ead\",\"text\":\"red\"},\"ui\":{\"radius\":10},\"fonts\":{\"ui\":\"Inter\",\"size\":11}}");
        const QVariantMap t = loadTokens(path);
        QCOMPARE(t.value("present").toBool(), true);
        QCOMPARE(t.value("colors").toMap().value("bg").toString(), QString("#1f1a2b"));
        QVERIFY(!t.value("colors").toMap().contains("text")); // not a colour
        QCOMPARE(t.value("radius").toInt(), 10);
        QCOMPARE(t.value("fontUi").toString(), QString("Inter"));
    }

    void defaults()
    {
        QTemporaryDir dir;
        const QString user = dir.filePath("mimeapps.list");
        const QString sys = dir.filePath("sys.list");
        touch(sys, "[Default Applications]\nvideo/mp4=vlc.desktop\naudio/mpeg=vlc.desktop;mpv.desktop\n");
        QJsonObject s = defaultStatus(user, {sys});
        QCOMPARE(s.value("default").toBool(), false);
        QCOMPARE(s.value("video").toString(), QString("vlc.desktop"));
        touch(user, "[Added Associations]\ntext/plain=kate.desktop;\n\n[Default Applications]\n"
                    "text/plain=org.kde.kate.desktop\nvideo/mp4=vlc.desktop\n\n[Removed Associations]\n");
        QVERIFY(makeDefault(user).isEmpty());
        s = defaultStatus(user, {sys});
        QCOMPARE(s.value("default").toBool(), true);
        QCOMPARE(s.value("video").toString(), QString(kDesktopId));
        QFile f(user);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(f.readAll());
        QVERIFY(text.contains("text/plain=org.kde.kate.desktop"));
        QVERIFY(text.contains("[Added Associations]\ntext/plain=kate.desktop;"));
        QVERIFY(text.contains("[Removed Associations]"));
        QCOMPARE(text.count("video/mp4="), 1);
        // The new lines go inside [Default Applications], before [Removed Associations].
        QVERIFY(text.indexOf("audio/mpeg=") < text.indexOf("[Removed Associations]"));
        // A file without the section gets one.
        QCOMPARE(withDefaults("", {"video/mp4"}, "x.desktop"),
                 QString("[Default Applications]\nvideo/mp4=x.desktop;\n"));
    }
};

QTEST_GUILESS_MAIN(TstCore)
#include "tst_core.moc"
