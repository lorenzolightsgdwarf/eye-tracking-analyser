#include <QApplication>
#include <QtQml>
#include <QQuickView>
#include "eyetrackingeventgenerator.h"
#include "scenewalker.h"
#include "synchloop.h"
#include <QtQuick>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qmlRegisterType<EyeTrackingEventGenerator>("EyeTrackingEventGenerator",1,0,"EyeTrackingEventGenerator");
    qmlRegisterType<SceneWalker>("SceneWalker",1,0,"SceneWalker");
    qmlRegisterType<SynchLoop>("SynchLoop",1,0,"SynchLoop");

    QQuickView view;

    view.resize(1290, 960);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:/main_synch_loop.qml"));

//    QObject *rootObject = view.rootObject();
//    EyeTrackingEventGenerator *event_generator = qobject_cast<EyeTrackingEventGenerator*>(rootObject->findChild<QObject*>("event_generator"));
//    event_generator->setApplicationAndReceiver(&app,&view);

    view.show();

    return app.exec();
}
