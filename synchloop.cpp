#include "synchloop.h"
#include <QDebug>
#include <QQuaternion>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <QtConcurrent>

#define DIST_THRESHOLD 30
const char separator=',';

/*Utility function to skip comments and new lines in the config file*/
QString readLine(QTextStream* inputStream, char separator=separator){
    QString line;
    bool isValid=false;
    do{
        line=inputStream->readLine();
        if(line.isEmpty())
            continue;
        if(line.contains("#"))
            line=line.split("#").at(0);
        bool allEmpty=true;
        Q_FOREACH(QString part, line.split(separator)){
            if(!part.isEmpty()){
                allEmpty=false;
                break;
            }
        }
        if(allEmpty)
            continue;
        isValid=true;
    }while(!isValid);
    return line;
}

SynchLoop::SynchLoop(QObject *parent) : QObject(parent)
{
    ar_param=NULL;
    ar_handle=NULL;
    ar_3d_handle=NULL;
    ar_patt_handle=NULL;
    frame_counter=0;
    cameraSize=QSize(640,480);
    cameraSize=QSize(1280,960);

    projectionMatrix=QMatrix4x4(
                610, 0., 640/2 ,0,
                0., 610, 480/2,0,
                0,              0,              1,  0,
                0,              0,              0,  1
                );

    projectionMatrix=QMatrix4x4(1.1326430126128812e+03, 0., 6.3950000000000000e+02,0,
                                0.   ,     1.1326430126128812e+03, 4.7950000000000000e+02, 0,
                                0., 0., 1,0.,
                                0,0,0,1);


    camera.setProjectionType(Qt3DCore::QCameraLens::FrustumProjection);
    camera.setNearPlane(0.1);
    camera.setFarPlane(10000);
    camera.setTop( 0.1*(projectionMatrix(1,2)/(projectionMatrix(0,0))));
    camera.setBottom(-0.1*(projectionMatrix(1,2)/(projectionMatrix(0,0))));
    camera.setLeft(-0.1*(projectionMatrix(0,2)/(projectionMatrix(1,1))));
    camera.setRight(0.1*(projectionMatrix(0,2)/(projectionMatrix(1,1))));
    camera.setPosition(QVector3D(0,0,0));
    camera.setUpVector(QVector3D(0,1,0));
    camera.setViewCenter(QVector3D(0,0,-1));

    write_subtitles=false;
    subtitles_counter=0;

    setupCameraParameters();
    setupMarkerParameters();

//    for(int i=0;i<10000;i++)
//       fixations.insert(i,QPair<QVector2D,int>(QVector2D(640/2,480/2),1));

}

void SynchLoop::setFileVideoFileName(QString video_file_name,QString skip_intervals)
{
    this->video_file_name=video_file_name;
    //video_capture.open(1);
    video_capture.open(video_file_name.toStdString());
    tot_frame=video_capture.get(CV_CAP_PROP_FRAME_COUNT);
    if(!video_capture.isOpened()){
        qFatal("Cannot open video file");
    }
    if(write_subtitles){
        subtitle_file.setFileName(video_file_name+".srt");
        subtitle_file.open(QFile::WriteOnly | QFile::Truncate);
        if(subtitle_file.isOpen()){
            subtitles_stream.setDevice(&subtitle_file);
        }
        else{
            qDebug()<<"Can't make subtitles";
            write_subtitles=false;
        }
    }
    if(!skip_intervals.isEmpty()){
        QStringList intervals=skip_intervals.split(";");
        Q_FOREACH(QString interval, intervals){
            QString start=interval.split("-")[0];
            QString end=interval.split("-")[1];

            QStringList start_parts=start.split(":");
            QStringList end_parts=end.split(":");

            int start_frame= (start_parts[0].toInt()*3600+start_parts[1].toInt()*60+start_parts[2].toInt())*fps;
            int end_frame= (end_parts[0].toInt()*3600+end_parts[1].toInt()*60+end_parts[2].toInt())*fps;

            for(int i=start_frame;i<=end_frame;i++){
                skip_frames.append(i);
            }
        }
    }
}

void SynchLoop::setManualProperties(QString solvingTime, QString verifyTime, QString exploreTime, QString stopTime, QString condition, QString trial)
{
    manual_condition=condition;
    manual_trial=trial;
    QStringList parts=solvingTime.split(":");
    solving_frame= (parts[0].toInt()*3600+parts[1].toInt()*60+parts[2].toInt())*fps;

    parts=verifyTime.split(":");
    verify_frame= (parts[0].toInt()*3600+parts[1].toInt()*60+parts[2].toInt())*fps;

    parts=exploreTime.split(":");
    explore_frame= (parts[0].toInt()*3600+parts[1].toInt()*60+parts[2].toInt())*fps;

    parts=stopTime.split(":");
    stop_frame= (parts[0].toInt()*3600+parts[1].toInt()*60+parts[2].toInt())*fps;

}

void SynchLoop::loadGazeData(QString file_name){
    gaze_data_file_name=file_name;
    int fixationGroup=0;
    QString event_type;
    QFile file(file_name);
    if(!file.open(QFile::ReadOnly)){
        qDebug()<<"Can't open gaze file";
        return;
    }
    QTextStream stream(&file);
    //Read line and split
    QString line;
    int i=0;
    if(mode=="split" || mode=="split-manual")
        line=readLine(&stream);
    while(!stream.atEnd()){
        line=readLine(&stream);
        QStringList parts=line.split(",");
        if((mode=="split" || mode=="split-manual") && parts.size()!=5){
            qDebug()<<"Invalid file format for gaze data";
            return;
        }
        if(mode=="analyse" && parts.size()!=11){
            qDebug()<<"Invalid file format for gaze data";
            return;
        }
        if(parts[1]!=event_type && parts[1]=="Visual Intake")
            fixationGroup++;
        event_type=parts[1];
        if(event_type!="Visual Intake")
            continue;
        QVector2D pos(parts[2].toDouble(),parts[3].toDouble());
        QString timestamp=parts[4];
        QStringList timestamp_parts=timestamp.split(':');
        int frame;

        if(mode=="split" || mode=="split-manual"){
            frame=timestamp_parts[0].toInt()*3600*fps+
                    timestamp_parts[1].toInt()*60*fps+
                    timestamp_parts[2].toInt()*fps+
                    round(timestamp_parts[3].toInt()*(fps/1000.0));
        }
        else if(mode=="analyse")
            frame=parts[10].toInt();

        QPair<QVector2D,QString> fixation;
        fixation.first=pos;
        if(mode=="split")
            fixation.second=line+","+QString::number(fixationGroup);
        else if(mode=="analyse")
            fixation.second=line;
        if(mode=="split-manual")
            fixation.second=line+","+QString::number(fixationGroup);
        fixations.insert(frame,fixation);
    }
    file.close();
}

void SynchLoop::loadStructure(QString file_name){

    bool ok;

    QFile inputFile(file_name);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"Failed to open structure file";
        return;
    }

    QTextStream inputStream(&inputFile);

    QString line=readLine(&inputStream);

    /*Model to real scale factor*/
    qreal modelScale=line.split(separator).at(0).toDouble(&ok);
    if(!ok){
        qWarning("Fail to read model scale factor");
        return;
    }
    /*Get number of nodes*/
    line=readLine(&inputStream);
    int number_nodes=line.split(separator).at(0).toInt(&ok);
    if(!ok){
        qWarning("Fail to read number of nodes");
        return;
    }
    QVector<QVector3D> tmp_joint;

    /*Read node data*/
    for(int i=0;i<number_nodes;i++){
        line=readLine(&inputStream);
        QStringList line_parts=line.split(separator);
        if(line_parts.size()!=5){
            qWarning("Issue reading node line:");
            qWarning(line.toStdString().c_str());
            return;
        }
        qreal x=line_parts[1].toFloat(&ok);
        if(!ok){
            qWarning("Fail to convert coordinate:");
            qWarning(line_parts[1].toStdString().c_str());
            return;
        }
        qreal y=line_parts[2].toFloat(&ok);
        if(!ok){
            qWarning("Fail to convert coordinate:");
            qWarning(line_parts[2].toStdString().c_str());
            return;
        }
        qreal z=line_parts[3].toFloat(&ok);
        if(!ok){
            qWarning("Fail to convert coordinate:");
            qWarning(line_parts[3].toStdString().c_str());
            return;
        }
        joints[line_parts[0]]=QVector3D(x,y,z)*modelScale;
        tmp_joint.append(QVector3D(x,y,z)*modelScale);
    }

    /*Read number of reactions */
    line=readLine(&inputStream);
    int number_reactions=line.split(separator).at(0).toInt(&ok);
    if(!ok){
        qWarning("Fail to read number of reactions");
        qWarning(line.toStdString().c_str());
        return;
    }
    /*Get reactions*/
    for(int i=0;i<number_reactions;i++){
        line=readLine(&inputStream);
    }

    /*Read number of beams*/
    line=readLine(&inputStream);
    int number_beams=line.split(separator).at(0).toInt(&ok);
    if(!ok){
        qWarning("Fail to read number of elements");
        qWarning(line.toStdString().c_str());
        return;
    }
    /*Read beams*/
    for(int i=0;i<number_beams;i++){
        line=readLine(&inputStream);
        QStringList line_parts=line.split(separator);
        if(line_parts.size()!=8){
            qWarning("Issue reading element line:");
            qWarning(line.toStdString().c_str());
            return;
        }
        int joint_index_1=line_parts[1].toInt(&ok);
        if(!ok){
            qWarning("Fail to convert extreme index");
            qWarning(line_parts[1].toStdString().c_str());
            return ;
        }
        int joint_index_2=line_parts[2].toInt(&ok);
        if(!ok){
            qWarning("Fail to convert extreme index");
            qWarning(line_parts[2].toStdString().c_str());
            return ;
        }
        if(joint_index_1==joint_index_2 ||
                joint_index_1>=number_nodes ||
                joint_index_2>=number_nodes ||
                joint_index_1<0 || joint_index_2<0){
            qWarning("Invalid etreme index:");
            qWarning()<<joint_index_1;
            qWarning()<<joint_index_2;
            return;
        }
        beams[line_parts[0]]=QPair<QVector3D,QVector3D>(tmp_joint[joint_index_1],tmp_joint[joint_index_2]);
    }

}

void SynchLoop::run(){
    if(mode=="split")
        QtConcurrent::run(this,&SynchLoop::run_split_private);
    else if(mode=="analyse")
        QtConcurrent::run(this,&SynchLoop::run_private);
    else if(mode=="split-manual")
        QtConcurrent::run(this,&SynchLoop::run_split_manual_private);

}

void SynchLoop::run_private(){

    cv::Mat frame;
    AR2VideoBufferT* ar_buffer;
    ar_buffer=(AR2VideoBufferT*)malloc(sizeof(AR2VideoBufferT));
    int marker_num;
    ARMarkerInfo* marker_info;
    QMatrix3x3 rotMat;
    qreal err;
    QQuaternion openglAlignment=QQuaternion::fromAxisAndAngle(1,0,0,180);

    m_current_frame= QImage(cameraSize.width(),cameraSize.height(),QImage::Format_Grayscale8);

    cv::Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
    cv::BackgroundSubtractorMOG2 pMOG2; //MOG2 Background subtractor

    pMOG2 = cv::BackgroundSubtractorMOG2();
    QString latest_value;
    QVector2D prev_gaze(-100,-100);
    while(video_capture.read(frame)){
        if(frame_counter%fps==0) emit positionChanged();
        if(skip_frames.contains(frame_counter)){
            frame_counter++;
            continue;
        }
        cv::Mat gray_img;
        cv::cvtColor(frame, gray_img, CV_BGR2GRAY,1);

        ar_buffer->buff=gray_img.data;
        ar_buffer->bufPlaneCount=0;
        ar_buffer->bufPlanes=NULL;
        ar_buffer->fillFlag=1;
        ar_buffer->time_sec=(ARUint32)QDateTime::currentMSecsSinceEpoch()/1000;
        ar_buffer->time_usec=(ARUint32)QDateTime::currentMSecsSinceEpoch()-ar_buffer->time_sec*1000;
        ar_buffer->buffLuma=ar_buffer->buff;

        if (arDetectMarker(ar_handle, ar_buffer) < 0) {
            qDebug()<<"Error in arDetectMarker";
            continue;
        }
        marker_num=arGetMarkerNum(ar_handle);
        /*Detection*/
        if(marker_num>0){
            marker_info=arGetMarker(ar_handle);
            Q_FOREACH(QString id, ar_multimarker_objects.keys()){
                AR3DMultiPatternObject* o=ar_multimarker_objects[id];
                err=arGetTransMatMultiSquareRobust(ar_3d_handle,marker_info,marker_num,o->marker_info);
                if(err<0)
                    o->visible=false;
                else{
                    /*Filter*/
                    arFilterTransMat(o->ftmi,o->marker_info->trans,!o->visible);
                    /*...*/
                    o->visible=true;
                    for(int i=0;i<3;i++)
                        for(int j=0;j<3;j++)
                            rotMat(i,j)=o->marker_info->trans[i][j];

                    QQuaternion alignedRotation=openglAlignment*QQuaternion::fromRotationMatrix(rotMat);
                    rotMat=alignedRotation.toRotationMatrix();

                    for(int i=0;i<3;i++)
                        for(int j=0;j<3;j++)
                            o->pose(i,j)=rotMat(i,j);
                    o->pose(0,3)=o->marker_info->trans[0][3];
                    o->pose(1,3)=-o->marker_info->trans[1][3];
                    o->pose(2,3)=-o->marker_info->trans[2][3];
                }
            }
        }
        /*Intersection Test*/
        QString hitAOI;
        pMOG2.operator ()(gray_img,fgMaskMOG2);
        int nonZero=cv::countNonZero(fgMaskMOG2);

        if(fixations.contains(frame_counter)){
            Q_FOREACH(auto fixation,fixations.values(frame_counter)){
                QVector2D gaze_point=fixation.first;
                QVector2D gaze_point_norm=QVector2D((2.0f * gaze_point.x()) / cameraSize.width() - 1.0f, 1.0f - (2.0f * gaze_point.y()) / cameraSize.height());
                qreal dist_mesh;
                qreal dist_structure;
                QString hitAOI_mesh=select_on_mesh(gaze_point_norm,dist_mesh);
                QString hitAOI_structure=select_on_structure(gaze_point_norm,dist_structure);
                dist_mesh < dist_structure ? hitAOI=hitAOI_mesh : hitAOI=hitAOI_structure;

                if(nonZero<0.3*1280*960 && prev_gaze.distanceToPoint(gaze_point)<20){
                    hitAOI=latest_value;
                }
                else{
                    frame_mutex.lock();
                    cv::circle(frame,cv::Point2f(gaze_point.x(),gaze_point.y()),10,cv::Scalar(0,0,255),-1);
                    cv::imwrite("current_frame.png",frame);
                    emit current_frameChanged();
                    m_suggestion=hitAOI;
                    emit suggestionChanged();
                    wait_condition.wait(&frame_mutex);
                    frame_mutex.unlock();
                    hitAOI=user_value;
                    prev_gaze=gaze_point;
                }
                fixationsAOI.insert(frame_counter,hitAOI);
                latest_value=hitAOI;
            }
        }
        if(write_subtitles){
            if(frame_counter%fps==0){
                subtitles_counter++;
                double seconds =(double)frame_counter/fps;
                double int_seconds;
                double ms=modf(seconds,&int_seconds);

                int hh= int_seconds/3600;
                int mm= (int_seconds-hh*3600)/60;
                int s= (int_seconds-hh*3600-mm*60);

                subtitles_stream <<"\n\n" << subtitles_counter << "\n";
                subtitles_stream<<QString::number(hh)<<":"<<QString::number(mm)<<":"<<QString::number(s)<<",000"
                               <<" --> "<<QString::number(hh)<<":"<<QString::number(mm)<<":"<<QString::number(s+1)<<",000\n";
            }
            if(!hitAOI.isEmpty())
                subtitles_stream<<hitAOI<<";";
        }
        frame_counter++;
    }
    video_capture.release();

    QFileInfo fixation_file_info(gaze_data_file_name);
    QString fixation_AOI_file_path=fixation_file_info.absolutePath()+"/"+fixation_file_info.baseName()+"_AOI.txt";
    QFile fixation_with_AOI(fixation_AOI_file_path);
    fixation_with_AOI.open(QFile::WriteOnly| QFile::Truncate);
    QTextStream file_with_AOI_stream(&fixation_with_AOI);

    QFile fixation_file(gaze_data_file_name);
    fixation_file.open(QFile::ReadOnly);
    QTextStream fixation_file_stream(&fixation_file);

    auto timestamps=fixationsAOI.keys().toSet().toList();
    qSort(timestamps.begin(), timestamps.end());
    int i=0;
    Q_FOREACH(auto timestamp, timestamps){
        Q_FOREACH(auto AOI,fixationsAOI.values(timestamp)){
            i++;
            int hh=timestamp/(fps*3600);
            int mm=(timestamp-hh*3600*fps)/(60*fps);
            int ss=(timestamp-hh*3600*fps - mm*60*fps)/fps;
            int fms=timestamp-hh*3600*fps - mm*60*fps - ss*fps;
            file_with_AOI_stream<<readLine(&fixation_file_stream)<<","<<AOI<<","<<QString::number(hh)<<":"
                               <<QString::number(mm)<<":"<<QString::number(ss)<<":"<<fms<<"\n";

        }
    }

    file_with_AOI_stream.flush();
    fixation_with_AOI.close();
    fixation_file.close();

    if(write_subtitles){
        subtitles_stream.flush();
        subtitle_file.close();

    }
    emit done();

}

void SynchLoop::run_split_private(){

    int tag_ex_1_cond_hand=1;
    int tag_ex_2_cond_hand=2;
    int tag_ex_3_cond_hand=3;
    int tag_ex_4_cond_hand=4;

    int tag_ex_1_cond_fix=5;
    int tag_ex_2_cond_fix=6;
    int tag_ex_3_cond_fix=7;
    int tag_ex_4_cond_fix=8;



    int tag_home=9;
    int tag_explore=10;
    int tag_verify=11;


    QString stage="solving";
    QString condition;
    QString trial;

    bool writing=false;
    int framecounter_offset=0;

    cv::Mat frame;
    AR2VideoBufferT* ar_buffer;
    ar_buffer=(AR2VideoBufferT*)malloc(sizeof(AR2VideoBufferT));
    int marker_num;
    ARMarkerInfo* marker_info;


    cv::VideoWriter video_writer;
    QFileInfo video_file_info(video_file_name);
    QString tmp_video_file_name=video_file_info.absolutePath()+"/tmp_video.avi";
    video_writer.open(tmp_video_file_name.toStdString(),CV_FOURCC('X','V','I','D'),fps,cv::Size(cameraSize.width(),cameraSize.height()));
    if(!video_writer.isOpened()){
        qFatal("Cannot open tmp video file");
    }
    QFileInfo fixation_file_info(gaze_data_file_name);
    QString tmp_fixation_file_name=fixation_file_info.absolutePath()+"/tmp_fix.txt";
    QFile fix_file(tmp_fixation_file_name);
    if(!fix_file.open(QFile::WriteOnly|QFile::Truncate)){
        qFatal("Cannot open tmp fixation file");
    }
    QTextStream fix_stream;
    fix_stream.setDevice(&fix_file);

    while(video_capture.read(frame)){
        if(frame_counter%fps==0) emit positionChanged();
        if(skip_frames.contains(frame_counter)){
            frame_counter++;
            continue;
        }
        cv::Mat gray_img;
        cv::cvtColor(frame, gray_img, CV_BGR2GRAY,1);
        cv::equalizeHist( gray_img, gray_img );
        ar_buffer->buff=gray_img.data;
        ar_buffer->bufPlaneCount=0;
        ar_buffer->bufPlanes=NULL;
        ar_buffer->fillFlag=1;
        ar_buffer->time_sec=(ARUint32)QDateTime::currentMSecsSinceEpoch()/1000;
        ar_buffer->time_usec=(ARUint32)QDateTime::currentMSecsSinceEpoch()-ar_buffer->time_sec*1000;
        ar_buffer->buffLuma=ar_buffer->buff;

        if (arDetectMarker(ar_handle, ar_buffer) < 0) {
            qDebug()<<"Error in arDetectMarker";
            continue;
        }
        marker_num=arGetMarkerNum(ar_handle);
        /*Detection*/
        if(marker_num>0){
            marker_info=arGetMarker(ar_handle);
            for(int i=0;i<marker_num;i++){
                if(marker_info[i].idMatrix==tag_ex_1_cond_fix && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Fixed";
                        trial="Howe";
                        writing=true;
                        framecounter_offset=frame_counter;
                    }
                }
                else if(marker_info[i].idMatrix==tag_ex_2_cond_fix && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Fixed";
                        trial="Vault";
                        writing=true;
                        framecounter_offset=frame_counter;
                    }
                }
                else if(marker_info[i].idMatrix==tag_ex_3_cond_fix && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Fixed";
                        trial="Roof";
                        writing=true;
                        framecounter_offset=frame_counter;}
                }
                else if(marker_info[i].idMatrix==tag_ex_4_cond_fix && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Fixed";
                        trial="Gazebo";
                        writing=true;
                        framecounter_offset=frame_counter; }
                }
                else if(marker_info[i].idMatrix==tag_ex_1_cond_hand && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Hand";
                        trial="Howe";
                        writing=true;
                        framecounter_offset=frame_counter; }
                }
                else if(marker_info[i].idMatrix==tag_ex_2_cond_hand && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Hand";
                        trial="Vault";
                        writing=true;
                        framecounter_offset=frame_counter;}
                }
                else if(marker_info[i].idMatrix==tag_ex_3_cond_hand && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Hand";
                        trial="Roof";
                        writing=true;
                        framecounter_offset=frame_counter;}

                }
                else if(marker_info[i].idMatrix==tag_ex_4_cond_hand && marker_info[i].cfMatrix>0.20){
                    if(!writing){
                        condition="Hand";
                        trial="Gazebo";
                        writing=true;
                        framecounter_offset=frame_counter; }

                }
                else if(marker_info[i].idMatrix==tag_home && marker_info[i].cfMatrix>0.20){
                    //Close the video,close fix file, close
                    if(writing){
                        video_writer.release();
                        fix_stream.flush();
                        fix_file.close();
                        QString name_suff="Participant_"+participant+"_Condition_"+condition+"_Trial_"+trial;
                        QFile::remove(fixation_file_info.absolutePath()+"/"+name_suff+".txt");
                        fix_file.rename(fixation_file_info.absolutePath()+"/"+name_suff+".txt");
                        QFile video_file(tmp_video_file_name);
                        QFile::remove(video_file_info.absolutePath()+"/"+name_suff+".avi");
                        video_file.rename(video_file_info.absolutePath()+"/"+name_suff+".avi");
                        video_writer.open(tmp_video_file_name.toStdString(),CV_FOURCC('X','V','I','D'),fps,cv::Size(cameraSize.width(),cameraSize.height()));
                        if(!video_writer.isOpened()){
                            qFatal("Cannot open tmp video file");
                        }
                        fix_file.setFileName(tmp_fixation_file_name);
                        if(!fix_file.open(QFile::WriteOnly|QFile::Truncate)){
                            qFatal("Cannot open tmp fixation file");
                        }
                        fix_stream.setDevice(&fix_file);
                        stage="solving";
                        condition.clear();
                        trial.clear();
                    }
                    writing=false;
                }
                else if(marker_info[i].idMatrix==tag_verify && marker_info[i].cfMatrix>0.20)
                    stage="verify";
                else if(marker_info[i].idMatrix==tag_explore && marker_info[i].cfMatrix>0.20)
                    stage="explore";
            }
        }
        if(writing){
            video_writer<<frame;
            if(fixations.contains(frame_counter)){
                Q_FOREACH(auto fixation,fixations.values(frame_counter)){
                    //write fixation;
                    fix_stream<<fixation.second<<","<<participant<<","<<condition<<","<<trial<<","<<stage<<","<<frame_counter-framecounter_offset<<"\n";
                }
            }

        }
        frame_counter++;
    }
    video_capture.release();
    if(writing){
        video_writer.release();
        fix_stream.flush();
        fix_file.close();
        QString name_suff="Participant_"+participant+"_Condition_"+condition+"_Trial_"+trial;
        QFile::remove(fixation_file_info.absolutePath()+"/"+name_suff+".txt");
        fix_file.rename(fixation_file_info.absolutePath()+"/"+name_suff+".txt");
        QFile video_file(tmp_video_file_name);
        QFile::remove(video_file_info.absolutePath()+"/"+name_suff+".avi");
        video_file.rename(video_file_info.absolutePath()+"/"+name_suff+".avi");
    }
    emit done();

}

void SynchLoop::run_split_manual_private(){

    QString stage="solving";
    QString condition=manual_condition;
    QString trial=manual_trial;

    bool writing=false;
    int framecounter_offset=0;

    cv::Mat frame;

    cv::VideoWriter video_writer;
    QFileInfo video_file_info(video_file_name);
    QString tmp_video_file_name=video_file_info.absolutePath()+"/tmp_video.avi";
    video_writer.open(tmp_video_file_name.toStdString(),CV_FOURCC('X','V','I','D'),fps,cv::Size(cameraSize.width(),cameraSize.height()));
    if(!video_writer.isOpened()){
        qFatal("Cannot open tmp video file");
    }
    QFileInfo fixation_file_info(gaze_data_file_name);
    QString tmp_fixation_file_name=fixation_file_info.absolutePath()+"/tmp_fix.txt";
    QFile fix_file(tmp_fixation_file_name);
    if(!fix_file.open(QFile::WriteOnly|QFile::Truncate)){
        qFatal("Cannot open tmp fixation file");
    }
    QTextStream fix_stream;
    fix_stream.setDevice(&fix_file);

    while(video_capture.read(frame)){
        if(frame_counter%fps==0) emit positionChanged();
        if(skip_frames.contains(frame_counter)){
            frame_counter++;
            continue;
        }

        if(frame_counter==solving_frame){
            writing=true;
            framecounter_offset=frame_counter;
            stage="solving";
        }
        if(frame_counter==verify_frame)
            stage="verify";
        if(frame_counter==explore_frame)
            stage="explore";
        if(frame_counter==stop_frame){
            video_writer.release();
            fix_stream.flush();
            fix_file.close();
            QString name_suff="Participant_"+participant+"_Condition_"+condition+"_Trial_"+trial;
            QFile::remove(fixation_file_info.absolutePath()+"/"+name_suff+".txt");
            fix_file.rename(fixation_file_info.absolutePath()+"/"+name_suff+".txt");
            QFile video_file(tmp_video_file_name);
            QFile::remove(video_file_info.absolutePath()+"/"+name_suff+".avi");
            video_file.rename(video_file_info.absolutePath()+"/"+name_suff+".avi");
            writing=false;
            break;
        }
        if(writing){
            video_writer<<frame;
            if(fixations.contains(frame_counter)){
                Q_FOREACH(auto fixation,fixations.values(frame_counter)){
                    //write fixation;
                    fix_stream<<fixation.second<<","<<participant<<","<<condition<<","<<trial<<","<<stage<<","<<frame_counter-framecounter_offset<<"\n";
                }
            }
        }
        frame_counter++;
    }
    video_capture.release();
    emit done();
}


void SynchLoop::setupCameraParameters()
{
    ARParam cparam;
    /*
        * The values for  dist_function_version  correspond to the following algorithms:
        version 1: The original ARToolKit lens model, with a single radial distortion factor, plus center of distortion.<br>
        version 2: Improved distortion model, introduced in ARToolKit v4.0. This algorithm adds a quadratic term to the radial distortion factor of the version 1 algorithm.<br>
        version 3: Improved distortion model with aspect ratio, introduced in ARToolKit v4.0. The addition of an aspect ratio to the version 2 algorithm allows for non-square pixels, as found e.g. in DV image streams.<br>
        version 4: OpenCV-based distortion model, introduced in ARToolKit v4.3. This differs from the standard OpenCV model by the addition of a scale factor, so that input values do not exceed the range [-1.0, 1.0] in either forward or inverse conversions.
        */

    cparam.xsize=cameraSize.width();
    cparam.ysize=cameraSize.height();
    cparam.dist_function_version=4;
    cparam.dist_factor[0]=distortionCoeff.x();
    cparam.dist_factor[1]=distortionCoeff.y();
    cparam.dist_factor[2]=distortionCoeff.z();
    cparam.dist_factor[3]=distortionCoeff.w();

    cparam.dist_factor[4]=projectionMatrix.operator ()(0,0);
    cparam.dist_factor[5]=projectionMatrix.operator ()(1,1);
    cparam.dist_factor[6]=projectionMatrix.operator ()(0,3);
    cparam.dist_factor[7]=projectionMatrix.operator ()(1,3);
    cparam.dist_factor[8]=1;

    int i,j;
    for(i=0;i<3;i++){
        for(j=0;j<3;j++)
            cparam.mat[i][j]=projectionMatrix.operator ()(i,j);
        cparam.mat[i][3]=0;
    }

    if ((ar_param = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
        qWarning("Error in creating ar_param");
        return;
    }

    if ((ar_handle = arCreateHandle(ar_param)) == NULL) {
        qWarning("Error in creating ar_handle");
        return;

    }
    if (arSetPixelFormat(ar_handle, AR_PIXEL_FORMAT_MONO) < 0) {
        qWarning("Error in setting pixel format");
        return;
    }
    if (arSetDebugMode(ar_handle, AR_DEBUG_DISABLE) < 0) {
        qWarning("Error in setting debug mode");
        return;
    }

    if ((ar_3d_handle = ar3DCreateHandle(&cparam)) == NULL) {
        qWarning("Error in creating ar_3d_handle");
        return;
    }

}

void SynchLoop::setupMarkerParameters()
{
    if(ar_handle){
        arSetLabelingThreshMode(ar_handle, AR_LABELING_THRESH_MODE_MANUAL);
        arSetLabelingThresh(ar_handle,AR_DEFAULT_LABELING_THRESH);
        arSetPatternDetectionMode(ar_handle,AR_TEMPLATE_MATCHING_MONO_AND_MATRIX);
        arSetMatrixCodeType(ar_handle, AR_MATRIX_CODE_4x4_BCH_13_9_3);
        if(ar_patt_handle==NULL){
            ar_patt_handle=arPattCreateHandle();
        }
        arPattAttach(ar_handle, ar_patt_handle);
    }
}

void SynchLoop::loadMultiMarkersConfigFile(QString config_name,QString file_path)
{
    if(!ar_handle){
        qWarning("AR Handle is null");
        return;
    }
    ARMultiMarkerInfoT* marker_info=arMultiReadConfigFile(file_path.toStdString().c_str(),ar_patt_handle);
    if(marker_info!=NULL){
        marker_info->min_submarker=0;
        AR3DMultiPatternObject* o=new AR3DMultiPatternObject;
        o->marker_info=marker_info;
        o->visible=false;
        o->ftmi=arFilterTransMatInit(25,15);
        ar_multimarker_objects[config_name]=o;
    }
    else{
        qWarning()<<"Cannot load multipatter file";
    }

}

void SynchLoop::loadMesh(QString file_path,QString transform_name){

    QFile file(file_path);
    if(file.open(QFile::ReadOnly)){
        QTextStream stream(&file);

        QString line;

        QString objectName;
        int face_offset;
        int verteces_count=0;
        while (!stream.atEnd()) {
            line = stream.readLine();
            if(line.at(0)==QChar('#')) continue;
            if(line.at(0)==QChar('o')){
                QStringList parts= line.split(" ");
                objectName=parts[1];
                mesh_name2pose_name[objectName]=transform_name;
                face_offset=verteces_count;
            }
            else if(line.at(0)==QChar('v')){
                QStringList parts= line.split(" ");
                mesh_vertices_objects[objectName].append(QVector3D(parts[1].toFloat(),parts[2].toFloat(),parts[3].toFloat()));
                verteces_count++;
            }
            else if(line.at(0)==QChar('f')){
                QStringList parts= line.split(" ");
                QVector<int> face;
                if(parts[1].toInt()<0 || parts[2].toInt()<0 || parts[3].toInt()<0)
                    qFatal("Obj is using a negative index");
                face.append(parts[1].toInt()-face_offset);
                face.append(parts[2].toInt()-face_offset);
                face.append(parts[3].toInt()-face_offset);
                mesh_faces_objects[objectName].append(face);
            }
        }
        file.close();
    }
    else
        qDebug("Can't open mesh file");


}

QString SynchLoop::select_on_structure(QVector2D mouseXYNormalized,qreal &tnear){

    QVector4D ray_clip(mouseXYNormalized.x(),mouseXYNormalized.y(),-1,1);
    QVector4D ray_eye = camera.projectionMatrix().inverted().map(ray_clip);
    ray_eye.setZ(-1);ray_eye.setW(0);

    QVector4D ray_wor_4D = camera.viewMatrix().inverted().map(ray_eye);
    QVector3D ray_wor(ray_wor_4D.x(),ray_wor_4D.y(),ray_wor_4D.z());
    ray_wor.normalize();

    Qt3DCore::QRay3D ray;
    ray.setOrigin(QVector3D(0,0,0));
    ray.setDirection(ray_wor);

    qreal hitEntity_tnear=DBL_MAX;
    QString hitEntity;

    if(!ar_multimarker_objects.contains("default")){
        tnear=DBL_MAX;
        return "";
    }

    if(!ar_multimarker_objects["default"]->visible)
    {
        tnear=DBL_MAX;
        return "";
    }



    QMatrix4x4 pose= ar_multimarker_objects["default"]->pose*QMatrix4x4(1,0,0,0,
                                                                        0,0,-1,0,
                                                                        0,1,0,0,
                                                                        0,0,0,1);
    QMatrix4x4 pose_inv= pose.inverted();

    Qt3DCore::QRay3D ray_transform(pose_inv*ray.origin(),pose_inv.mapVector(ray.direction()));

    Q_FOREACH(QString joint_name, joints.keys()){
        QVector3D pos=joints[joint_name];

        qreal dist=ray_transform.distance(pos);
        if(dist<DIST_THRESHOLD)
            dist=ray_transform.projectedDistance(pos);
        else
            dist=DBL_MAX;
        if(dist>=0 && dist<hitEntity_tnear){
            hitEntity=joint_name;
            hitEntity_tnear=dist;
        }
    }

    QVector3D ray_end=ray_transform.origin()+ray_transform.direction()*10000;

    Q_FOREACH(QString beam_name, beams.keys()){
        QVector3D p1=beams[beam_name].first;
        QVector3D p2=beams[beam_name].second;
        //qDebug()<<beam_name;
        qreal dist=distance_segments(ray_transform.origin(),ray_end,p1,p2);
        if(dist>=0 && dist<hitEntity_tnear){
            hitEntity=beam_name;
            hitEntity_tnear=dist;
        }
    }

    tnear=hitEntity_tnear;
    return hitEntity;
}

QString SynchLoop::select_on_mesh(QVector2D mouseXYNormalized,qreal &tnear){
    QVector4D ray_clip(mouseXYNormalized.x(),mouseXYNormalized.y(),-1,1);
    QVector4D ray_eye = camera.projectionMatrix().inverted().map(ray_clip);
    ray_eye.setZ(-1);ray_eye.setW(0);

    QVector4D ray_wor_4D = camera.viewMatrix().inverted().map(ray_eye);
    QVector3D ray_wor(ray_wor_4D.x(),ray_wor_4D.y(),ray_wor_4D.z());
    ray_wor.normalize();

    Qt3DCore::QRay3D ray;
    ray.setOrigin(QVector3D(0,0,0));
    ray.setDirection(ray_wor);

    qreal hitEntity_tnear=DBL_MAX;
    QString hitEntity;

    QVector<QVector3D> triangle(3,QVector3D());
    Q_FOREACH(QString AOI, mesh_faces_objects.keys()){
        if(!ar_multimarker_objects.contains(mesh_name2pose_name[AOI])) continue;
        if(!ar_multimarker_objects[mesh_name2pose_name[AOI]]->visible) continue;
        QMatrix4x4 pose_inv= ar_multimarker_objects[mesh_name2pose_name[AOI]]->pose.inverted();
        Qt3DCore::QRay3D ray_transform(pose_inv*ray.origin(),pose_inv.mapVector(ray.direction()));
        Q_FOREACH(QVector<int> triangle_face, mesh_faces_objects[AOI]){
            triangle[0]=mesh_vertices_objects[AOI][triangle_face[0]-1];
            triangle[1]=mesh_vertices_objects[AOI][triangle_face[1]-1];
            triangle[2]=mesh_vertices_objects[AOI][triangle_face[2]-1];
            qreal t_near;
            if(checkIntersectionRay_Triangle(ray_transform,triangle,t_near)){
                if(t_near<hitEntity_tnear){
                    hitEntity_tnear=t_near;
                    hitEntity=AOI;
                }
            }
        }
    }

    tnear=hitEntity_tnear;
    return hitEntity;
}

/*Based on http://geomalgorithms.com/a06-_intersect-2.html#intersect3D_RayTriangle%28%29*/
bool SynchLoop::checkIntersectionRay_Triangle(const Qt3DCore::QRay3D ray, const QVector<QVector3D> triangle,qreal &tnear){
    if(triangle.size()!=3){qWarning()<<"Triangle doesn't have 3 points!";return false;}

    QVector3D u=triangle[1]-triangle[0]; //precomputable
    QVector3D v=triangle[2]-triangle[0]; //precomputable

    QVector3D n= QVector3D::crossProduct(u,v);

    if(n.length()==0){
        qWarning()<<"Triangle is degenerate";return false;}

    QVector3D w0=ray.origin()-triangle[0];

    qreal a=-QVector3D::dotProduct(n,w0);
    qreal b=QVector3D::dotProduct(n,ray.direction());

    if(fabs(b)<0.00000001){
        if(a==0) {qWarning()<<"Ray lies in triangle plane";return false;}

        return false;
    }

    qreal r=a/b;

    if(r<0.0)
        return 0;

    QVector3D P=ray.origin()+r*ray.direction();

    qreal uu, uv, vv, wu, wv, D;

    uu = QVector3D::dotProduct(u,u);
    uv = QVector3D::dotProduct(u,v);
    vv = QVector3D::dotProduct(v,v);
    QVector3D w = P - triangle[0];
    wu = QVector3D::dotProduct(w,u);
    wv = QVector3D::dotProduct(w,v);
    D = uv * uv - uu * vv;

    qreal s, t;
    s = (uv * wv - vv * wu) / D;
    if (s < 0.0 || s > 1.0)         // I is outside T
        return false;
    t = (uv * wu - uu * wv) / D;
    if (t < 0.0 || (s + t) > 1.0)  // I is outside T
        return false;

    tnear=r;

    return 1;                       // I is in T
}

/*BASED ON http://geomalgorithms.com/a07-_distance.html#dist3D_Segment_to_Segment()
    Computes the shortest distance between 2 segmnets and return the distance from p1 of the point on the first segmented
    which is closest to the second segment

*/

double SynchLoop::distance_segments( QVector3D p1, QVector3D p2, QVector3D s1, QVector3D s2 )
{

    QVector3D   u = p2 - p1;

    QVector3D   v = s2 - s1;

    if(v.length()>60){
        s2= s2 - v.normalized()*30;
        s1= s1 + v.normalized()*30;
    }
    else{
        s2= s2 - v.normalized()*10;
        s1= s1 + v.normalized()*10;
    }
    v= s2-s1;

    QVector3D   w = p1 - s1;
    float    a = QVector3D::dotProduct(u,u);         // always >= 0
    float    b = QVector3D::dotProduct(u,v);
    float    c = QVector3D::dotProduct(v,v);         // always >= 0
    float    d = QVector3D::dotProduct(u,w);
    float    e = QVector3D::dotProduct(v,w);
    float    D = a*c - b*b;        // always >= 0
    float    sc, sN, sD = D;       // sc = sN / sD, default sD = D >= 0
    float    tc, tN, tD = D;       // tc = tN / tD, default tD = D >= 0

    // compute the line parameters of the two closest points
    if (D < 0.00000001) { // the lines are almost parallel
        sN = 0.0;         // force using point P0 on segment S1
        sD = 1.0;         // to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    }
    else {                 // get the closest points on the infinite lines
        sN = (b*e - c*d);
        tN = (a*e - b*d);
        if (sN < 0.0) {        // sc < 0 => the s=0 edge is visible
            sN = 0.0;
            tN = e;
            tD = c;
        }
        else if (sN > sD) {  // sc > 1  => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0) {            // tc < 0 => the t=0 edge is visible
        tN = 0.0;
        // recompute sc for this edge
        if (-d < 0.0)
            sN = 0.0;
        else if (-d > a)
            sN = sD;
        else {
            sN = -d;
            sD = a;
        }
    }
    else if (tN > tD) {      // tc > 1  => the t=1 edge is visible
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0.0)
            sN = 0;
        else if ((-d + b) > a)
            sN = sD;
        else {
            sN = (-d +  b);
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = (fabs(sN) < 0.00000001 ? 0.0 : sN / sD);
    tc = (fabs(tN) < 0.00000001 ? 0.0 : tN / tD);

    // get the difference of the two closest points
    QVector3D   dP = w + (sc * u) - (tc * v);  // =  S1(sc) - S2(tc)

    //qDebug()<<dP.length();

    if(dP.length()>DIST_THRESHOLD)
        return DBL_MAX;
    else
        return (sc*u).length();


}

