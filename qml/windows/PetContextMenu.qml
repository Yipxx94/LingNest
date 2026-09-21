import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "../components"

Window {
    id: root

    property var ownerWindow: null
    property bool actionsPage: false
    property bool hasBeenActive: false
    // The two 6 px card margins and two 8 px content margins need an
    // additional 8 px below the last button so the rounded glass edge
    // never clips its text or hover state.
    readonly property int contentChromeHeight: 2 * (6 + 8) + 8

    objectName: "contextMenu"
    width: 232
    height: Math.ceil((actionsPage ? actionsContent.implicitHeight
                                   : mainContent.implicitHeight)
                      + contentChromeHeight)
    visible: false
    color: "transparent"
    title: qsTr("LingNest")
    transientParent: ownerWindow
    flags: Qt.Tool
           | Qt.FramelessWindowHint
           | Qt.WindowStaysOnTopHint
           | Qt.NoDropShadowWindowHint

    function boundedPosition(preferredX, preferredY) {
        return petController.boundedPopupPosition(
                    preferredX, preferredY, width, height, 10)
    }

    function openAt(globalX, globalY, targetScreen) {
        actionsPage = false
        hasBeenActive = false
        var point = boundedPosition(globalX + 6, globalY + 6)
        x = point.x
        y = point.y
        visible = true
        requestActivate()
        openAnimation.restart()
        // Column implicitHeight may settle only after the popup is visible.
        // Clamp once more with the final measured window size.
        positionTimer.restart()
    }

    function closeMenu() {
        visible = false
        actionsPage = false
    }

    function clampToScreen() {
        var point = boundedPosition(x, y)
        x = point.x
        y = point.y
    }

    onActionsPageChanged: {
        if (visible)
            positionTimer.restart()
    }
    onWidthChanged: {
        if (visible)
            positionTimer.restart()
    }
    onHeightChanged: {
        if (visible)
            positionTimer.restart()
    }
    onActiveChanged: {
        if (active)
            hasBeenActive = true
        else if (visible && hasBeenActive)
            closeMenu()
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (root.visible && Qt.application.state !== Qt.ApplicationActive)
                root.closeMenu()
        }
    }

    Timer {
        id: positionTimer
        interval: 0
        repeat: false
        onTriggered: root.clampToScreen()
    }

    Shortcut {
        sequence: "Esc"
        enabled: root.visible
        onActivated: {
            if (root.actionsPage)
                root.actionsPage = false
            else
                root.closeMenu()
        }
    }

    ParallelAnimation {
        id: openAnimation

        NumberAnimation {
            target: card
            property: "opacity"
            from: 0
            to: 1
            duration: 130
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: card
            property: "scale"
            from: 0.96
            to: 1
            duration: 180
            easing.type: Easing.OutBack
        }
    }

    Rectangle {
        x: 10
        y: 12
        width: parent.width - 14
        height: parent.height - 15
        radius: 22
        color: "#26101B2B"
    }

    Rectangle {
        x: 7
        y: 8
        width: parent.width - 12
        height: parent.height - 13
        radius: 21
        color: "#12101B2B"
    }

    LiquidGlassSurface {
        id: card

        anchors.fill: parent
        anchors.margins: 6
        radius: 20
        clearMaterial: false
        highlighted: root.active

        Item {
            anchors.fill: parent
            anchors.margins: 8
            visible: opacity > 0
            enabled: !root.actionsPage
            opacity: root.actionsPage ? 0 : 1
            transform: Translate {
                x: root.actionsPage ? -12 : 0
                Behavior on x {
                    NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                }
            }
            Behavior on opacity { NumberAnimation { duration: 120 } }

            Column {
                id: mainContent
                objectName: "mainMenuContent"
                anchors.fill: parent
                spacing: 2

                Item {
                    width: parent.width
                    height: 50

                    Rectangle {
                        id: menuAvatarPlate

                        anchors.left: parent.left
                        anchors.leftMargin: 9
                        anchors.verticalCenter: parent.verticalCenter
                        width: 32
                        height: 32
                        radius: 16
                        color: "#78E5EFFB"
                        border.width: 1
                        border.color: "#7FFFFFFF"
                        clip: true

                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            source: chatController.avatarUrl
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }

                    Column {
                        anchors.left: menuAvatarPlate.right
                        anchors.leftMargin: 10
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 1

                        Text {
                            width: parent.width
                            text: chatController.displayName
                            color: "#27313E"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            font.family: "Segoe UI"
                        }
                        Text {
                            width: parent.width
                            text: petInteraction.paused
                                  ? qsTr("活动已暂停")
                                  : qsTr("正在陪伴")
                            color: "#778391"
                            font.pixelSize: 10
                            font.family: "Segoe UI"
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#52FFFFFF"
                }
                Item { width: 1; height: 4 }

                WarmMenuButton {
                    objectName: "chatMenuButton"
                    width: parent.width
                    text: qsTr("开始对话")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.openChat()
                    }
                }
                WarmMenuButton {
                    objectName: "settingsMenuButton"
                    width: parent.width
                    text: qsTr("AI 设置")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.showSettings()
                    }
                }
                WarmMenuButton {
                    objectName: "switchMenuButton"
                    width: parent.width
                    text: qsTr("切换角色")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.showCharacterSwitcher()
                    }
                }
                WarmMenuButton {
                    objectName: "pauseMenuButton"
                    width: parent.width
                    text: petInteraction.paused ? qsTr("继续活动") : qsTr("暂停活动")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.togglePaused()
                    }
                }
                WarmMenuButton {
                    objectName: "previewMenuButton"
                    width: parent.width
                    text: qsTr("动作预览")
                    showsArrow: true
                    onClicked: root.actionsPage = true
                }
                WarmMenuButton {
                    objectName: "quitMenuButton"
                    width: parent.width
                    text: qsTr("退出 LingNest")
                    destructive: true
                    onClicked: {
                        root.closeMenu()
                        petInteraction.quitApplication()
                    }
                }
            }
        }

        Item {
            anchors.fill: parent
            anchors.margins: 8
            visible: opacity > 0
            enabled: root.actionsPage
            opacity: root.actionsPage ? 1 : 0
            transform: Translate {
                x: root.actionsPage ? 0 : 12
                Behavior on x {
                    NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                }
            }
            Behavior on opacity { NumberAnimation { duration: 120 } }

            Column {
                id: actionsContent
                objectName: "actionsMenuContent"
                anchors.fill: parent
                spacing: 2

                WarmMenuButton {
                    width: parent.width
                    height: 40
                    backIndicator: true
                    text: qsTr("动作预览")
                    onClicked: root.actionsPage = false
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#52FFFFFF"
                }
                Item { width: 1; height: 4 }

                WarmMenuButton {
                    width: parent.width
                    text: qsTr("恢复待机")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("idle")
                    }
                }
                WarmMenuButton {
                    width: parent.width
                    text: qsTr("眨眼")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("blink")
                    }
                }
                WarmMenuButton {
                    width: parent.width
                    text: qsTr("开心")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("happy")
                    }
                }
                WarmMenuButton {
                    width: parent.width
                    text: qsTr("挥手")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("wave")
                    }
                }
                WarmMenuButton {
                    width: parent.width
                    text: qsTr("趴伏发呆")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("daze")
                    }
                }
                WarmMenuButton {
                    width: parent.width
                    text: qsTr("向左走")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("walkLeft")
                    }
                }
                WarmMenuButton {
                    width: parent.width
                    text: qsTr("向右走")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("walkRight")
                    }
                }
                WarmMenuButton {
                    objectName: "sleepMenuButton"
                    width: parent.width
                    text: qsTr("睡觉")
                    onClicked: {
                        root.closeMenu()
                        petInteraction.playDebugAction("sleep")
                    }
                }
            }
        }
    }
}
