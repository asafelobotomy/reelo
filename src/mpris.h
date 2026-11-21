// MPRIS 2 (org.mpris.MediaPlayer2 and .Player) on the session bus, so uQ's media widget,
// playerctl and the media keys see and control reelo.
#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QVariantMap>

namespace reelo {

class App;
class MpvObject;

class MprisRoot : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool CanSetFullscreen READ canSetFullscreen)
    Q_PROPERTY(bool Fullscreen READ fullscreen WRITE setFullscreen)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ uriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ mimeTypes)

public:
    MprisRoot(App *app, QObject *parent);
    bool canQuit() const { return true; }
    bool canRaise() const { return true; }
    bool canSetFullscreen() const { return true; }
    bool fullscreen() const;
    void setFullscreen(bool on);
    bool hasTrackList() const { return false; }
    QString identity() const { return QStringLiteral("reelo"); }
    QString desktopEntry() const { return QStringLiteral("io.github.asafelobotomy.reelo"); }
    QStringList uriSchemes() const;
    QStringList mimeTypes() const;

public slots:
    void Raise();
    void Quit();

private:
    App *m_app;
};

class MprisPlayer : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ shuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ canControl)

public:
    MprisPlayer(App *app, QObject *parent);

    QString playbackStatus() const;
    QString loopStatus() const { return QStringLiteral("None"); }
    double rate() const;
    void setRate(double r);
    bool shuffle() const { return false; }
    QVariantMap metadata() const;
    double volume() const;
    void setVolume(double v);
    qlonglong position() const;
    double minimumRate() const { return 0.25; }
    double maximumRate() const { return 4.0; }
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const { return canPlay(); }
    bool canSeek() const;
    bool canControl() const { return true; }

    /// Sends PropertiesChanged for these properties (current values).
    void changed(const QStringList &names);

public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong offsetUs);
    void SetPosition(const QDBusObjectPath &track, qlonglong positionUs);
    void OpenUri(const QString &uri);

signals:
    void Seeked(qlonglong position);

private:
    MpvObject *mpv() const;
    QDBusObjectPath trackId() const;
    App *m_app;
};

/// reelo's own call for a second start: Open(inputs) plays what it was given in this window
/// (nothing given: just comes forward). Sits beside MPRIS on the primary instance's object.
class WindowAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.asafelobotomy.reelo.Window")

public:
    WindowAdaptor(App *app, QObject *parent);

public slots:
    void Open(const QStringList &inputs);

private:
    App *m_app;
};

/// A second start of the window: hands `inputs` (made absolute against this start's folder)
/// to the reelo already running in the session and returns true, so this start exits; false
/// when there is none (or it doesn't answer), and this start opens its own window.
bool forwardToRunning(const QStringList &inputs);

/// Registers the adaptors at /org/mpris/MediaPlayer2 and claims
/// org.mpris.MediaPlayer2.reelo (or .reelo.instance<pid> when another reelo has it).
void startMpris(App *app);

} // namespace reelo
