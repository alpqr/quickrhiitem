import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestApp

Item {
    Text {
        id: apiInfo
        color: "black"
        font.pixelSize: 16
        property int api: GraphicsInfo.api
        text: {
            if (GraphicsInfo.api === GraphicsInfo.OpenGLRhi)
                "OpenGL on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Direct3D11Rhi)
                "D3D11 on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.VulkanRhi)
                "Vulkan on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.MetalRhi)
                "Metal on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Null)
                "Null on QRhi";
            else
                "Unknown API";
        }
    }
    Text {
        id: infoText
        anchors.top: apiInfo.bottom
        color: "black"
        text: "The green area is a QQuickRhiItem subclass instantiated from QML.\nIt renders the textured cube into a texture directly with the QRhi APIs."
    }
    RowLayout {
        anchors.top: infoText.bottom
        CheckBox {
            id: cbTrans
            text: "Transparent clear color"
            checked: false
        }
        CheckBox {
            id: cbBlend
            text: "Enable blending regardless of opacity"
            checked: true
        }
        CheckBox {
            id: cbFixedSize
            text: "Use an explicit texture size"
            checked: false
        }
    }

    Rectangle {
        color: "red"
        width: 300
        height: 300
        x: 200
        y: 200
        Text {
            text: "This is an ordinary Qt Quick rectangle"
            anchors.centerIn: parent
        }
        NumberAnimation on rotation { from: 0; to: 360; duration: 5000; loops: -1 }
    }

    TestRhiItem {
        id: renderer
        anchors.centerIn: parent
        width: parent.width - 400
        height: parent.height - 200

        transform: [
            Rotation { id: rotation; axis.x: 0; axis.z: 0; axis.y: 1; angle: 0; origin.x: renderer.width / 2; origin.y: renderer.height / 2; },
            Translate { id: txOut; x: -renderer.width / 2; y: -renderer.height / 2 },
            Scale { id: scale; },
            Translate { id: txIn; x: renderer.width / 2; y: renderer.height / 2 }
        ]

        cubeRotation.x: 30
        NumberAnimation on cubeRotation.y { from: 0; to: 360; duration: 5000; loops: -1 }

        property int counter: 0
        message: "This text is rendered with\nthe raster paint engine\ninto a texture.\nIt's dynamic too: counter=" + counter
        Timer {
            interval: 1000
            repeat: true
            running: true
            onTriggered: renderer.counter += 1
        }

        transparentBackground: cbTrans.checked
        alphaBlending: cbBlend.checked

        explicitTextureWidth: cbFixedSize.checked ? 128 : 0
        explicitTextureHeight: cbFixedSize.checked ? 128 : 0

        onEffectiveTextureSizeChanged: console.log("TestRhiItem is rendering to a texture of pixel size " + effectiveTextureSize)
    }

    SequentialAnimation {
        PauseAnimation { duration: 3000 }
        ParallelAnimation {
            NumberAnimation { target: scale; property: "xScale"; to: 0.6; duration: 1000; easing.type: Easing.InOutBack }
            NumberAnimation { target: scale; property: "yScale"; to: 0.6; duration: 1000; easing.type: Easing.InOutBack }
        }
        NumberAnimation { target: rotation; property: "angle"; to: 80; duration: 1000; easing.type: Easing.InOutCubic }
        NumberAnimation { target: rotation; property: "angle"; to: -80; duration: 1000; easing.type: Easing.InOutCubic }
        NumberAnimation { target: rotation; property: "angle"; to: 0; duration: 1000; easing.type: Easing.InOutCubic }
        NumberAnimation { target: renderer; property: "opacity"; to: 0.4; duration: 1000; easing.type: Easing.InOutCubic }
        PauseAnimation { duration: 1000 }
        NumberAnimation { target: renderer; property: "opacity"; to: 1.0; duration: 1000; easing.type: Easing.InOutCubic }
        ParallelAnimation {
            NumberAnimation { target: scale; property: "xScale"; to: 1; duration: 1000; easing.type: Easing.InOutBack }
            NumberAnimation { target: scale; property: "yScale"; to: 1; duration: 1000; easing.type: Easing.InOutBack }
        }
        running: true
        loops: Animation.Infinite
    }
}
