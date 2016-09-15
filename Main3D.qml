import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Input 2.0
import Qt3D.Extras 2.0
import QtQuick 2.5 as QQ2
Entity {
    id: sceneRoot

    property real cameraScaleX:1;
    property real cameraScaleY:1;

    property alias scene_url:structureLoader.source
    property alias scene_loader: structureLoader
    Camera {
        id: scene_camera
        objectName: "camera"
        projectionType: CameraLens.FrustumProjection
        nearPlane : 0.1
        farPlane : 100000

        top: 0.1*(marker_detector.projectionMatrix.m23/(cameraScaleX*marker_detector.projectionMatrix.m11))
        bottom: -0.1*(marker_detector.projectionMatrix.m23/(cameraScaleX*marker_detector.projectionMatrix.m11))
        left: -0.1*(marker_detector.projectionMatrix.m13/(cameraScaleY*marker_detector.projectionMatrix.m22))
        right: 0.1*(marker_detector.projectionMatrix.m13/(cameraScaleY*marker_detector.projectionMatrix.m22))

        position: Qt.vector3d( 0.0, 0.0, 1 )
        upVector: Qt.vector3d( 0.0, 1.0, 0.0 )
        viewCenter: Qt.vector3d( 0.0, 0.0, 0.0 )
    }

    components: [
        RenderSettings {
            activeFrameGraph: ForwardRenderer {
                clearColor: "transparent"
                camera: scene_camera
            }
        },
        // Event Source will be set by the Qt3DQuickWindow
        InputSettings { }
    ]

    Entity {
        id:structureEntity

        Transform {
            id: structureLoaderTransform
            rotation:structure_tag.rotationQuaternion
            translation:structure_tag.translation
            QQ2.Component.onCompleted:
                structure_tag.appendQuaternion(fromAxisAndAngle(1,0,0,90))
        }

        SceneLoader{
            id:structureLoader
            onStatusChanged: if(status==SceneLoader.Ready){
                                 scene_walker.startWalk();
                                 media_player.play();
                             }
        }

        components: [structureLoader,structureLoaderTransform]

    }


}
