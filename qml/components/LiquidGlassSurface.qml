import QtQuick 2.15

Rectangle {
    id: root

    property bool clearMaterial: false
    property bool highlighted: false
    property color accentColor: "#86B3F5"

    color: clearMaterial ? "#D7F8FAFD" : "#EEF5F8FC"
    border.width: 1
    border.color: highlighted ? "#F4FFFFFF" : "#D8FFFFFF"
    antialiasing: true
    clip: true

    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 120 } }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: root.clearMaterial ? "#A8FFFFFF" : "#C4FFFFFF"
            }
            GradientStop {
                position: 0.42
                color: root.clearMaterial ? "#36FFFFFF" : "#58FFFFFF"
            }
            GradientStop {
                position: 1.0
                color: root.highlighted ? "#2F86B5F3" : "#1D91B9E8"
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Math.max(8, parent.height * 0.28)
        color: "transparent"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00FFFFFF" }
            GradientStop { position: 1.0; color: "#1686B3ED" }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: "#62FFFFFF"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: Math.max(12, parent.radius)
        anchors.rightMargin: Math.max(12, parent.radius)
        anchors.topMargin: 2
        height: 1
        radius: 1
        color: "#C8FFFFFF"
    }
}
