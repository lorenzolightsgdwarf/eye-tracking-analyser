#include "eyetrackingeventgenerator.h"
#include <QMouseEvent>
#include <QWindow>
#include <QDebug>
#include "eyetrackingeventgeneratorfilterrunnable.h"
EyeTrackingEventGenerator::EyeTrackingEventGenerator(QObject *parent) : QAbstractVideoFilter(parent)
{
    m_video_pos=0;
    m_application=Q_NULLPTR;
    m_receiver=Q_NULLPTR;
    m_frameRate=1;
}

void EyeTrackingEventGenerator::increasePosition()
{
    m_video_pos++;
    qDebug()<<m_video_pos;
    if(m_application){
        QPointF mouse_position(320,240);
        QMouseEvent event(QEvent::MouseButtonPress, mouse_position,Qt::RightButton,Qt::RightButton,0);
        m_application->sendEvent(m_receiver, &event);
    }
}


void EyeTrackingEventGenerator::setApplicationAndReceiver(QGuiApplication *app,QObject* receiver)
{
    m_application=app;
    m_receiver=receiver;
}

QVideoFilterRunnable *EyeTrackingEventGenerator::createFilterRunnable()
{
    EyeTrackingEventGeneratorFilterRunnable* filter_runnable=new EyeTrackingEventGeneratorFilterRunnable();
    connect(this,SIGNAL(destroyed(QObject*)),filter_runnable,SLOT(deleteLater()));
    filter_runnable->setVideoFilter(this);
    return filter_runnable;
}

void EyeTrackingEventGenerator::setFrameRate(qreal frameRate)
{
    m_frameRate=frameRate;
}
