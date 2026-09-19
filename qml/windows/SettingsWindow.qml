import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

Window {
    id: root

    objectName: "settingsWindow"
    width: 510
    height: 650
    minimumWidth: 460
    minimumHeight: 610
    visible: petInteraction.settingsVisible
    title: qsTr("LingNest AI 设置")
    color: "#F7F2EA"

    function reloadFields() {
        baseUrlInput.text = aiSettings.baseUrl
        modelInput.text = aiSettings.model
        temperatureInput.text = Number(aiSettings.temperature).toFixed(2)
        maxTokensInput.value = aiSettings.maxTokens
        timeoutInput.value = aiSettings.timeoutSeconds
        apiKeyInput.clear()
    }

    onVisibleChanged: {
        if (visible) {
            reloadFields()
            requestActivate()
            baseUrlInput.forceActiveFocus()
        }
    }
    onClosing: {
        close.accepted = false
        petInteraction.closeSettings()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 18
        radius: 14
        color: "#FFFDFC"
        border.color: "#DED3C6"

        Column {
            id: form

            anchors.fill: parent
            anchors.margins: 22
            spacing: 8

            Text {
                width: parent.width
                text: qsTr("OpenAI 兼容服务")
                color: "#3F372F"
                font.pixelSize: 22
                font.bold: true
                font.family: "Microsoft YaHei UI"
            }

            Text {
                width: parent.width
                text: qsTr("支持兼容 Chat Completions 的服务。API Key 仅保存在 Windows 凭据管理器中。")
                color: "#75695E"
                wrapMode: Text.Wrap
                font.pixelSize: 13
                font.family: "Microsoft YaHei UI"
            }

            Label { text: qsTr("Base URL") }
            TextField {
                id: baseUrlInput
                width: parent.width
                placeholderText: "https://api.openai.com/v1"
                selectByMouse: true
            }

            Label { text: qsTr("模型") }
            TextField {
                id: modelInput
                width: parent.width
                placeholderText: qsTr("填写服务支持的模型名称")
                selectByMouse: true
            }

            Row {
                width: parent.width
                spacing: 18

                Column {
                    width: (parent.width - parent.spacing) / 2
                    spacing: 6
                    Label { text: qsTr("Temperature（0–2）") }
                    TextField {
                        id: temperatureInput
                        width: parent.width
                        selectByMouse: true
                        validator: DoubleValidator {
                            bottom: 0.0
                            top: 2.0
                            decimals: 2
                            notation: DoubleValidator.StandardNotation
                        }
                    }
                }

                Column {
                    width: (parent.width - parent.spacing) / 2
                    spacing: 6
                    Label { text: qsTr("Max Tokens") }
                    SpinBox {
                        id: maxTokensInput
                        width: parent.width
                        from: 1
                        to: 1000000
                        editable: true
                    }
                }
            }

            Label { text: qsTr("超时时间（秒）") }
            SpinBox {
                id: timeoutInput
                width: parent.width
                from: 1
                to: 600
                editable: true
            }

            Label {
                text: aiSettings.apiKeyConfigured
                      ? qsTr("API Key（已安全保存；留空表示保持不变）")
                      : qsTr("API Key（尚未配置）")
            }
            TextField {
                id: apiKeyInput
                width: parent.width
                placeholderText: aiSettings.apiKeyConfigured
                                 ? qsTr("••••••••（留空保持原密钥）")
                                 : qsTr("输入 API Key")
                echoMode: TextInput.Password
                selectByMouse: true
            }

            Text {
                width: parent.width
                height: Math.max(42, implicitHeight)
                text: aiSettings.statusMessage
                visible: text.length > 0
                color: aiSettings.statusError ? "#B23A35" : "#4E83CC"
                wrapMode: Text.Wrap
                font.pixelSize: 12
                font.family: "Microsoft YaHei UI"
            }

            Row {
                anchors.right: parent.right
                spacing: 10

                Button {
                    text: qsTr("删除密钥")
                    enabled: aiSettings.apiKeyConfigured
                    onClicked: {
                        aiSettings.clearApiKey()
                        apiKeyInput.clear()
                    }
                }
                Button {
                    text: qsTr("关闭")
                    onClicked: petInteraction.closeSettings()
                }
                Button {
                    text: qsTr("保存")
                    highlighted: true
                    onClicked: {
                        aiSettings.save(
                            baseUrlInput.text,
                            modelInput.text,
                            Number(temperatureInput.text.replace(",", ".")),
                            maxTokensInput.value,
                            timeoutInput.value,
                            apiKeyInput.text)
                        apiKeyInput.clear()
                    }
                }
            }
        }
    }
}
