import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.3

import VideoItem 1.0

Window {
    visible: true
    width: 1280
    height: 720
    title: qsTr("Hello World")

    VideoItem{
        id:videoitem
        anchors.fill: parent
    }

    Button {
        id: button
        x: 29
        y: 27
        text: qsTr("Play")

        onClicked: {
            videoitem.setUrl("C:\\Users\\hyper\\Videos\\Sample.wmv")
            videoitem.start()
        }
    }
}
