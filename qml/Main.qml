import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "windows"

Window {
    id: root

    objectName: "petWindow"
    width: Math.round(192 * petController.scale)
    height: Math.round(208 * petController.scale)
    visible: true
    title: petController.displayName
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool

    Image {
        id: petImage

        anchors.fill: parent
        source: petAnimation.currentFrame
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        cache: true
    }

    PetContextMenu {
        id: contextMenu

        ownerWindow: root
    }

    PetHoverDock {
        id: hoverDock

        ownerWindow: root
        suppressed: contextMenu.visible || !root.visible
    }

    SpeechBubbleWindow {
        id: speechBubbleWindow

        transientParent: root
    }

    ChatWindow {
        id: chatWindow

        ownerWindow: root
    }

    SettingsWindow {
        id: settingsWindow

        ownerWindow: root
    }

    MouseArea {
        id: gestureArea

        property real pressGlobalX: 0
        property real pressGlobalY: 0
        property bool dragged: false

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        cursorShape: pressed && pressedButtons & Qt.LeftButton
                     ? Qt.ClosedHandCursor
                     : Qt.OpenHandCursor
        preventStealing: true

        onEntered: hoverDock.petEntered()
        onExited: hoverDock.petExited()

        onPressed: {
            if (mouse.button !== Qt.LeftButton)
                return

            contextMenu.closeMenu()
            dragged = false
            pressGlobalX = root.x + mouse.x
            pressGlobalY = root.y + mouse.y
            petController.beginDrag(pressGlobalX, pressGlobalY)
        }
        onPositionChanged: {
            if (!(mouse.buttons & Qt.LeftButton))
                return

            var globalX = root.x + mouse.x
            var globalY = root.y + mouse.y
            if (!dragged
                    && Math.abs(globalX - pressGlobalX)
                       + Math.abs(globalY - pressGlobalY)
                       >= petController.dragThreshold) {
                dragged = true
                petController.startSystemDrag()
            }
            if (dragged)
                petController.updateDragFromCursor(globalX, globalY)
        }
        onReleased: {
            if (mouse.button === Qt.LeftButton)
                dragged = petController.endDrag() || dragged
        }
        onCanceled: {
            dragged = petController.endDrag() || dragged
        }
        onClicked: {
            if (mouse.button === Qt.RightButton) {
                var menuX = root.x + mouse.x
                var menuY = root.y + mouse.y
                petInteraction.closeChat()
                Qt.callLater(function() {
                    contextMenu.openAt(menuX, menuY, root.screen)
                })
            } else if (!dragged) {
                petInteraction.clickCandidate()
            }
        }
        onDoubleClicked: {
            if (mouse.button === Qt.LeftButton && !dragged) {
                petInteraction.doubleClick()
                mouse.accepted = true
            }
        }
    }
}
