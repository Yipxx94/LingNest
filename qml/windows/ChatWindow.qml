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

        Rectangle {
            id: leadingBadge

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 36
            height: 36
            radius: 18
            color: leadingMouse.containsMouse ? "#6AA9C8F8" : "#52FFFFFF"
            border.width: 1
            border.color: "#74FFFFFF"

            Behavior on color { ColorAnimation { duration: 90 } }

            Text {
                anchors.centerIn: parent
                text: chatController.busy
                      ? "···"
                      : input.text.length > 0 ? "×" : "+"
                color: "#657386"
                font.pixelSize: chatController.busy
                                ? 14 : input.text.length > 0 ? 18 : 22
                font.weight: Font.Light
                font.family: "Segoe UI"
            }

            MouseArea {
                id: leadingMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (input.text.length > 0)
                        input.clear()
                    input.forceActiveFocus()
                }
                ToolTip.visible: containsMouse
                ToolTip.delay: 500
                ToolTip.text: input.text.length > 0
                              ? qsTr("清空输入") : qsTr("输入消息")
            }

            SequentialAnimation on opacity {
                running: chatController.busy
                loops: Animation.Infinite
                NumberAnimation { to: 0.55; duration: 480 }
                NumberAnimation { to: 1.0; duration: 480 }
            }
        }

        TextField {
            id: input

            objectName: "chatComposerInput"
            anchors.left: parent.left
            anchors.leftMargin: 52
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

            contentItem: Text {
                text: "↑"
                color: sendButton.enabled ? "#FFFFFF" : "#AAB4C1"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 19
                font.weight: Font.DemiBold
                font.family: "Segoe UI"
            }

            background: Rectangle {
                radius: 19
                color: !sendButton.enabled
                       ? "#70E4ECF7"
                       : sendButton.down ? "#D577A7EA"
                       : sendButton.hovered ? "#E9A8C9FA" : "#D89CC1F7"
                border.width: 1
                border.color: sendButton.enabled ? "#C8FFFFFF" : "#7FFFFFFF"

                Behavior on color { ColorAnimation { duration: 90 } }
            }
        }
    }
}
