import QtQuick 2.0
import SynchLoop 1.0
Item {
    visible: true
    width: 200
    height: 200
    signal quit()
    Connections{
       target: synchLoop
       onDone:quit()
    }

    SynchLoop{
        id:synchLoop
        objectName: "synchLoop"

        function start(){
            setWrite_subtititles(enable_subs)//Always before load video
            setParticipant(participant)
            setMode(mode)
            setFileVideoFileName(video_file)
            loadGazeData(gaze_data_file)
            if(mode=="analyse"){
                loadStructure(structure_file)
                loadMultiMarkersConfigFile("default","/home/chili/board_configuration.data")
                loadMultiMarkersConfigFile("tablet","/home/chili/QTProjects/EyeTrackingExtractAOIHits/tablet_tags.data")
                loadMesh("/home/chili/QTProjects/EyeTrackingExtractAOIHits/tablet_mesh.obj","tablet")
            }
            run();
        }
    }

    Column{
        anchors.fill: parent
        anchors.margins: 10
        Text{
            text:"Processing video:"+video_file
        }
        Text{
            text:"Participant:"+participant
        }
        Text{
            text:"Gaze Data:"+gaze_data_file
        }
        Text{
            text:"Video second: "+synchLoop.position
        }
    }

    Timer{
        interval: 5000
        running: true
        repeat: false
        onTriggered: synchLoop.start()
    }


}
