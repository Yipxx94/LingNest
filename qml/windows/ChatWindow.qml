import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "../components"

Window {
    id: root

    property var ownerWindow: null
    property bool belowPet: false
    property bool hasBeenActive: false

    objectName: "chatWindow"
    width: 392
    height: 66
    visible: petInteraction.chatVisible
    title: qsTr("与 %1 对话").arg(chatController.displayName)
    color: "transparent"
    transientParent: ownerWindow
    flags: Qt.FramelessWindowHint
           | Qt.Tool
           | Qt.WindowStaysOnTopHint
           | Qt.NoDropShadowWindowHint

    function submitMessage() {
        var message = input.text.trim()
        if (chatController.busy || message.length === 0)
            return

        chatController.sendMessage(message)
        input.clear()
        petInteraction.closeChat()
    }

    function reposition() {
        if (!ownerWindow)
            return

        var preferredX = ownerWindow.x
                         + Math.round((ownerWindow.width - width) / 2)
        var belowY = ownerWindow.y + ownerWindow.height + 8
        var preferredY = belowY
        belowPet = true

        var targetScreen = ownerWindow.screen
        if (targetScreen && targetScreen.availableGeometry !== undefined) {
            var area = targetScreen.availableGeometry
            if (belowY + height > area.y + area.height - 10) {
                belowPet = false
                preferredY = ownerWindow.y - height - 8
            }
        }

        var point = petController.boundedPopupPosition(
                    preferredX, preferredY, width, height, 10)
        x = point.x
        y = point.y
    }

    onVisibleChanged: {
        if (visible) {
            hasBeenActive = false
            reposition()
            requestActivate()
            input.forceActiveFocus()
            appearAnimation.restart()
        } else {
            appearAnimation.stop()
            opacity = 1
        }
    }
    onActiveChanged: {
        if (active)
            hasBeenActive = true
        else if (visible && hasBeenActive)
            petInteraction.closeChat()
    }
    onClosing: {
        close.accepted = false
        petInteraction.closeChat()
    }

    Connections {
        target: ownerWindow
        ignoreUnknownSignals: true
        function onXChanged() { if (root.visible) root.reposition() }
        function onYChanged() { if (root.visible) root.reposition() }
        function onWidthChanged() { if (root.visible) root.reposition() }
        function onHeightChanged() { if (root.visible) root.reposition() }
        function onScreenChanged() { if (root.visible) root.reposition() }
    }

    Shortcut {
        sequence: "Esc"
        enabled: root.visible
        onActivated: petInteraction.closeChat()
    }

    ParallelAnimation {
        id: appearAnimation

        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            duration: 110
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: surface
            property: "scale"
            from: 0.97
            to: 1
            duration: 140
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        x: 10
        y: 8
        width: parent.width - 14
        height: 54
        radius: 27
        color: "#28101B2B"
    }

    Rectangle {
        x: 7
        y: 5
        width: parent.width - 14
        height: 54
        radius: 27
        color: "#12101B2B"
    }

    LiquidGlassSurface {
        id: surface

        x: 3
        y: 2
        width: parent.width - 6
        height: 54
        radius: 27
        clearMaterial: false
        highlighted: input.activeFocus

        TextField {
            id: input

            objectName: "chatComposerInput"
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.right: sendButton.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            height: 42
            placeholderText: chatController.busy
                             ? qsTr("%1 正在思考…").arg(chatController.displayName)
                             : qsTr("开始和 %1 聊天…").arg(chatController.displayName)
            enabled: !chatController.busy
            selectByMouse: true
            color: "#27313E"
            placeholderTextColor: "#9AA3AE"
            font.pixelSize: 14
            font.family: "Segoe UI"
            background: Item {}

            onAccepted: root.submitMessage()
        }

        Button {
            id: sendButton

            objectName: "chatComposerSendButton"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 38
            height: 38
            enabled: !chatController.busy && input.text.trim().length > 0
            hoverEnabled: true
            onClicked: root.submitMessage()
            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("发送")

            contentItem: Canvas {
                property string arrowColor: sendButton.enabled
                                            ? "#FFFFFF" : "#AAB4C1"

                onArrowColorChanged: requestPaint()
                onPaint: {
                    var painter = getContext("2d")
                    var centerX = width / 2
                    var centerY = height / 2
                    painter.clearRect(0, 0, width, height)
                    painter.strokeStyle = arrowColor
                    painter.lineWidth = 2.8
                    painter.lineCap = "round"
                    painter.lineJoin = "round"
                    painter.beginPath()
                    painter.moveTo(centerX, centerY + 7)
                    painter.lineTo(centerX, centerY - 7)
                    painter.moveTo(centerX - 6, centerY - 1)
                    painter.lineTo(centerX, centerY - 7)
                    painter.lineTo(centerX + 6, centerY - 1)
                    painter.stroke()
                }
            }

            background: Rectangle {
                radius: 19
                color: !sendButton.enabled
                       ? "#70E4ECF7"
                       : sendButton.down ? "#EE3F70C4"
                       : sendButton.hovered ? "#ED4F82D4" : "#E65D8DDD"
                border.width: 1
                border.color: sendButton.enabled ? "#C8FFFFFF" : "#7FFFFFFF"

                Behavior on color { ColorAnimation { duration: 90 } }
            }
        }
    }
}
