import QtQuick 2.0
import SynchLoop 1.0
Item {
    visible: true
    width: 1280
    height: 960

    SynchLoop{
        id:synchLoop
        Component.onCompleted: {
            setFileVideoFileName("/media/chili/6F9C-46F9/New Experiment 13-09-2016 14 51 02/Participant 1-[838c8d94-3bd4-40f1-9c60-f0c63fb6ef9a]/Participant 1 (1)-1-recording.avi")
            loadGazeData("/media/chili/6F9C-46F9/New Experiment 13-09-2016 14 51 02_Participant 1 (1)_002_Trial001 Samples.txt")
            //loadMesh("/media/chili/6F9C-46F9/ROOF.obj","default")
            loadStructure("/home/chili/QTProjects/staTIc/Scripts/Demo.structure")
            //loadMultiMarkersConfigFile("default","/home/chili/QTProjects/EyeTrackingExtractAOIHits/Test_tag.dat")
            loadMultiMarkersConfigFile("default","/home/chili/board_configuration.data")
            run();
        }
    }

    Text{
        text:synchLoop.position
        anchors.centerIn: parent
    }


}
