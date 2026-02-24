#include <QApplication>
#include <QIcon>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <QtGlobal>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <fcntl.h>
#endif

#include "main/main_window.h"
#include "core/session_manager.h"

#include <QSettings>
#include <cstdlib>

int main(int argc, char *argv[]) {
    
    {
        QSettings settings("CodePlace", "CodePlaceEditor");
        double scale = settings.value("ui/scale", 1.0).toDouble();
        if (scale != 1.0) {
            qputenv("QT_SCALE_FACTOR", QByteArray::number(scale));
        }
    }
    
    QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus, true);
    
#ifdef Q_OS_LINUX
    bool shouldDetach = true;
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--no-detach" || arg == "-h" || arg == "--help" || arg == "-v" || arg == "--version") {
            shouldDetach = false;
            break;
        }
    }

    if (shouldDetach && (isatty(STDIN_FILENO) || isatty(STDOUT_FILENO) || isatty(STDERR_FILENO))) {
        pid_t pid = fork();
        if (pid > 0) {
            return 0;
        } else if (pid == 0) {
            setsid();
            int devNull = open("/dev/null", O_RDWR);
            if (devNull != -1) {
                dup2(devNull, STDIN_FILENO);
                dup2(devNull, STDOUT_FILENO);
                dup2(devNull, STDERR_FILENO);
                close(devNull);
            }
        }
    }
#endif

    QApplication app(argc, argv);
    app.setApplicationName("CodePlaceEditor");
    app.setApplicationDisplayName("CodePlace Editor");
    app.setOrganizationName("CodePlace");
    app.setOrganizationDomain("https://codeplace.net");
    app.setApplicationVersion("0.1.1-pre1");
    app.setWindowIcon(QIcon(":/resources/codeplace.png"));

    QCommandLineParser parser;
    parser.setApplicationDescription("CodePlace Editor - A modern, fast, and lightweight C++ code editor.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("path", "File or folder to open.");

    QCommandLineOption noDetachOption("no-detach", "Do not detach from the terminal.");
    parser.addOption(noDetachOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    

    Main::MainWindow mainWindow;

    if (!args.isEmpty()) {
        QString path = args.first();
        QFileInfo info(path);
        if (info.isDir()) {
            mainWindow.openFolder(QDir(path).absolutePath());
        } else {
            mainWindow.openFile(info.absoluteFilePath());
        }
    } else {
        
    }

    mainWindow.show();

    if (args.isEmpty()) {
        
        QMetaObject::invokeMethod(&mainWindow, "loadSession", Qt::QueuedConnection);
    }

    return app.exec();
}
