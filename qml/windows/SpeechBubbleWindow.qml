import QtQuick 2.15
import QtQuick.Window 2.15
import "../components"

Window {
    id: root

    readonly property int messageBodyHeight: Math.max(
        68,
        Math.min(330, messageText.implicitHeight + 52))

    objectName: "speechBubbleWindow"
    width: 344
    height: (bubbleController.thinking ? 68 : messageBodyHeight) + 4
    visible: bubbleController.visible && !petInteraction.chatVisible
    color: "transparent"
    flags: Qt.FramelessWindowHint
           | Qt.WindowStaysOnTopHint
           | Qt.Tool
           | Qt.WindowTransparentForInput
           | Qt.WindowDoesNotAcceptFocus
           | Qt.NoDropShadowWindowHint

    Rectangle {
        x: 10
        y: 7
        width: parent.width - 14
        height: parent.height - 9
        radius: 25
        color: "#28101B2B"
    }

    LiquidGlassSurface {
        id: body

        x: 3
        y: 2
        width: parent.width - 6
        height: parent.height - 6
        radius: 24
        clearMaterial: false
        highlighted: bubbleController.thinking

        Row {
            id: identityRow

            x: 17
            y: 12
            spacing: 7

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 14
                height: 14
                radius: 7
                color: bubbleController.thinking ? "#F0B45E" : "#76A9F5"

                Text {
                    anchors.centerIn: parent
                    visible: !bubbleController.thinking
                    text: "✓"
                    color: "#FFFFFF"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.family: "Segoe UI"
                }
            }

            Text {
                text: chatController.displayName
                color: "#6E7A88"
                font.pixelSize: 10
                font.weight: Font.DemiBold
                font.family: "Segoe UI"
            }
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.top: identityRow.bottom
            anchors.topMargin: 11
            spacing: 6
            visible: bubbleController.thinking

            Repeater {
                model: 3

                Rectangle {
                    width: 7
                    height: 7
                    radius: 4
                    color: "#76A9F5"
                    opacity: 0.28

                    SequentialAnimation on opacity {
                        running: bubbleController.thinking
                        loops: Animation.Infinite
                        PauseAnimation { duration: index * 120 }
                        NumberAnimation { to: 1.0; duration: 220 }
                        NumberAnimation { to: 0.28; duration: 300 }
                        PauseAnimation { duration: (2 - index) * 120 }
                    }

                    SequentialAnimation on y {
                        running: bubbleController.thinking
                        loops: Animation.Infinite
                        PauseAnimation { duration: index * 120 }
                        NumberAnimation { to: -3; duration: 180; easing.type: Easing.OutCubic }
                        NumberAnimation { to: 0; duration: 220; easing.type: Easing.InCubic }
                        PauseAnimation { duration: (2 - index) * 120 }
                    }
                }
            }
        }

        Text {
            id: messageText

            anchors.left: parent.left
            anchors.leftMargin: 17
            anchors.right: parent.right
            anchors.rightMargin: 17
            anchors.top: identityRow.bottom
            anchors.topMargin: 7
            visible: !bubbleController.thinking
            text: bubbleController.text
            color: "#27313E"
            font.pixelSize: 14
            font.family: "Segoe UI"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignTop
            wrapMode: Text.Wrap
            lineHeight: 1.16
            maximumLineCount: 13
            elide: Text.ElideRight
        }
    }
}
