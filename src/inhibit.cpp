#include "inhibit.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

namespace reelo {

void Inhibitor::set(bool on)
{
    if (on == m_on)
        return;
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    if (on) {
        QDBusMessage m = QDBusMessage::createMethodCall(
            "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver", "Inhibit");
        m << QStringLiteral("reelo") << QStringLiteral("Playing a video");
        const QDBusReply<uint> r = bus.call(m, QDBus::Block, 2000);
        if (!r.isValid())
            return; // nothing on the bus to ask; try again next time
        m_cookie = r.value();
        m_on = true;
    } else {
        QDBusMessage m = QDBusMessage::createMethodCall(
            "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver", "UnInhibit");
        m << m_cookie;
        bus.call(m, QDBus::NoBlock);
        m_on = false;
        m_cookie = 0;
    }
}

} // namespace reelo
