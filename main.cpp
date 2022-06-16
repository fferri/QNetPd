#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "osc.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<OSC>("OSC", 1, 0, "OSC");

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/QNetPd/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
