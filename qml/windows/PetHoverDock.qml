import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "../components"

Window {
    id: root

    property var ownerWindow: null
    property bool petHovered: false
    property bool suppressed: false
    readonly property bool controlsHovered: composeMouse.containsMouse
                                            || responseMouse.containsMouse

    objectName: "petHoverDock"
    width: 102
    height: 48
    visible: !suppressed
             && !petInteraction.chatVisible
             && !petInteraction.settingsVisible
             && (petHovered || controlsHovered || hideDelay.running)
    color: "transparent"
    title: qsTr("对话")
    transientParent: ownerWindow
    flags: Qt.FramelessWindowHint
           | Qt.Tool
           | Qt.WindowStaysOnTopHint
           | Qt.NoDropShadowWindowHint
           | Qt.WindowDoesNotAcceptFocus

    function petEntered() {
        petHovered = true
        hideDelay.stop()
        reposition()
    }

    function petExited() {
        petHovered = false
        if (!controlsHovered)
            hideDelay.restart()
    }

    function reposition() {
        if (!ownerWindow)
            return

        var preferredX = ownerWindow.x
                         + Math.round((ownerWindow.width - width) / 2)
        // A slight overlap keeps the control reachable while the pointer moves
        // from a transparent pet frame into this separate tool window.
        var preferredY = ownerWindow.y + ownerWindow.height - 4
        var point = petController.boundedPopupPosition(
                    preferredX, preferredY, width, height, 8)
        x = point.x
        y = point.y
    }

    onVisibleChanged: {
        if (visible) {
            reposition()
            revealAnimation.restart()
        } else {
            revealAnimation.stop()
            glassSurface.opacity = 1
            glassSurface.scale = 1
        }
    }

    Connections {
        target: ownerWindow
        ignoreUnknownSignals: true
        function onXChanged() { if (root.visible) root.reposition() }
        function onYChanged() { if (root.visible) root.reposition() }
        function onWidthChanged() { if (root.visible) root.reposition() }
        function onHeightChanged() { if (root.visible) root.reposition() }
        function onScreenChanged() { if (root.visible) root.reposition() }
        function onVisibleChanged() {
            if (!ownerWindow.visible)
                root.petHovered = false
        }
    }

    Timer {
        id: hideDelay
        interval: 260
        repeat: false
    }

    ParallelAnimation {
        id: revealAnimation

        NumberAnimation {
            target: glassSurface
            property: "opacity"
            from: 0
            to: 1
            duration: 130
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: glassSurface
            property: "scale"
            from: 0.92
            to: 1
            duration: 180
            easing.type: Easing.OutBack
        }
    }

    Rectangle {
        x: 8
        y: 9
        width: parent.width - 13
        height: parent.height - 12
        radius: 15
        color: "#28101B2B"
    }

    Rectangle {
        x: 5
        y: 6
        width: parent.width - 10
        height: parent.height - 10
        radius: 15
        color: "#12101B2B"
    }

    LiquidGlassSurface {
        id: glassSurface

        x: 3
        y: 2
        width: parent.width - 6
        height: parent.height - 7
        radius: 14
        clearMaterial: true
        highlighted: root.controlsHovered

        Row {
            anchors.fill: parent
            anchors.margins: 3

            Item {
                width: Math.floor((parent.width - divider.width) / 2)
                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 10
                    color: composeMouse.pressed
                           ? "#82FFFFFF"
                           : composeMouse.containsMouse ? "#56FFFFFF" : "transparent"
                    border.width: composeMouse.containsMouse ? 1 : 0
                    border.color: "#86FFFFFF"
                    Behavior on color { ColorAnimation { duration: 90 } }
                }

                Text {
                    anchors.centerIn: parent
                    text: "✎"
                    color: "#344151"
                    font.pixelSize: 20
                    font.family: "Segoe UI Symbol"
                    rotation: -5
                }

                MouseArea {
                    id: composeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: hideDelay.stop()
                    onExited: {
                        if (!root.petHovered && !root.controlsHovered)
                            hideDelay.restart()
                    }
                    onClicked: {
                        root.petHovered = false
                        hideDelay.stop()
                        petInteraction.openChat()
                    }
                    ToolTip.visible: containsMouse
                    ToolTip.delay: 500
                    ToolTip.text: qsTr("开始对话")
                }
            }

            Rectangle {
                id: divider
                anchors.verticalCenter: parent.verticalCenter
                width: 1
                height: 21
                color: "#709AA7B5"
            }

            Item {
                width: Math.ceil((parent.width - divider.width) / 2)
                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 10
                    color: responseMouse.pressed
                           ? "#82FFFFFF"
                           : responseMouse.containsMouse ? "#56FFFFFF" : "transparent"
                    border.width: responseMouse.containsMouse ? 1 : 0
                    border.color: "#86FFFFFF"
                    Behavior on color { ColorAnimation { duration: 90 } }
                }

                Item {
                    anchors.centerIn: parent
                    width: 22
                    height: 18

                    Rectangle { x: 1; y: 6; width: 2; height: 6; radius: 1; color: "#344151" }
                    Rectangle { x: 5; y: 3; width: 2; height: 12; radius: 1; color: "#344151" }
                    Rectangle { x: 9; y: 1; width: 2; height: 16; radius: 1; color: "#344151" }
                    Rectangle { x: 13; y: 4; width: 2; height: 10; radius: 1; color: "#344151" }
                    Rectangle { x: 17; y: 6; width: 2; height: 6; radius: 1; color: "#344151" }
                }

                MouseArea {
                    id: responseMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: hideDelay.stop()
                    onExited: {
                        if (!root.petHovered && !root.controlsHovered)
                            hideDelay.restart()
                    }
                    onClicked: {
                        root.petHovered = false
                        hideDelay.restart()
                        petInteraction.quickResponse()
                    }
                    ToolTip.visible: containsMouse
                    ToolTip.delay: 500
                    ToolTip.text: qsTr("让桌宠回应")
                }
            }
        }
    }
}
