QT += core multimedia testlib

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle

include(../ffmpeg.pri)

DESTDIR     = ../bin
TARGET      = audio_decode
OBJECTS_DIR = obj
MOC_DIR     = moc
RCC_DIR     = rcc
UI_DIR      = ui

SOURCES += \
        main.cpp
