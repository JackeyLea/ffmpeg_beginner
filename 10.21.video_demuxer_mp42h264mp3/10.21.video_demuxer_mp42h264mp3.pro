TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

include(../ffmpeg.pri)

DESTDIR     = ../bin
TARGET      = 22_demuxer_mp42h264mp3
OBJECTS_DIR = obj
MOC_DIR     = moc
RCC_DIR     = rcc
UI_DIR      = ui

SOURCES += \
    main.cpp
