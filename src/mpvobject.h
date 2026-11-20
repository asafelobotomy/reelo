// libmpv in Qt Quick: mpv draws each frame into the item's OpenGL framebuffer through its render
// API (vo=libmpv), and the properties the UI needs are observed and exposed as Qt properties.
#pragma once

#include <QQuickFramebufferObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

struct mpv_handle;
struct mpv_render_context;

namespace reelo {

class MpvObject : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool idle READ idle NOTIFY idleChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(double cached READ cached NOTIFY cachedChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaTitleChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY tracksChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY tracksChanged)
    Q_PROPERTY(bool eof READ eof NOTIFY eofChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged)
    Q_PROPERTY(bool buffering READ buffering NOTIFY bufferingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit MpvObject(QQuickItem *parent = nullptr);
    ~MpvObject() override;
    Renderer *createRenderer() const override;

    bool paused() const { return m_paused; }
    bool idle() const { return m_idle; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    double cached() const { return m_cached; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    double speed() const { return m_speed; }
    QString mediaTitle() const { return m_title; }
    QString path() const { return m_path; }
    QVariantMap metadata() const { return m_metadata; }
    QVariantList tracks() const { return m_tracks; }
    bool hasVideo() const;
    bool eof() const { return m_eof; }
    bool seekable() const { return m_seekable; }
    bool buffering() const { return m_buffering; }
    QString error() const { return m_error; }

    void setPaused(bool p);
    void setVolume(int v);
    void setMuted(bool m);
    void setSpeed(double s);

    /// Sets an mpv option/property by name (strings, bools, numbers).
    Q_INVOKABLE void set(const QString &name, const QVariant &value);
    Q_INVOKABLE QVariant get(const QString &name) const;
    /// Runs an mpv command (["seek", 5, "relative"]) asynchronously.
    Q_INVOKABLE void command(const QVariantList &args);

    Q_INVOKABLE void load(const QString &pathOrUrl);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seekTo(double seconds);
    Q_INVOKABLE void seekBy(double seconds);
    /// "sub", "audio" or "video"; id 0 turns it off.
    Q_INVOKABLE void selectTrack(const QString &type, int id);
    /// Saves the resume position of what's playing (mpv's watch-later file).
    Q_INVOKABLE void rememberPosition();
    Q_INVOKABLE void forgetPosition();

    mpv_handle *handle() const { return m_mpv; }

signals:
    void pausedChanged();
    void idleChanged();
    void positionChanged();
    void durationChanged();
    void cachedChanged();
    void volumeChanged();
    void mutedChanged();
    void speedChanged();
    void mediaTitleChanged();
    void pathChanged();
    void metadataChanged();
    void tracksChanged();
    void eofChanged();
    void seekableChanged();
    void bufferingChanged();
    void errorChanged();
    /// A file finished on its own (not stopped or replaced).
    void endOfFile();
    /// A file started (after `load`).
    void fileLoaded();
    /// A seek finished (MPRIS sends Seeked).
    void seeked();
    void frameReady();

private slots:
    void onEventsSlot() { onEvents(); }
    /// The renderer has its OpenGL context: files can load now (vo=libmpv needs it).
    void markReady();

private:
    void onEvents();
    void handleProperty(const char *name, int format, void *data);
    mpv_handle *m_mpv = nullptr;
    bool m_paused = false;
    bool m_idle = true;
    double m_position = 0;
    double m_duration = 0;
    double m_cached = 0;
    int m_volume = 100;
    bool m_muted = false;
    double m_speed = 1.0;
    QString m_title;
    QString m_path;
    QVariantMap m_metadata;
    QVariantList m_tracks;
    bool m_eof = false;
    bool m_seekable = false;
    bool m_buffering = false;
    QString m_error;
    bool m_seeking = false;
    bool m_ready = false;
    QString m_pending;
    friend class MpvRenderer;
};

} // namespace reelo
