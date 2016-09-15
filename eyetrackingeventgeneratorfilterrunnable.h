#ifndef EYETRACKINGEVENTGENERATORFILTERRUNNABLE_H
#define EYETRACKINGEVENTGENERATORFILTERRUNNABLE_H
#include <QObject>
#include <QVideoFilterRunnable>
#include "eyetrackingeventgenerator.h"
class EyeTrackingEventGeneratorFilterRunnable :public QObject ,public QVideoFilterRunnable
{
    Q_OBJECT
public:
    EyeTrackingEventGeneratorFilterRunnable(QObject* parent=0);
    void setVideoFilter(EyeTrackingEventGenerator* vf);
    QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags);
private:
    EyeTrackingEventGenerator* m_video_filter;
};

#endif // EYETRACKINGEVENTGENERATORFILTERRUNNABLE_H
