// #include <iostream>

// int main() {
//     std::cout << "Hello Ninja + CMake!" << std::endl;
//     return 0;
// }

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "McuReportModel.h"
#include "SerialPortController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    McuReportModel reportModel;
    SerialPortController serialController(&reportModel);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("report", &reportModel);
    engine.rootContext()->setContextProperty("serial", &serialController);

    // 开发时从源码目录加载 QML，改完文件重启即可，无需重新编译。
    // 发布时改回 qrc:/ 路径。
    const QUrl url(QUrl::fromLocalFile(QStringLiteral(PROJECT_SOURCE_DIR "/qml/Main.qml")));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );

    engine.load(url);

    return app.exec();
}
