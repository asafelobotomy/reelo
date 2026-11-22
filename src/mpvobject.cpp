#include "mpvobject.h"

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QtQml/qqmlregistration.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>

namespace reelo {

namespace {

QVariant fromNode(const mpv_node &n)
{
    switch (n.format) {
    case MPV_FORMAT_STRING:
        return QString::fromUtf8(n.u.string);
    case MPV_FORMAT_FLAG:
        return bool(n.u.flag);
    case MPV_FORMAT_INT64:
        return qlonglong(n.u.int64);
    case MPV_FORMAT_DOUBLE:
        return n.u.double_;
    case MPV_FORMAT_NODE_ARRAY: {
        QVariantList l;
        for (int i = 0; i < n.u.list->num; ++i)
            l.append(fromNode(n.u.list->values[i]));
        return l;
    }
    case MPV_FORMAT_NODE_MAP: {
        QVariantMap m;
        for (int i = 0; i < n.u.list->num; ++i)
            m.insert(QString::fromUtf8(n.u.list->keys[i]), fromNode(n.u.list->values[i]));
        return m;
    }
    default:
        return {};
    }
}

void *procAddress(void *, const char *name)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    return ctx ? reinterpret_cast<void *>(ctx->getProcAddress(QByteArray(name))) : nullptr;
}

void onRenderUpdate(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<MpvObject *>(ctx), &MpvObject::frameReady,
                              Qt::QueuedConnection);
}

void onWakeup(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<QObject *>(ctx), "onEventsSlot", Qt::QueuedConnection);
}

} // namespace

class MpvRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit MpvRenderer(MpvObject *obj) : m_obj(obj) {}
    ~MpvRenderer() override
    {
        if (m_ctx)
            mpv_render_context_free(m_ctx);
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        if (!m_ctx) {
            mpv_opengl_init_params gl{procAddress, nullptr};
            QList<mpv_render_param> params = {
                {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl},
            };
#if QT_CONFIG(wayland)
            if (auto *wl = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>())
                params.append({MPV_RENDER_PARAM_WL_DISPLAY, wl->display()});
#endif
#if QT_CONFIG(xcb)
            if (auto *x = qGuiApp->nativeInterface<QNativeInterface::QX11Application>())
                params.append({MPV_RENDER_PARAM_X11_DISPLAY, x->display()});
#endif
            params.append({MPV_RENDER_PARAM_INVALID, nullptr});
            if (mpv_render_context_create(&m_ctx, m_obj->handle(), params.data()) < 0) {
                qWarning("[REELO-001] mpv could not start its OpenGL renderer");
                m_ctx = nullptr;
            } else {
                mpv_render_context_set_update_callback(m_ctx, onRenderUpdate, m_obj);
                QMetaObject::invokeMethod(m_obj, "markReady", Qt::QueuedConnection);
            }
        }
        return QQuickFramebufferObject::Renderer::createFramebufferObject(size);
    }

    void render() override
    {
        if (!m_ctx)
            return;
        QOpenGLFramebufferObject *fbo = framebufferObject();
        mpv_opengl_fbo target{int(fbo->handle()), fbo->width(), fbo->height(), 0};
        int flip = 0;
        mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &target},
                                     {MPV_RENDER_PARAM_FLIP_Y, &flip},
                                     {MPV_RENDER_PARAM_INVALID, nullptr}};
        QQuickWindow *w = m_obj->window();
        if (w)
            w->beginExternalCommands();
        mpv_render_context_render(m_ctx, params);
        if (w)
            w->endExternalCommands();
    }

private:
    MpvObject *m_obj;
    mpv_render_context *m_ctx = nullptr;
};

MpvObject::MpvObject(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
    m_mpv = mpv_create();
    if (!m_mpv)
        qFatal("reelo: could not create the mpv player");
    // Nothing from ~/.config/mpv: reelo's own settings decide (and the person's mpv.conf
    // might set a vo or keys that break the embedding).
    mpv_set_option_string(m_mpv, "config", "no");
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "terminal", "no");
    mpv_set_option_string(m_mpv, "msg-level", "all=warn");
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "audio-client-name", "reelo");
    mpv_set_option_string(m_mpv, "audio-display", "embedded-first");
    mpv_set_option_string(m_mpv, "ytdl", "yes");
    mpv_set_option_string(m_mpv, "screenshot-directory", "~~desktop/");
    // mpv's own on-screen scripts: reelo draws its own controls.
    for (const char *opt : {"load-stats-overlay", "load-console", "load-select", "load-positioning",
                            "load-commands", "load-context-menu", "load-auto-profiles"})
        mpv_set_option_string(m_mpv, opt, "no");
    // For debugging and tests: REELO_MPV_OPTIONS="ao=null,msg-level=all=v" (and terminal=yes).
    for (const QString &kv : qEnvironmentVariable("REELO_MPV_OPTIONS").split(',', Qt::SkipEmptyParts)) {
        const int eq = kv.indexOf('=');
        if (eq > 0)
            mpv_set_option_string(m_mpv, kv.left(eq).toUtf8().constData(),
                                  kv.mid(eq + 1).toUtf8().constData());
    }
    if (mpv_initialize(m_mpv) < 0)
        qFatal("reelo: could not start mpv");

    struct Watch { const char *name; mpv_format format; };
    static const Watch watched[] = {
        {"pause", MPV_FORMAT_FLAG},          {"idle-active", MPV_FORMAT_FLAG},
        {"time-pos", MPV_FORMAT_DOUBLE},     {"duration", MPV_FORMAT_DOUBLE},
        {"demuxer-cache-time", MPV_FORMAT_DOUBLE},
        {"volume", MPV_FORMAT_DOUBLE},       {"mute", MPV_FORMAT_FLAG},
        {"speed", MPV_FORMAT_DOUBLE},        {"media-title", MPV_FORMAT_STRING},
        {"path", MPV_FORMAT_STRING},         {"metadata", MPV_FORMAT_NODE},
        {"track-list", MPV_FORMAT_NODE},     {"eof-reached", MPV_FORMAT_FLAG},
        {"seekable", MPV_FORMAT_FLAG},       {"paused-for-cache", MPV_FORMAT_FLAG},
    };
    for (const Watch &w : watched)
        mpv_observe_property(m_mpv, 0, w.name, w.format);

    connect(this, &MpvObject::frameReady, this, &QQuickItem::update, Qt::QueuedConnection);
    mpv_set_wakeup_callback(m_mpv, onWakeup, this);
}

MpvObject::~MpvObject()
{
    mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
    // The renderer (and its render context) belongs to the scene graph; mpv_terminate_destroy
    // waits for it to be freed, so the core goes last.
    mpv_terminate_destroy(m_mpv);
}

QQuickFramebufferObject::Renderer *MpvObject::createRenderer() const
{
    window()->setPersistentGraphics(true);
    window()->setPersistentSceneGraph(true);
    return new MpvRenderer(const_cast<MpvObject *>(this));
}

bool MpvObject::hasVideo() const
{
    for (const QVariant &t : m_tracks) {
        const QVariantMap m = t.toMap();
        if (m.value("type") == "video" && m.value("selected").toBool()
            && !m.value("albumart").toBool())
            return true;
    }
    return false;
}

void MpvObject::set(const QString &name, const QVariant &value)
{
    const QByteArray n = name.toUtf8();
    int rc;
    switch (value.typeId()) {
    case QMetaType::Bool: {
        int f = value.toBool();
        rc = mpv_set_property(m_mpv, n.constData(), MPV_FORMAT_FLAG, &f);
        break;
    }
    case QMetaType::Int:
    case QMetaType::LongLong: {
        int64_t i = value.toLongLong();
        rc = mpv_set_property(m_mpv, n.constData(), MPV_FORMAT_INT64, &i);
        break;
    }
    case QMetaType::Double: {
        double d = value.toDouble();
        rc = mpv_set_property(m_mpv, n.constData(), MPV_FORMAT_DOUBLE, &d);
        break;
    }
    default:
        rc = mpv_set_property_string(m_mpv, n.constData(), value.toString().toUtf8().constData());
    }
    if (rc < 0)
        qWarning("[REELO-002] mpv refused %s: %s", n.constData(), mpv_error_string(rc));
}

QVariant MpvObject::get(const QString &name) const
{
    mpv_node node;
    if (mpv_get_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_NODE, &node) < 0)
        return {};
    QVariant v = fromNode(node);
    mpv_free_node_contents(&node);
    return v;
}

void MpvObject::command(const QVariantList &args)
{
    QList<QByteArray> bytes;
    for (const QVariant &a : args)
        bytes.append(a.typeId() == QMetaType::Double ? QByteArray::number(a.toDouble(), 'f', 3)
                                                     : a.toString().toUtf8());
    QList<const char *> argv;
    for (const QByteArray &b : bytes)
        argv.append(b.constData());
    argv.append(nullptr);
    mpv_command_async(m_mpv, 0, argv.data());
}

void MpvObject::markReady()
{
    m_ready = true;
    if (!m_pending.isEmpty()) {
        const QString p = std::exchange(m_pending, QString());
        load(p);
    }
}

void MpvObject::load(const QString &pathOrUrl)
{
    if (m_error.size()) {
        m_error.clear();
        emit errorChanged();
    }
    // Until the window has drawn once there is no render context, and mpv would start the
    // file without video.
    if (!m_ready) {
        m_pending = pathOrUrl;
        return;
    }
    command({"loadfile", pathOrUrl, "replace"});
    setPaused(false);
}

void MpvObject::stop()
{
    m_pending.clear();
    command({"stop"});
}
void MpvObject::seekTo(double s) { command({"seek", s, "absolute+keyframes"}); }
void MpvObject::seekBy(double s) { command({"seek", s, "relative"}); }

void MpvObject::selectTrack(const QString &type, int id)
{
    static const QHash<QString, QString> props = {{"sub", "sid"}, {"audio", "aid"}, {"video", "vid"}};
    if (!props.contains(type))
        return;
    set(props.value(type), id > 0 ? QVariant(QString::number(id)) : QVariant(QStringLiteral("no")));
}

void MpvObject::rememberPosition()
{
    // Synchronous: this runs right before another file loads, or before quitting.
    if (!m_idle && !m_eof && m_position > 5) {
        const char *argv[] = {"write-watch-later-config", nullptr};
        mpv_command(m_mpv, argv);
    }
}

void MpvObject::forgetPosition()
{
    if (!m_path.isEmpty())
        command({"delete-watch-later-config", m_path});
}

void MpvObject::setPaused(bool p)
{
    if (p != m_paused)
        set("pause", p);
}

void MpvObject::setVolume(int v)
{
    v = std::clamp(v, 0, 130);
    if (v != m_volume)
        set("volume", double(v));
}

void MpvObject::setMuted(bool m)
{
    if (m != m_muted)
        set("mute", m);
}

void MpvObject::setSpeed(double s)
{
    s = std::clamp(s, 0.25, 4.0);
    if (!qFuzzyCompare(s, m_speed))
        set("speed", s);
}

void MpvObject::onEvents()
{
    while (m_mpv) {
        mpv_event *ev = mpv_wait_event(m_mpv, 0);
        if (ev->event_id == MPV_EVENT_NONE)
            break;
        switch (ev->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto *p = static_cast<mpv_event_property *>(ev->data);
            handleProperty(p->name, p->format, p->data);
            break;
        }
        case MPV_EVENT_FILE_LOADED:
            emit fileLoaded();
            break;
        case MPV_EVENT_END_FILE: {
            auto *e = static_cast<mpv_event_end_file *>(ev->data);
            if (e->reason == MPV_END_FILE_REASON_ERROR) {
                m_error = QString::fromUtf8(mpv_error_string(e->error));
                emit errorChanged();
            }
            break;
        }
        case MPV_EVENT_SEEK:
            m_seeking = true;
            break;
        case MPV_EVENT_PLAYBACK_RESTART:
            if (m_seeking) {
                m_seeking = false;
                emit seeked();
            }
            break;
        default:
            break;
        }
    }
}

void MpvObject::handleProperty(const char *name, int format, void *data)
{
    const QByteArray n(name);
    const bool has = format != MPV_FORMAT_NONE && data;
    auto flag = [&] { return has && *static_cast<int *>(data) != 0; };
    auto num = [&] { return has ? *static_cast<double *>(data) : 0.0; };
    auto str = [&] { return has ? QString::fromUtf8(*static_cast<char **>(data)) : QString(); };
    auto node = [&] { return has ? fromNode(*static_cast<mpv_node *>(data)) : QVariant(); };

    if (n == "pause") {
        m_paused = flag();
        emit pausedChanged();
    } else if (n == "idle-active") {
        m_idle = flag();
        emit idleChanged();
    } else if (n == "time-pos") {
        m_position = num();
        emit positionChanged();
    } else if (n == "duration") {
        m_duration = num();
        emit durationChanged();
    } else if (n == "demuxer-cache-time") {
        m_cached = num();
        emit cachedChanged();
    } else if (n == "volume") {
        m_volume = int(std::lround(num()));
        emit volumeChanged();
    } else if (n == "mute") {
        m_muted = flag();
        emit mutedChanged();
    } else if (n == "speed") {
        m_speed = has ? num() : 1.0;
        emit speedChanged();
    } else if (n == "media-title") {
        m_title = str();
        emit mediaTitleChanged();
    } else if (n == "path") {
        m_path = str();
        emit pathChanged();
    } else if (n == "metadata") {
        m_metadata = node().toMap();
        emit metadataChanged();
    } else if (n == "track-list") {
        m_tracks = node().toList();
        emit tracksChanged();
    } else if (n == "eof-reached") {
        const bool was = m_eof;
        m_eof = flag();
        emit eofChanged();
        if (m_eof && !was)
            emit endOfFile();
    } else if (n == "seekable") {
        m_seekable = flag();
        emit seekableChanged();
    } else if (n == "paused-for-cache") {
        m_buffering = flag();
        emit bufferingChanged();
    }
}

} // namespace reelo
