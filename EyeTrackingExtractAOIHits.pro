TEMPLATE = app

QT += qml quick 3dcore 3drender 3dinput multimedia widgets

CONFIG += c++11

SOURCES += main.cpp \
    eyetrackingeventgenerator.cpp \
    scenewalker.cpp \
    eyetrackingeventgeneratorfilterrunnable.cpp \
    synchloop.cpp

RESOURCES += qml.qrc

LIBS += -L/home/chili/opencv-2.4.13/build-linux/install/lib/ -lopencv_calib3d -lopencv_highgui -lopencv_imgproc -lopencv_core
LIBS += -L/home/chili/artoolkit5/lib  -lz -lAR -lARICP -lARWrapper

INCLUDEPATH += /home/chili/artoolkit5/include /home/chili/opencv-2.4.13/build-linux/install/include/ /home/chili/opencv-2.4.13/build-linux/install/include/opencv /home/chili/opencv-2.4.13/build-linux/install/include/opencv2


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
