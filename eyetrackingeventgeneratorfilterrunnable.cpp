#include "eyetrackingeventgeneratorfilterrunnable.h"

EyeTrackingEventGeneratorFilterRunnable::EyeTrackingEventGeneratorFilterRunnable(QObject *parent):
    QObject(parent)
{
    m_video_filter=Q_NULLPTR;
}

void EyeTrackingEventGeneratorFilterRunnable::setVideoFilter(EyeTrackingEventGenerator *vf)
{
    m_video_filter=vf;

}

QVideoFrame EyeTrackingEventGeneratorFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, QVideoFilterRunnable::RunFlags flags)
{
    if(m_video_filter)
        m_video_filter->increasePosition();
    return *input;
}


