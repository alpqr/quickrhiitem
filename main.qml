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

    TestRhiItem {
        anchors.centerIn: parent
        width: 400
        height: 400
    }
}
