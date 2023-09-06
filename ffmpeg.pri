
# 包含FFmpeg库文件位置

DEFINES += FFMPEG4

# windows平台 VS2019
win32{
DEFINES +=_CRT_SECURE_NO_WARNINGS #非安全函数警告
QMAKE_CXXFLAGS += /WX#警告作为错误
if(contains(DEFINES,FFMPEG6)){
    #使用6.0版
    INCLUDEPATH+= . $$PWD/lib/6.x/include/
    LIBS += -L$$PWD/lib/6.x/lib \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}
if(contains(DEFINES,FFMPEG5)){
    #使用5.1.3版
    INCLUDEPATH+= . $$PWD/lib/5.x/include/
    LIBS += -L$$PWD/lib/5.x/lib \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}
if(contains(DEFINES,FFMPEG4)){
    #使用4.4.4版
    INCLUDEPATH+= . $$PWD/lib/4.x/include/
    LIBS += -L$$PWD/lib/4.x/lib/ \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}

if(contains(DEFINES,SYSTEM)){
    #使用系统库
    message("no system lib on windows,please use other conf instead.")
}
}

# linux平台 GCC
unix{
QMAKE_CXXFLAGS += -Werror#警告作为错误
if(contains(DEFINES,FFMPEG6)){
    #使用6.0版
    INCLUDEPATH+= . $$PWD/lib/6.x/include/ /usr/include/mfx/
    LIBS += -L$$PWD/lib/6.x/lib \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}
if(contains(DEFINES,FFMPEG5)){
    #使用5.1.3版
    INCLUDEPATH+= . $$PWD/lib/5.x/include/
    LIBS += -L$$PWD/lib/5.x/lib \
            -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc \
            -lswresample -lswscale
}
if(contains(DEFINES,FFMPEG4)){
    #使用4.4.4版
    INCLUDEPATH+= . $$PWD/lib/4.x/include/
    LIBS += -L$$PWD/lib/4.x/lib/ \
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
