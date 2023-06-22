QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 包含FFmpeg库文件位置

# windows平台
win32{
INCLUDEPATH += C:\\ffmpeg\\include
LIBS += -LC:/ffmpeg/lib \
        -lavcodec \
        -lavdevice \
        -lavformat \
        -lavutil   \
        -lpostproc \
        -lswresample \
        -lswscale
}

# linux平台
unix{
    INCLUDEPATH+= .
    LIBS += -L/usr/lib/x86_64-linux-gnu/ \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}


SOURCES += \
    ffmpegwidget.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ffmpegwidget.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
