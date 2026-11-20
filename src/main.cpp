#include "app.h"
#include "cli.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTextStream>
#include <clocale>
#include <cstdio>

int main(int argc, char **argv)
{
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args.append(QString::fromLocal8Bit(argv[i]));

    if (reelo::isCliCommand(args)) {
        QCoreApplication core(argc, argv);
        QTextStream out(stdout), err(stderr);
        return reelo::runCli(args, out, err);
    }
    // "--" ends options: everything after it is a file, even "config".
    args.removeAll(QStringLiteral("--"));

    // mpv draws with OpenGL into the item's framebuffer.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QGuiApplication gui(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("reelo"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("reelo"));
    QGuiApplication::setApplicationVersion(QStringLiteral(REELO_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.asafelobotomy.reelo"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("io.github.asafelobotomy.reelo"),
                                                    QIcon(QStringLiteral(":/qt/qml/Reelo/icons/reelo.svg"))));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("Basic"));
    // libmpv needs C number formatting; Qt sets the locale from the environment.
    std::setlocale(LC_NUMERIC, "C");

    reelo::App app(args);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &gui,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Reelo", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    app.setWindow(qobject_cast<QQuickWindow *>(engine.rootObjects().first()));
    return QGuiApplication::exec();
}
