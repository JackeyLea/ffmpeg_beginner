TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

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
    LIBS += -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}


SOURCES += \
    main.cpp
