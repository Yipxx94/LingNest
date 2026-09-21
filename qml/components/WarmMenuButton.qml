import QtQuick 2.15
import QtQuick.Controls 2.15

AbstractButton {
    id: control

    property bool destructive: false
    property bool showsArrow: false
    property bool backIndicator: false
    readonly property bool pointerHovered: control.enabled && pointerHover.hovered

    implicitHeight: 36
    hoverEnabled: true
    activeFocusOnTab: true

    HoverHandler {
        id: pointerHover
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Item {
        Text {
            id: backGlyph

            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            visible: control.backIndicator
            text: "‹"
            color: control.enabled ? "#4E5968" : "#98A2AE"
            font.pixelSize: 21
            font.family: "Segoe UI"
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: control.backIndicator ? 31 : 12
            anchors.right: arrowLabel.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: control.text
            color: !control.enabled
                   ? "#98A2AE"
                   : control.destructive ? "#C74E4A" : "#293440"
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 13
            font.weight: Font.Normal
            font.family: "Segoe UI"
        }

        Text {
            id: arrowLabel

            anchors.right: parent.right
            anchors.rightMargin: 11
            anchors.verticalCenter: parent.verticalCenter
            visible: control.showsArrow
            text: "›"
            color: "#6F7B89"
            font.pixelSize: 20
            font.family: "Segoe UI"
        }
    }

    background: Rectangle {
        radius: 10
        color: control.down
               ? (control.destructive ? "#24D65A54" : "#78FFFFFF")
               : control.pointerHovered
                 ? (control.destructive ? "#18D65A54" : "#4CFFFFFF")
                 : "transparent"
        border.width: control.pointerHovered || control.activeFocus ? 1 : 0
        border.color: control.destructive ? "#32D65A54" : "#7FFFFFFF"

        Behavior on color { ColorAnimation { duration: 100 } }
    }
}
