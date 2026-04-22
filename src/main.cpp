#include "appcontroller.h"

#include <QDebug>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("gnu_jcom"));
    app.setApplicationDisplayName(QStringLiteral("gnu_jcom"));
    app.setOrganizationName(QStringLiteral("gnu_jcom"));

    AppController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    QObject::connect(&engine, &QQmlEngine::warnings, &app, [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings) {
            qWarning().noquote() << warning.toString();
        }
    });

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("GnuJCom"), QStringLiteral("Main"));

    return app.exec();
}
