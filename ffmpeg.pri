
# 包含FFmpeg库文件位置

DEFINES += FFMPEG4

QMAKE_CXXFLAGS += -Werror#警告作为错误

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

if(contains(DEFINES,FFMPEG6)){
    #使用6.0版
    INCLUDEPATH+= . $$PWD/lib/6.0/include/ /usr/include/mfx/
    LIBS += -L$$PWD/lib/6.0/lib \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}
if(contains(DEFINES,FFMPEG5)){
    #使用5.1.3版
    INCLUDEPATH+= . $$PWD/lib/5.1.3/include/
    LIBS += -L$$PWD/lib/5.1.3/lib \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}
if(contains(DEFINES,FFMPEG4)){
    #使用4.4.4版
    INCLUDEPATH+= . $$PWD/lib/4.4.4/include/
    LIBS += -L$$PWD/lib/4.4.4/include/ \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}

if(contains(DEFINES,SYSTEM)){
    #使用系统库
    INCLUDEPATH+= . /usr/include/x86_64-linux-gnu/
    LIBS += -L/usr/lib/x86_64-linux-gnu/ \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}
}
