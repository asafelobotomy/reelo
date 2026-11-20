#include "app.h"
#include "defaultapp.h"
#include "inhibit.h"
#include "library.h"
#include "media.h"
#include "mpris.h"
#include "mpvobject.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QDir>
#include <QFileInfo>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

namespace reelo {

static App *s_instance = nullptr;

App *App::create(QQmlEngine *, QJSEngine *)
{
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

App::App(const QStringList &initial, QObject *parent)
    : QObject(parent), m_initial(initial), m_inhibitor(new Inhibitor(this))
{
    s_instance = this;
    loadSettings();
    refreshLibrary();
    updateDefault();

    // The settings file is replaced on save, so watch its folder as well.
    const QString cfg = m_config.path();
    QDir().mkpath(QFileInfo(cfg).absolutePath());
    m_configWatch.addPath(QFileInfo(cfg).absolutePath());
    if (QFileInfo::exists(cfg))
        m_configWatch.addPath(cfg);
    auto reload = [this, cfg] {
        if (QFileInfo::exists(cfg) && !m_configWatch.files().contains(cfg))
            m_configWatch.addPath(cfg);
        const QVariantMap before = m_settings;
        loadSettings();
        if (before != m_settings) {
            applySettings();
            if (before.value("library") != m_settings.value("library"))
                refreshLibrary();
        }
    };
    connect(&m_configWatch, &QFileSystemWatcher::fileChanged, this, reload);
    connect(&m_configWatch, &QFileSystemWatcher::directoryChanged, this, reload);

    connect(&m_playlist, &Playlist::currentChanged, this, &App::playCurrent);
    // While the window (and mpv) still exist: keep where the file was for next time.
    if (auto *core = QCoreApplication::instance())
        connect(core, &QCoreApplication::aboutToQuit, this, [this] {
            if (m_player)
                m_player->rememberPosition();
            m_inhibitor->set(false);
        });
    startMpris(this);
}

App::~App() = default;

void App::loadSettings()
{
    m_config.load();
    QVariantMap s;
    for (const Key &k : keys())
        s.insert(k.name, m_config.value(k.name));
    if (s != m_settings) {
        m_settings = s;
        emit settingsChanged();
    }
}

void App::applySettings()
{
    if (!m_player)
        return;
    const QString watchLater = stateDir() + "/watch_later";
    QDir().mkpath(watchLater);
    m_player->set("watch-later-dir", watchLater);
    m_player->set("watch-later-options", "start,vid,aid,sid,sub-delay,audio-delay,speed");
    m_player->set("resume-playback", m_config.boolean("resume"));
    m_player->set("hwdec", m_config.text("hwdec") == "auto" ? "auto-safe" : "no");
    const bool subs = m_config.text("subtitles") == "auto";
    m_player->set("sub-auto", subs ? "fuzzy" : "no");
    m_player->set("sid", subs ? "auto" : "no");
    m_player->set("slang", m_config.text("sub_lang"));
    m_player->set("alang", m_config.text("audio_lang"));
    updateInhibit();
}

void App::setPlayer(MpvObject *p)
{
    if (p == m_player)
        return;
    m_player = p;
    if (p) {
        applySettings();
        connect(p, &MpvObject::endOfFile, this, &App::onEnd);
        connect(p, &MpvObject::pausedChanged, this, &App::updateInhibit);
        connect(p, &MpvObject::tracksChanged, this, &App::updateInhibit);
        connect(p, &MpvObject::idleChanged, this, &App::updateInhibit);
        connect(p, &MpvObject::errorChanged, this, [this] {
            if (!m_player->error().isEmpty())
                emit notice(tr("Couldn't play this: %1").arg(m_player->error()));
        });
        if (m_mpris) {
            auto status = [this] { m_mpris->changed({"PlaybackStatus", "CanPlay", "CanPause"}); };
            connect(p, &MpvObject::pausedChanged, this, status);
            connect(p, &MpvObject::eofChanged, this, status);
            connect(p, &MpvObject::idleChanged, this, status);
            auto meta = [this] { m_mpris->changed({"Metadata", "CanSeek"}); };
            connect(p, &MpvObject::metadataChanged, this, meta);
            connect(p, &MpvObject::mediaTitleChanged, this, meta);
            connect(p, &MpvObject::durationChanged, this, meta);
            connect(p, &MpvObject::seekableChanged, this, meta);
            connect(p, &MpvObject::volumeChanged, this, [this] { m_mpris->changed({"Volume"}); });
            connect(p, &MpvObject::speedChanged, this, [this] { m_mpris->changed({"Rate"}); });
            connect(p, &MpvObject::seeked, this, [this] {
                emit m_mpris->Seeked(qlonglong(m_player->position() * 1e6));
            });
            connect(&m_playlist, &Playlist::currentChanged, this, [this] {
                m_mpris->changed({"Metadata", "PlaybackStatus", "CanGoNext", "CanGoPrevious",
                                  "CanPlay", "CanPause"});
            });
        }
        // Files from the command line start once mpv is there.
        if (!m_initial.isEmpty()) {
            const QStringList files = m_initial;
            m_initial.clear();
            QTimer::singleShot(0, this, [this, files] {
                if (!open(files))
                    emit notice(tr("Nothing playable in what was opened"));
            });
        }
    }
    emit playerChanged();
}

void App::setFullscreen(bool on)
{
    if (on == m_fullscreen)
        return;
    m_fullscreen = on;
    emit fullscreenChanged();
}

bool App::open(const QStringList &inputs)
{
    QStringList expanded;
    for (const QString &in : inputs) {
        // A folder plays its media, in order.
        const QString local = in.startsWith("file://") ? QUrl(in).toLocalFile() : expandHome(in);
        if (!in.contains("://") || in.startsWith("file://")) {
            const QFileInfo fi(local);
            if (fi.isDir()) {
                QStringList names = QDir(local).entryList(QDir::Files | QDir::Readable);
                naturalSort(names);
                for (const QString &n : names)
                    if (isMedia(n))
                        expanded.append(QDir(local).absoluteFilePath(n));
                continue;
            }
        }
        expanded.append(in);
    }
    const bool withFolder = m_config.boolean("autoplay_next");
    // Playlist::open always moves the current entry, so playCurrent follows.
    return m_playlist.open(expanded, withFolder);
}

void App::enqueue(const QStringList &inputs) { m_playlist.append(inputs); }

void App::next()
{
    if (!m_playlist.next() && m_player)
        m_player->seekTo(m_player->duration()); // the end of the last one
}

void App::previous()
{
    // Like most players: back to the start first, then the one before.
    if (m_player && m_player->position() > 3) {
        m_player->seekTo(0);
        return;
    }
    if (!m_playlist.previous() && m_player)
        m_player->seekTo(0);
}

void App::playAt(int index)
{
    if (index == m_playlist.currentIndex())
        playCurrent();
    else
        m_playlist.playAt(index);
}

void App::close()
{
    if (m_player) {
        m_player->rememberPosition();
        m_player->stop();
    }
    m_playlist.clear();
    refreshLibrary();
}

void App::playCurrent()
{
    if (!m_player)
        return;
    const QString item = m_playlist.current();
    if (item.isEmpty()) {
        m_player->stop();
        return;
    }
    if (m_player->path() != item)
        m_player->rememberPosition();
    m_player->load(item);
}

void App::onEnd()
{
    if (m_player)
        m_player->forgetPosition();
    if (m_config.boolean("autoplay_next") && m_playlist.hasNext())
        m_playlist.next();
}

void App::updateInhibit()
{
    const bool on = m_player && m_config.boolean("keep_awake") && !m_player->idle()
                    && !m_player->paused() && m_player->hasVideo();
    m_inhibitor->set(on);
}

void App::refreshLibrary()
{
    m_library = scanLibrary(m_config.text("library")).toVariantList();
    emit libraryChanged();
}

void App::updateDefault()
{
    const bool d = defaultStatus().value("default").toBool();
    if (d != m_isDefault) {
        m_isDefault = d;
        emit defaultChanged();
    }
}

void App::makeDefault()
{
    const QString e = reelo::makeDefault();
    if (!e.isEmpty())
        emit notice(e);
    updateDefault();
}

void App::raise()
{
    if (m_window) {
        m_window->show();
        m_window->raise();
        m_window->requestActivate();
    }
}

void App::quit() { QCoreApplication::quit(); }

} // namespace reelo
