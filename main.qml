import QtQuick 2.6
import QtQuick.Controls 1.5
import QtMultimedia 5.5
import EyeTrackingEventGenerator 1.0
import QtQuick.Scene3D 2.0
import ARToolkit 1.0
import SceneWalker 1.0
Item {
    visible: true
    width: 1280
    height: 960

    MediaPlayer{
        id:media_player
        source: "file:///media/chili/6F9C-46F9/Participant 1-1-recording_x264.mp4"
        playbackRate: 1
        autoPlay: false
    }

    VideoOutput{
        id:video_output
        source: media_player
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        filters:[marker_detector,event_generator]
    }

    EyeTrackingEventGenerator{
        id:event_generator
        frameRate:25
        objectName: "event_generator"
    }

    Scene3D {
        id:scene3DContainer
        anchors.fill: parent
        focus: true
        aspects: ["input"]
        multisample:true
        Main3D {
            id:main3d
            property real view_finder_width:video_output.contentRect.width
            property real view_finder_height:video_output.contentRect.height;
            cameraScaleX: view_finder_height/scene3DContainer.height
            cameraScaleY: view_finder_width/scene3DContainer.width
            scene_url:"qrc:/test.dae"
        }
    }

    SceneWalker{
        id:scene_walker
        loader:main3d.scene_loader
    }

    ARToolkit{
        id:marker_detector
        matrixCode: ARToolkit.MATRIX_CODE_4x4_BCH_13_9_3
        Component.onCompleted: {
            loadMultiMarkersConfigFile("default","file:///home/chili/board_configuration.data")
        }
    }
    ARToolkitObject{
        id:structure_tag
        objectId: "default"
        Component.onCompleted: marker_detector.registerObserver(structure_tag)

    }


}
