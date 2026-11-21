#include "mpris.h"
#include "app.h"
#include "media.h"
#include "mpvobject.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QDBusMessage>
#include <QFileInfo>
#include <QMetaProperty>
#include <QUrl>

namespace reelo {

static constexpr auto kPath = "/org/mpris/MediaPlayer2";

MprisRoot::MprisRoot(App *app, QObject *parent) : QDBusAbstractAdaptor(parent), m_app(app) {}

bool MprisRoot::fullscreen() const { return m_app->fullscreen(); }
void MprisRoot::setFullscreen(bool on) { m_app->setFullscreen(on); }
QStringList MprisRoot::uriSchemes() const { return {"file", "http", "https", "rtsp", "rtmp"}; }
QStringList MprisRoot::mimeTypes() const { return reelo::mimeTypes(); }
void MprisRoot::Raise() { m_app->raise(); }
void MprisRoot::Quit() { m_app->quit(); }

MprisPlayer::MprisPlayer(App *app, QObject *parent) : QDBusAbstractAdaptor(parent), m_app(app)
{
    setAutoRelaySignals(true);
}

MpvObject *MprisPlayer::mpv() const { return m_app->player(); }

QString MprisPlayer::playbackStatus() const
{
    MpvObject *p = mpv();
    if (!p || p->idle() || m_app->playlist()->currentIndex() < 0)
        return QStringLiteral("Stopped");
    return p->paused() || p->eof() ? QStringLiteral("Paused") : QStringLiteral("Playing");
}

double MprisPlayer::rate() const { return mpv() ? mpv()->speed() : 1.0; }
void MprisPlayer::setRate(double r)
{
    if (mpv() && r > 0)
        mpv()->setSpeed(r);
}

QDBusObjectPath MprisPlayer::trackId() const
{
    const int i = m_app->playlist()->currentIndex();
    return QDBusObjectPath(i < 0 ? QStringLiteral("/org/mpris/MediaPlayer2/TrackList/NoTrack")
                                 : QStringLiteral("/io/github/asafelobotomy/reelo/track/%1").arg(i));
}

QVariantMap MprisPlayer::metadata() const
{
    QVariantMap m;
    m.insert("mpris:trackid", QVariant::fromValue(trackId()));
    MpvObject *p = mpv();
    const QString item = m_app->playlist()->current();
    if (!p || item.isEmpty())
        return m;
    if (p->duration() > 0)
        m.insert("mpris:length", qlonglong(p->duration() * 1e6));
    const QVariantMap tags = p->metadata();
    auto tag = [&](const char *a, const char *b = nullptr) {
        for (auto it = tags.begin(); it != tags.end(); ++it)
            if (it.key().compare(a, Qt::CaseInsensitive) == 0
                || (b && it.key().compare(b, Qt::CaseInsensitive) == 0))
                return it.value().toString();
        return QString();
    };
    QString title = tag("title");
    if (title.isEmpty())
        title = p->mediaTitle();
    if (title.isEmpty())
        title = QFileInfo(item).completeBaseName();
    m.insert("xesam:title", title);
    const QString artist = tag("artist", "album_artist");
    if (!artist.isEmpty())
        m.insert("xesam:artist", QStringList{artist});
    const QString album = tag("album");
    if (!album.isEmpty())
        m.insert("xesam:album", album);
    m.insert("xesam:url", item.contains("://") ? item : QUrl::fromLocalFile(item).toString());
    return m;
}

double MprisPlayer::volume() const { return mpv() ? mpv()->volume() / 100.0 : 1.0; }
void MprisPlayer::setVolume(double v)
{
    if (mpv())
        mpv()->setVolume(int(std::lround(std::clamp(v, 0.0, 1.3) * 100)));
}

qlonglong MprisPlayer::position() const { return mpv() ? qlonglong(mpv()->position() * 1e6) : 0; }
bool MprisPlayer::canGoNext() const { return m_app->playlist()->hasNext(); }
bool MprisPlayer::canGoPrevious() const { return m_app->playlist()->currentIndex() >= 0; }
bool MprisPlayer::canPlay() const { return m_app->playlist()->currentIndex() >= 0; }
bool MprisPlayer::canSeek() const { return mpv() && mpv()->seekable(); }

void MprisPlayer::Next() { m_app->next(); }
void MprisPlayer::Previous() { m_app->previous(); }
void MprisPlayer::Pause()
{
    if (mpv())
        mpv()->setPaused(true);
}
void MprisPlayer::Play()
{
    if (!mpv() || !canPlay())
        return;
    if (mpv()->eof())
        mpv()->seekTo(0);
    mpv()->setPaused(false);
}
void MprisPlayer::PlayPause()
{
    if (mpv() && (mpv()->paused() || mpv()->eof()))
        Play();
    else
        Pause();
}
void MprisPlayer::Stop() { m_app->close(); }
void MprisPlayer::Seek(qlonglong offsetUs)
{
    if (canSeek())
        mpv()->seekBy(offsetUs / 1e6);
}
void MprisPlayer::SetPosition(const QDBusObjectPath &track, qlonglong positionUs)
{
    if (!canSeek() || track != trackId() || positionUs < 0
        || positionUs > qlonglong(mpv()->duration() * 1e6))
        return;
    mpv()->seekTo(positionUs / 1e6);
}
void MprisPlayer::OpenUri(const QString &uri)
{
    m_app->open({uri});
}

WindowAdaptor::WindowAdaptor(App *app, QObject *parent) : QDBusAbstractAdaptor(parent), m_app(app) {}

void WindowAdaptor::Open(const QStringList &inputs)
{
    if (!inputs.isEmpty() && !m_app->open(inputs))
        emit m_app->notice(QObject::tr("Nothing playable in what was opened"));
    m_app->raise();
}

static constexpr auto kName = "org.mpris.MediaPlayer2.reelo";

bool forwardToRunning(const QStringList &inputs)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected() || !bus.interface()->isServiceRegistered(kName))
        return false;
    // The running reelo has its own working folder: send absolute paths.
    QStringList absolute;
    for (const QString &in : inputs)
        absolute.append(in.contains(QStringLiteral("://")) ? in : QFileInfo(in).absoluteFilePath());
    QDBusInterface window(kName, kPath, QStringLiteral("io.github.asafelobotomy.reelo.Window"), bus);
    window.setTimeout(3000);
    const QDBusMessage reply = window.call(QStringLiteral("Open"), absolute);
    return reply.type() == QDBusMessage::ReplyMessage;
}

void MprisPlayer::changed(const QStringList &names)
{
    QVariantMap props;
    for (const QString &n : names) {
        const int i = metaObject()->indexOfProperty(n.toUtf8().constData());
        if (i >= 0)
            props.insert(n, metaObject()->property(i).read(this));
    }
    if (props.isEmpty())
        return;
    QDBusMessage msg = QDBusMessage::createSignal(kPath, "org.freedesktop.DBus.Properties",
                                                  "PropertiesChanged");
    msg << QStringLiteral("org.mpris.MediaPlayer2.Player") << props << QStringList();
    QDBusConnection::sessionBus().send(msg);
}

void startMpris(App *app)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    auto *holder = new QObject(app);
    new MprisRoot(app, holder);
    auto *player = new MprisPlayer(app, holder);
    new WindowAdaptor(app, holder);
    app->m_mpris = player;
    bus.registerObject(kPath, holder);
    QString name = QString::fromLatin1(kName);
    if (bus.interface()->isServiceRegistered(name))
        name += QStringLiteral(".instance%1").arg(QCoreApplication::applicationPid());
    if (!bus.registerService(name))
        qWarning("reelo: could not claim %s on the session bus", qPrintable(name));
}

} // namespace reelo
