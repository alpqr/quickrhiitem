import QtQuick
import TestApp

Item {
    Text {
        color: "#ffffff"
        style: Text.Outline
        styleColor: "#606060"
        font.pixelSize: 28
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

    Rectangle {
        color: "red"
        width: 300
        height: 300
        anchors.centerIn: parent
        NumberAnimation on rotation { from: 0; to: 360; duration: 5000; loops: -1 }
    }

    TestRhiItem {
        anchors.right: parent.right
        width: 200
        height: 200
    }
}
