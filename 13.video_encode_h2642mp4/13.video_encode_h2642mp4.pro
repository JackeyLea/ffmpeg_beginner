TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

include(../ffmpeg.pri)

DESTDIR     = ../bin
TARGET      = 13_encode_h2642mp4
OBJECTS_DIR = obj
MOC_DIR     = moc
RCC_DIR     = rcc
UI_DIR      = ui

SOURCES += \
        main.c
