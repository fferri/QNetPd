import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OSC

Window {
    id: root
    width: 640
    height: 480
    visible: true
    title: qsTr("QNetPd v0.0.1")
    color: activeSystemPalette.window

    property int socketNumber: -1

    property alias state: rootItem.state

    Item {
        id: rootItem
        anchors.fill: parent
        anchors.margins: 5

        ColumnLayout {
            anchors.fill: parent
            Layout.margins: 5

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter

                Label { text: "Nickname:" }
                TextField {
                    id: nicknameField
                    enabled: root.state === ''
                    selectByMouse: true
                    text: "<anon>"
                }
                Label { text: "Host:" }
                TextField {
                    id: hostField
                    Layout.fillWidth: true
                    enabled: root.state === ''
                    selectByMouse: true
                    text: "netpd.org"
                }
                Label { text: "Port:" }
                TextField {
                    id: portField
                    enabled: root.state === ''
                    selectByMouse: true
                    text: "3025"
                    readonly property int intValue: parseInt(text)
                }
                Button {
                    id: connectButton
                    text: "Connect"
                    onClicked: root.connectClicked()
                }
            }

            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: ListModel {id: listModel}
                delegate: Rectangle {
                    width: listView.width
                    implicitHeight: messageCell.implicitHeight
                    color: Qt.rgba(0, 0, 0, index % 2 ? 0.02 : 0)

                    Row {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            id: timeCell
                            width: 50
                            text: time
                            color: '#ccc'
                        }

                        Text {
                            id: nicknameCell
                            width: 100
                            text: nickname
                            font.bold: true
                            horizontalAlignment: Qt.AlignRight
                        }

                        Text {
                            id: messageCell
                            width: listView.width - 20 - timeCell.width - nicknameCell.width
                            text: message
                            onLinkActivated: (link) => Qt.openUrlExternally(link)
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter

                Label {
                    id: connectedUsersCountLabel
                }
                TextField {
                    id: messageField
                    Layout.fillWidth: true
                    enabled: root.state === 'Connected'
                    selectByMouse: true
                    Keys.onReturnPressed: sendButton.clicked()
                }
                Button {
                    id: sendButton
                    text: "Send"
                    enabled: false
                    onClicked: root.sendClicked()
                }
            }
        }

        states: [
            State {
                name: "LookingUpHost"
                when: osc.state === OSC.HostLookupState
                PropertyChanges {
                    target: connectButton
                    enabled: false
                    text: "Looking up host..."
                }
            },
            State {
                name: "Connecting"
                when: osc.state === OSC.ConnectingState
                PropertyChanges {
                    target: connectButton
                    enabled: false
                    text: "Connecting..."
                }
            },
            State {
                name: "Disconnecting"
                when: osc.state === OSC.ClosingState
                PropertyChanges {
                    target: connectButton
                    enabled: false
                    text: "Disonnecting..."
                }
            },
            State {
                name: "Connected"
                when: osc.state === OSC.ConnectedState
                PropertyChanges {
                    target: connectButton
                    text: "Disconnect"
                }
                PropertyChanges {
                    target: sendButton
                    enabled: true
                }
            }
        ]
    }

    SystemPalette {
        id: activeSystemPalette
    }

    OSC {
        id: osc
        host: hostField.text
        port: portField.intValue

        onMessage: (addr, args) => root.onMessage(addr, args)
        onStateChanged: {
            if(state === OSC.ConnectedState)
                root.onConnected()
            else if(state === OSC.UnconnectedState)
                root.onDisconnected()
        }
    }

    Timer {
        id: keepAliveTimer
        interval: 30000
        running: osc.state === OSC.ConnectedState
        repeat: true
        onTriggered: root.sendMessage('/s/dummy', [])
    }

    function escapeHTML(txt) {
        return txt
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#039;")
    }

    function linkify(txt) {
        // URLs starting with http://, https://, or ftp://
        var p1 = /(\b(https?|ftp):\/\/[-A-Z0-9+&@#\/%?=~_|!:,.;]*[-A-Z0-9+&@#\/%=~_|])/gim
        txt = txt.replace(p1, '<a href="$1" target="_blank">$1</a>')

        // URLs starting with "www." (without // before it, or it'd re-link the ones done above).
        var p2 = /(^|[^\/])(www\.[\S]+(\b|$))/gim
        txt = txt.replace(p2, '$1<a href="http://$2" target="_blank">$2</a>')

        // Change email addresses to mailto:: links.
        var p3 = /(([a-zA-Z0-9\-\_\.])+@[a-zA-Z\_]+?(\.[a-zA-Z]{2,6})+)/gim
        txt = txt.replace(p3, '<a href="mailto:$1">$1</a>')

        return txt
    }

    function appendTextLine(message, time, nickname) {
        message = escapeHTML(message)
        message = linkify(message)
        nickname = escapeHTML(nickname)
        listModel.append({time, nickname, message})
        listView.positionViewAtEnd()
    }

    function connectClicked() {
        if(state === "") osc.connect()
        else osc.disconnect()
    }

    function sendClicked() {
        if(messageField.text === "") return

        if(messageField.text[0] === '/') {
            // process command:
            if(messageField.text === '/list') {
                sendMessage(`/b/chat/listrequest`, [root.socketNumber])
            } else {
                appendTextLine(`Unrecognized command: ${messageField.text}`)
            }
        } else {
            sendMessage("/b/chat/msg", [nicknameField.text, messageField.text, root.socketNumber])
        }

        messageField.text = ""
    }

    function onConnected() {
        helloPending = true
        sendMessage("/s/server/socket", [])
    }

    function onDisconnected() {
        connectedUsersCountLabel.text = ''
        root.socketNumber = -1
    }

    property bool helloPending: false

    onSocketNumberChanged: {
        if(root.socketNumber > 0 && helloPending) {
            helloPending = false
            sendMessage("/b/chat/msg", [nicknameField.text, '<<<<<<<<<<<<<connected<<<<<<<<<<<<<<<<<', root.socketNumber])
        }
    }

    function sendMessage(addr, args) {
        console.log('Sending:', addr, args)

        osc.sendMessage(addr, args)
    }

    function onMessage(addr, args) {
        console.log('Received:', addr, args)

        var addrv = addr.split('/').splice(1)
        var addr0 = addrv.shift()
        if(addrv[0] === 'server') {
            if(addrv[1] === 'socket') {
                root.socketNumber = args[0]
            }
            if(addrv[1] === 'num_of_clients') {
                connectedUsersCountLabel.text = args[0]
            }
        }
        if(addrv[0] === 'chat') {
            if(addrv[1] === 'msg') {
                appendTextLine(args[1], Qt.formatTime(new Date(), 'h:mm:ss'), args[0])
            }
            if(addrv[1] === 'listrequest') {
                sendMessage(`/${args[0]}/chat/msg`, [nicknameField.text, `▬▬▬ client-ID: ${root.socketNumber} / ${root.title}`, root.socketNumber])
            }
        }
    }
}
