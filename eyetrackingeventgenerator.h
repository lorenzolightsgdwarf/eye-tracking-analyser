#ifndef EYETRACKINGEVENTGENERATOR_H
#define EYETRACKINGEVENTGENERATOR_H

#include <QObject>
#include <QGuiApplication>
#include <QAbstractVideoFilter>

class EyeTrackingEventGenerator : public QAbstractVideoFilter
{
    Q_OBJECT
    Q_PROPERTY(qreal frameRate READ frameRate WRITE setFrameRate)
public:
    explicit EyeTrackingEventGenerator(QObject *parent = 0);
    void increasePosition();
    void setApplicationAndReceiver(QGuiApplication* app,QObject* receiver);
    QVideoFilterRunnable * createFilterRunnable();
    void setFrameRate(qreal frameRate);
    qreal frameRate(){return m_frameRate;}
signals:

public slots:
private:

  qint64 m_video_pos;
  QGuiApplication* m_application;
  QObject* m_receiver;
  qreal m_frameRate;
};

#endif // EYETRACKINGEVENTGENERATOR_H
