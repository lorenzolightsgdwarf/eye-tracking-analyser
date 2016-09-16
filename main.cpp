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

    QCommandLineParser parser;
    parser.setApplicationDescription("Eye Fixation Parser");
    parser.addHelpOption();

    QCommandLineOption videoFile("video", "Video file","Video file");
    parser.addOption(videoFile);

    QCommandLineOption fixationFile("fixation", "Fixation file","Fixation file");
    parser.addOption(fixationFile);

    QCommandLineOption participantOption("participant", "Participant","Participant");
    parser.addOption(participantOption);

    QCommandLineOption mode("mode", "Mode: split/analyse","Mode");
    parser.addOption(mode);

    QCommandLineOption subtitles("subs", "Generate subtitles");
    parser.addOption(subtitles);

    parser.process(app);

    if(!parser.isSet(videoFile) || !parser.isSet(fixationFile) || !parser.isSet(mode)){
        qDebug()<<"Missing options";
        return -1;
    }
    if(parser.value(mode)!="split" && parser.value(mode)!="analyse"){
        qDebug()<<"Invalid mode";
        return -1;
    }
    if(parser.value(mode)=="split" && !parser.isSet(participantOption)){
        qDebug()<<"Missing Participant";
        return-1;
    }

    QString participant=parser.value(participantOption);
    QString structure_file;

    if(parser.value(mode)=="analyse"){
        QString video_file_name=parser.value(videoFile);
        QFileInfo video_file_info(video_file_name);
        QStringList parts=video_file_info.baseName().split("_");
        participant=parts[1];
        if(parts[5]=="Howe")
            structure_file="/home/chili/QTProjects/staTIc/Scripts/Howe/Howe.structure";
        else if (parts[5]=="Vault")
            structure_file="/home/chili/QTProjects/staTIc/Scripts/Vault/Vault.structure";
        else if(parts[5]=="Gazebo")
            structure_file="/home/chili/QTProjects/staTIc/Scripts/Gazebo/Gazebo.structure";
        else if (parts[5]=="Roof")
            structure_file="/home/chili/QTProjects/staTIc/Scripts/Full_roof/Full_roof.structure";
        if(participant.isEmpty() || structure_file.isEmpty()){
            qDebug()<<"Can't extract participant or structure file";
            return -1;
        }
    }

    qmlRegisterType<EyeTrackingEventGenerator>("EyeTrackingEventGenerator",1,0,"EyeTrackingEventGenerator");
    qmlRegisterType<SceneWalker>("SceneWalker",1,0,"SceneWalker");
    qmlRegisterType<SynchLoop>("SynchLoop",1,0,"SynchLoop");

    QQuickView view;

    view.resize(200, 200);
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.setSource(QUrl("qrc:/main_synch_loop.qml"));

//    QObject *rootObject = view.rootObject();
//    EyeTrackingEventGenerator *event_generator = qobject_cast<EyeTrackingEventGenerator*>(rootObject->findChild<QObject*>("event_generator"));
//    event_generator->setApplicationAndReceiver(&app,&view);

    view.show();

    view.rootContext()->setContextProperty("video_file",parser.value(videoFile));
    view.rootContext()->setContextProperty("mode",parser.value(mode));
    view.rootContext()->setContextProperty("participant",participant);
    view.rootContext()->setContextProperty("gaze_data_file",parser.value(fixationFile));
    view.rootContext()->setContextProperty("structure_file",structure_file);
    if(parser.isSet(subtitles))
        view.rootContext()->setContextProperty("enable_subs",true);
    else
        view.rootContext()->setContextProperty("enable_subs",false);

    QObject::connect(view.rootObject(), SIGNAL(quit()), qApp, SLOT(quit()));

    return app.exec();
}
