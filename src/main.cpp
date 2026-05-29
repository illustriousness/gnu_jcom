// #include <iostream>

// int main() {
//     std::cout << "Hello Ninja + CMake!" << std::endl;
//     return 0;
// }

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "McuReportModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    McuReportModel reportModel;
    reportModel.startMock(); // 无硬件时用模拟数据预览界面

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("report", &reportModel);

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
