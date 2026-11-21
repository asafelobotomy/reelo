// The glue between the window, mpv, the queue and the settings. QML sees it as `App`.
#pragma once

#include "config.h"
#include "mpvobject.h"
#include "playlist.h"
#include "theme.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QQuickWindow>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace reelo {

class Inhibitor;
class MprisPlayer;

class App : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(reelo::Playlist *playlist READ playlist CONSTANT)
    Q_PROPERTY(uqtheme::Watcher *theme READ theme CONSTANT)
    Q_PROPERTY(reelo::MpvObject *player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList library READ library NOTIFY libraryChanged)
    Q_PROPERTY(QString libraryFolder READ libraryFolder NOTIFY settingsChanged)
    Q_PROPERTY(bool isDefault READ isDefault NOTIFY defaultChanged)
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit App(const QStringList &initial, QObject *parent = nullptr);
    ~App() override;
    /// The one App main() made; QML gets it as the `App` singleton.
    static App *create(QQmlEngine *, QJSEngine *);

    Playlist *playlist() { return &m_playlist; }
    uqtheme::Watcher *theme() { return &m_theme; }
    MpvObject *player() const { return m_player; }
    void setPlayer(MpvObject *p);
    QVariantMap settings() const { return m_settings; }
    QVariantList library() const { return m_library; }
    QString libraryFolder() const { return m_config.text("library"); }
    bool isDefault() const { return m_isDefault; }
    bool fullscreen() const { return m_fullscreen; }
    void setFullscreen(bool on);
    QString version() const { return QStringLiteral(REELO_VERSION); }
    void setWindow(QQuickWindow *w) { m_window = w; }

    /// Files, folders' media, file:// URLs or stream URLs. Replaces the queue.
    Q_INVOKABLE bool open(const QStringList &inputs);
    Q_INVOKABLE void enqueue(const QStringList &inputs);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void playAt(int index);
    /// Back to the start screen (stops playback, keeps where it was).
    Q_INVOKABLE void close();
    Q_INVOKABLE void refreshLibrary();
    Q_INVOKABLE void makeDefault();
    Q_INVOKABLE void raise();
    Q_INVOKABLE void quit();

signals:
    void playerChanged();
    void settingsChanged();
    void libraryChanged();
    void defaultChanged();
    void fullscreenChanged();
    /// Something to say for a moment over the video (opened nothing, a file failed...).
    void notice(const QString &text);

private:
    void loadSettings();
    void applySettings();
    void playCurrent();
    void onEnd();
    void updateInhibit();
    void updateDefault();
    QStringList m_initial;
    Config m_config;
    Playlist m_playlist;
    uqtheme::Watcher m_theme{tokensPath()};
    QFileSystemWatcher m_configWatch;
    QPointer<MpvObject> m_player;
    QPointer<QQuickWindow> m_window;
    Inhibitor *m_inhibitor;
    MprisPlayer *m_mpris = nullptr;
    QVariantMap m_settings;
    QVariantList m_library;
    bool m_isDefault = false;
    bool m_fullscreen = false;
    friend void startMpris(App *app);
};

} // namespace reelo
