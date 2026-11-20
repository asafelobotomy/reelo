// Keeps the screen on while a video plays, through org.freedesktop.ScreenSaver (hypridle and
// most desktops honour it).
#pragma once

#include <QObject>

namespace reelo {

class Inhibitor : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~Inhibitor() override { set(false); }
    void set(bool on);

private:
    uint m_cookie = 0;
    bool m_on = false;
};

} // namespace reelo
