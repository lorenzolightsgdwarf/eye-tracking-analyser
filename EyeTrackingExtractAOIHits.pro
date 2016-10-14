TEMPLATE = app

QT += qml quick 3dcore 3drender 3dinput multimedia widgets concurrent

CONFIG += c++11

SOURCES += main.cpp \
    eyetrackingeventgenerator.cpp \
    scenewalker.cpp \
    eyetrackingeventgeneratorfilterrunnable.cpp \
    synchloop.cpp

RESOURCES += qml.qrc

LIBS += -lopencv_calib3d -lopencv_highgui -lopencv_imgproc -lopencv_core -lopencv_video

LIBS += -L/home/chili/artoolkit5/lib  -lz -lAR -lARICP -lARWrapper

INCLUDEPATH += /home/chili/artoolkit5/include /usr/local/include/opencv2


# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    eyetrackingeventgenerator.h \
    scenewalker.h \
    eyetrackingeventgeneratorfilterrunnable.h \
    synchloop.h

DISTFILES += \
    Test_tag.dat

#To run LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/chili/opencv-2.4.13/build-linux/install/lib:/home/chili/artoolkit5/lib
#To run LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/chili/artoolkit5/lib

#--video=/home/chili/QTProjects/EyeTrackingExtractAOIHits/Test\ File/test.avi --participant=Part1 --mode=split --fixation=/home/chili/QTProjects/EyeTrackingExtractAOIHits/Test\ File/test.txt
#--video=/home/chili/QTProjects/EyeTrackingExtractAOIHits/Test\ File/Participant_Part1_Condition_Hand_Trial_Howe.avi --subs --mode=analyse --fixation=/home/chili/QTProjects/EyeTrackingExtractAOIHits/Test\ File/Participant_Part1_Condition_Hand_Trial_Howe.txt

#--video=/home/chili/Participant\ 1-1-recording.avi --participant=Participant1 --mode=split --fixation=/home/chili/Participant-1.txt --skipIntervals=0:0:0-0:0:59;0:1:0-0:22:0
