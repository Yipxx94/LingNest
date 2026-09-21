import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "../components"

Window {
    id: root

    property var ownerWindow: null

    objectName: "settingsWindow"
    width: 510
    height: 580
    visible: petInteraction.settingsVisible
    title: qsTr("LingNest AI 设置")
    color: "transparent"
    transientParent: ownerWindow
    flags: Qt.Tool
           | Qt.FramelessWindowHint
           | Qt.WindowStaysOnTopHint
           | Qt.NoDropShadowWindowHint

    function ownerAvailableArea() {
        var area = petController.availableScreenGeometry()
        return area.width > 0 && area.height > 0 ? area : null
    }

    function moveWithinScreen(preferredX, preferredY) {
        var area = ownerAvailableArea()
        if (!area)
            return

        var margin = 12
        var minimumX = area.x + margin
        var minimumY = area.y + margin
        var maximumX = Math.max(minimumX,
                                area.x + area.width - width - margin)
        var maximumY = Math.max(minimumY,
                                area.y + area.height - height - margin)
        x = Math.max(minimumX, Math.min(Math.round(preferredX), maximumX))
        y = Math.max(minimumY, Math.min(Math.round(preferredY), maximumY))
    }

    function fitToOwnerScreen() {
        if (!ownerWindow)
            return

        var area = ownerAvailableArea()
        if (!area)
            return

        width = Math.min(510, Math.max(1, area.width - 24))
        height = Math.min(580, Math.max(1, area.height - 24))
        moveWithinScreen(
                    ownerWindow.x + (ownerWindow.width - width) / 2,
                    ownerWindow.y + (ownerWindow.height - height) / 2)
    }

    function reloadFields() {
        baseUrlInput.text = aiSettings.baseUrl
        modelInput.text = aiSettings.model
        temperatureInput.text = Number(aiSettings.temperature).toFixed(2)
        maxTokensInput.text = String(aiSettings.maxTokens)
        timeoutInput.text = String(aiSettings.timeoutSeconds)
        apiKeyInput.clear()
    }

    function fieldsValid() {
        return baseUrlInput.text.trim().length > 0
                && modelInput.text.trim().length > 0
                && temperatureInput.acceptableInput
                && maxTokensInput.acceptableInput
                && timeoutInput.acceptableInput
    }

    function saveFields() {
        if (!fieldsValid())
            return

        aiSettings.save(
                    baseUrlInput.text.trim(),
                    modelInput.text.trim(),
                    Number(temperatureInput.text.replace(",", ".")),
                    Number(maxTokensInput.text),
                    Number(timeoutInput.text),
                    apiKeyInput.text)
        apiKeyInput.clear()
    }

    onVisibleChanged: {
        if (visible) {
            reloadFields()
            fitToOwnerScreen()
            positionTimer.restart()
            requestActivate()
            baseUrlInput.forceActiveFocus()
            appearAnimation.restart()
        } else {
            appearAnimation.stop()
            opacity = 1
        }
    }
    onClosing: {
        close.accepted = false
        petInteraction.closeSettings()
    }
    onXChanged: { if (visible) positionTimer.restart() }
    onYChanged: { if (visible) positionTimer.restart() }
    onWidthChanged: { if (visible) positionTimer.restart() }
    onHeightChanged: { if (visible) positionTimer.restart() }

    Timer {
        id: positionTimer
        interval: 0
        repeat: false
        onTriggered: root.moveWithinScreen(root.x, root.y)
    }

    Shortcut {
        sequence: "Esc"
        enabled: root.visible
        onActivated: petInteraction.closeSettings()
    }

    ParallelAnimation {
        id: appearAnimation

        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            duration: 120
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: card
            property: "scale"
            from: 0.98
            to: 1
            duration: 160
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        x: 10
        y: 12
        width: parent.width - 14
        height: parent.height - 16
        radius: 25
        color: "#28101B2B"
    }

    Rectangle {
        x: 7
        y: 8
        width: parent.width - 13
        height: parent.height - 14
        radius: 24
        color: "#12101B2B"
    }

    LiquidGlassSurface {
        id: card

        objectName: "settingsGlassCard"
        anchors.fill: parent
        anchors.margins: 6
        radius: 23
        highlighted: root.active

        Item {
            id: header

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 76

            MouseArea {
                id: dragHandle

                property real pointerStartX: 0
                property real pointerStartY: 0
                property real windowStartX: 0
                property real windowStartY: 0

                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

                onPressed: {
                    pointerStartX = root.x + mouse.x
                    pointerStartY = root.y + mouse.y
                    windowStartX = root.x
                    windowStartY = root.y
                }
                onPositionChanged: {
                    if (!(mouse.buttons & Qt.LeftButton))
                        return
                    root.moveWithinScreen(
                                windowStartX + root.x + mouse.x - pointerStartX,
                                windowStartY + root.y + mouse.y - pointerStartY)
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                width: 38
                height: 38
                radius: 13
                color: "#91DCEAFB"
                border.width: 1
                border.color: "#BFFFFFFF"

                Text {
                    anchors.centerIn: parent
                    text: "✦"
                    color: "#527FC5"
                    font.pixelSize: 22
                    font.family: "Segoe UI Symbol"
                }
            }

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 69
                anchors.right: headerCloseButton.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: qsTr("AI 设置")
                    color: "#27313E"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                    font.family: "Segoe UI"
                }
                Text {
                    text: qsTr("连接与你习惯的模型服务")
                    color: "#778391"
                    font.pixelSize: 11
                    font.family: "Segoe UI"
                }
            }

            Button {
                id: headerCloseButton

                objectName: "settingsCloseButton"
                anchors.right: parent.right
                anchors.rightMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                width: 34
                height: 34
                hoverEnabled: true
                onClicked: petInteraction.closeSettings()
                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: qsTr("关闭")

                contentItem: Text {
                    text: "×"
                    color: "#627083"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 24
                    font.family: "Segoe UI"
                }
                background: Rectangle {
                    radius: 17
                    color: headerCloseButton.hovered
                           ? "#8AFFFFFF" : "#45FFFFFF"
                    border.width: 1
                    border.color: "#8FFFFFFF"
                }
            }
        }

        Rectangle {
            id: headerDivider

            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            height: 1
            color: "#78FFFFFF"
        }

        ScrollView {
            id: formScroll

            objectName: "settingsFormScroll"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: headerDivider.bottom
            anchors.bottom: footer.top
            anchors.leftMargin: 23
            anchors.rightMargin: 17
            anchors.topMargin: 14
            anchors.bottomMargin: 8
            clip: true
            background: Item {}
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                id: form

                width: formScroll.availableWidth
                spacing: 12

                Text {
                    width: parent.width
                    text: qsTr("支持兼容 Chat Completions 的服务。API Key 只保存在 Windows 凭据管理器中。")
                    color: "#687687"
                    wrapMode: Text.Wrap
                    font.pixelSize: 12
                    font.family: "Segoe UI"
                }

                Column {
                    width: parent.width
                    spacing: 6
                    Text {
                        text: qsTr("Base URL")
                        color: "#465363"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        font.family: "Segoe UI"
                    }
                    TextField {
                        id: baseUrlInput
                        width: parent.width
                        height: 42
                        placeholderText: "https://api.openai.com/v1"
                        selectByMouse: true
                        color: "#27313E"
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        background: Rectangle {
                            radius: 12
                            color: "#BCFFFFFF"
                            border.width: 1
                            border.color: baseUrlInput.activeFocus
                                          ? "#A56A99E0" : "#B8FFFFFF"
                        }
                    }
                }

                Column {
                    width: parent.width
                    spacing: 6
                    Text {
                        text: qsTr("模型")
                        color: "#465363"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        font.family: "Segoe UI"
                    }
                    TextField {
                        id: modelInput
                        width: parent.width
                        height: 42
                        placeholderText: qsTr("填写服务支持的模型名称")
                        selectByMouse: true
                        color: "#27313E"
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        background: Rectangle {
                            radius: 12
                            color: "#BCFFFFFF"
                            border.width: 1
                            border.color: modelInput.activeFocus
                                          ? "#A56A99E0" : "#B8FFFFFF"
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 12

                    Column {
                        width: (parent.width - 12) / 2
                        spacing: 6
                        Text {
                            text: qsTr("Temperature（0–2）")
                            color: "#465363"
                            font.pixelSize: 12
                            font.weight: Font.Normal
                            font.family: "Segoe UI"
                        }
                        TextField {
                            id: temperatureInput
                            width: parent.width
                            height: 42
                            selectByMouse: true
                            color: "#27313E"
                            font.pixelSize: 13
                            font.family: "Segoe UI"
                            validator: DoubleValidator {
                                bottom: 0.0
                                top: 2.0
                                decimals: 2
                                notation: DoubleValidator.StandardNotation
                            }
                            background: Rectangle {
                                radius: 12
                                color: "#BCFFFFFF"
                                border.width: 1
                                border.color: temperatureInput.activeFocus
                                              ? "#A56A99E0" : "#B8FFFFFF"
                            }
                        }
                    }

                    Column {
                        width: (parent.width - 12) / 2
                        spacing: 6
                        Text {
                            text: qsTr("Max Tokens")
                            color: "#465363"
                            font.pixelSize: 12
                            font.weight: Font.Normal
                            font.family: "Segoe UI"
                        }
                        TextField {
                            id: maxTokensInput
                            objectName: "settingsMaxTokensInput"
                            width: parent.width
                            height: 42
                            selectByMouse: true
                            color: "#27313E"
                            font.pixelSize: 13
                            font.family: "Segoe UI"
                            validator: IntValidator { bottom: 1; top: 1000000 }
                            inputMethodHints: Qt.ImhDigitsOnly
                            background: Rectangle {
                                radius: 12
                                color: "#BCFFFFFF"
                                border.width: 1
                                border.color: maxTokensInput.activeFocus
                                              ? "#A56A99E0" : "#B8FFFFFF"
                            }
                        }
                    }
                }

                Column {
                    width: parent.width
                    spacing: 6
                    Text {
                        text: qsTr("超时时间（秒）")
                        color: "#465363"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        font.family: "Segoe UI"
                    }
                    TextField {
                        id: timeoutInput
                        width: parent.width
                        height: 42
                        selectByMouse: true
                        color: "#27313E"
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        validator: IntValidator { bottom: 1; top: 600 }
                        inputMethodHints: Qt.ImhDigitsOnly
                        background: Rectangle {
                            radius: 12
                            color: "#BCFFFFFF"
                            border.width: 1
                            border.color: timeoutInput.activeFocus
                                          ? "#A56A99E0" : "#B8FFFFFF"
                        }
                    }
                }

                Column {
                    width: parent.width
                    spacing: 6
                    Text {
                        text: aiSettings.apiKeyConfigured
                              ? qsTr("API Key · 已安全保存")
                              : qsTr("API Key · 尚未配置")
                        color: "#465363"
                        font.pixelSize: 12
                        font.weight: Font.Normal
                        font.family: "Segoe UI"
                    }
                    TextField {
                        id: apiKeyInput
                        width: parent.width
                        height: 42
                        placeholderText: aiSettings.apiKeyConfigured
                                         ? qsTr("留空以保持原密钥")
                                         : qsTr("输入 API Key")
                        echoMode: TextInput.Password
                        selectByMouse: true
                        color: "#27313E"
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        background: Rectangle {
                            radius: 12
                            color: "#BCFFFFFF"
                            border.width: 1
                            border.color: apiKeyInput.activeFocus
                                          ? "#A56A99E0" : "#B8FFFFFF"
                        }
                    }
                }

                Text {
                    width: parent.width
                    visible: text.length > 0
                    text: aiSettings.statusMessage
                    color: aiSettings.statusError ? "#BE5755" : "#527FC5"
                    wrapMode: Text.Wrap
                    font.pixelSize: 12
                    font.family: "Segoe UI"
                }
            }
        }

        Item {
            id: footer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 66

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                height: 1
                color: "#78FFFFFF"
            }

            Button {
                id: deleteKeyButton

                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 2
                width: 88
                height: 38
                enabled: aiSettings.apiKeyConfigured
                hoverEnabled: true
                onClicked: {
                    aiSettings.clearApiKey()
                    apiKeyInput.clear()
                }
                contentItem: Text {
                    text: qsTr("删除密钥")
                    color: deleteKeyButton.enabled ? "#BE5755" : "#9AA6B4"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    font.family: "Segoe UI"
                }
                background: Rectangle {
                    radius: 12
                    color: deleteKeyButton.hovered && deleteKeyButton.enabled
                           ? "#38E8B4B2" : "#55FFFFFF"
                    border.width: 1
                    border.color: "#8FFFFFFF"
                }
            }

            Button {
                id: saveButton

                objectName: "settingsSaveButton"

                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 2
                width: 90
                height: 38
                enabled: root.fieldsValid()
                hoverEnabled: true
                onClicked: root.saveFields()
                contentItem: Text {
                    text: qsTr("保存")
                    color: saveButton.enabled ? "#FFFFFF" : "#AAB4C1"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.family: "Segoe UI"
                }
                background: Rectangle {
                    radius: 12
                    color: !saveButton.enabled
                           ? "#70E4ECF7"
                           : saveButton.down ? "#EE3F70C4"
                           : saveButton.hovered ? "#ED4F82D4" : "#E65D8DDD"
                    border.width: 1
                    border.color: "#BFFFFFFF"
                }
            }

            Button {
                id: footerCloseButton

                anchors.right: saveButton.left
                anchors.rightMargin: 9
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 2
                width: 76
                height: 38
                hoverEnabled: true
                onClicked: petInteraction.closeSettings()
                contentItem: Text {
                    text: qsTr("关闭")
                    color: "#465363"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.family: "Segoe UI"
                }
                background: Rectangle {
                    radius: 12
                    color: footerCloseButton.hovered
                           ? "#9BFFFFFF" : "#66FFFFFF"
                    border.width: 1
                    border.color: "#AFFFFFFF"
                }
            }
        }
    }
}
