import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Dialogs.qml 1.0
import QtQuick.Extras 1.4
import QtMultimedia 5.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import VideoItem 1.0

Window {
    objectName: "mainWindow"
    width: 640
    height: 480
    visible: true
    title: qsTr("Mpplayer")

    property string url: textField.text //地址变量

    Item{
        id:root
        anchors.fill: parent
        VideoItem{//视频模块
            id: videoitem
            anchors.fill: parent
        }

        Label {
            id: label
            x: 24
            y: 372
            width: 51
            height: 31
            text: qsTr("Url")
            transformOrigin: Item.Center
        }

        TextField {
            id: textField
            x: 81
            y: 363
            width: 507
            height: 40
            text: qsTr("rtsp://192.168.1.217/stream0")
        }

        Button {
            id: button
            x: 24
            y: 416
            text: qsTr("Play1")

            onClicked: {
                videoitem.setUrl(url)
                console.log(url)
                videoitem.start()
            }
        }

        Button {
            id: button1
            x: 139
            y: 416
            text: qsTr("Play2")
            onClicked: {
                videoitem.setUrl(url)
                console.log(url)
                videoitem.start()
            }
        }

        Button {
            id: button2
            x: 258
            y: 416
            text: qsTr("Play3")
            onClicked: {
                videoitem.setUrl(url)
                console.log(url)
                videoitem.start()
            }
        }

        Button {
            id: button3
            x: 390
            y: 416
            text: qsTr("Play4")
            onClicked: {
                videoitem.setUrl(url)
                console.log(url)
                videoitem.start()
            }
        }

        Button {
            id: button4
            x: 515
            y: 416
            text: qsTr("Stop")
            onClicked: {
                videoitem.stop();
            }
        }
    }
}

/*##^## Designer {
    D{i:7;anchors_height:31;anchors_width:51;anchors_x:24;anchors_y:372}
}
 ##^##*/
