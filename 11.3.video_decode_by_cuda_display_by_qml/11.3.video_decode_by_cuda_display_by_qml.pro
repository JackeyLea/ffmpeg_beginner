QT += quick qml quickwidgets multimedia gui opengl

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(../ffmpeg.pri)

DESTDIR     = ../bin
TARGET      = decode_by_cuda_qml
OBJECTS_DIR = obj
MOC_DIR     = moc
RCC_DIR     = rcc
UI_DIR      = ui

HEADERS += \
    ffmpegdecoder.h \
    nv12render.h \
    videoitem.h


SOURCES += \
        ffmpegdecoder.cpp \
        main.cpp \
        nv12render.cpp \
        videoitem.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =
