#ifndef SYNCHLOOP_H
#define SYNCHLOOP_H

#include <QObject>
#include <AR/ar.h>
#include <opencv.hpp>
#include <QFile>
#include <QTextStream>
extern "C"{
#include <AR/ar.h>
#include <AR/config.h>
#include <AR/arFilterTransMat.h>
#include <AR/arMulti.h>
}
#include <QTime>
#include <QMatrix4x4>
#include <Qt3DCore/Qt3DCore>

struct AR3DMultiPatternObject {
    ARMultiMarkerInfoT* marker_info;
    /*In Opengl coordinate system*/

    QMatrix4x4 pose;

    /*....*/
    ARFilterTransMatInfo* ftmi;
    bool visible;
    inline AR3DMultiPatternObject& operator=(const AR3DMultiPatternObject& o) {
        marker_info=o.marker_info;
        pose=o.pose;
        visible=o.visible;
        ftmi=o.ftmi;
        return *this;
    }
};


class SynchLoop : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
public:
    explicit SynchLoop(QObject *parent = 0);

    Q_INVOKABLE void loadMesh(QString file_path, QString transform_name);
    Q_INVOKABLE void loadMultiMarkersConfigFile(QString config_name, QString file_path);
    Q_INVOKABLE void setWrite_subtititles(bool write_subtitles){this->write_subtitles=write_subtitles;}
    Q_INVOKABLE void run();
    Q_INVOKABLE void loadGazeData(QString file_name);
    Q_INVOKABLE void setFileVideoFileName(QString video_file_name);
    int position(){return frame_counter/fps;}
    Q_INVOKABLE void loadStructure(QString file_name);
signals:
    void positionChanged();
public slots:

private:

    QString mode="split";

    bool write_subtitles;
    QFile subtitle_file;
    QTextStream subtitles_stream;
    int subtitles_counter;
    int fps=24;
    QSize cameraSize;

    QString video_file_name,eye_events_file_name;

    cv::VideoCapture video_capture;

    long long frame_counter;
    long long tot_frame;

    ARParamLT* ar_param;
    ARHandle* ar_handle;
    AR3DHandle* ar_3d_handle;
    ARPattHandle* ar_patt_handle;
    QMatrix4x4 projectionMatrix;
    QVector4D distortionCoeff;

    Qt3DCore::QCamera camera;

    QHash<QString,AR3DMultiPatternObject*> ar_multimarker_objects;

    QHash<QString,QString> mesh_name2pose_name;
    QHash<QString, QVector<QVector3D> > mesh_vertices_objects;
    QHash<QString, QVector< QVector <int> > > mesh_faces_objects;

    QHash<QString,QVector3D> joints;
    QHash<QString, QPair<QVector3D,QVector3D> > beams;

    QMultiHash<long long, QPair<QVector2D,int>> fixations;

    void setupCameraParameters();
    void setupMarkerParameters();
    bool checkIntersectionRay_Triangle(const Qt3DCore::QRay3D ray, const QVector<QVector3D> triangle, qreal &tnear);
    QString select_on_structure(QVector2D mouseXYNormalized, qreal &tnear);
    void run_private();


    QString select_on_mesh(QVector2D mouseXYNormalized, qreal &tnear);
    double distance_segments(QVector3D p1, QVector3D p2, QVector3D s1, QVector3D s2);
    void run_split_private();
};

#endif // SYNCHLOOP_H
