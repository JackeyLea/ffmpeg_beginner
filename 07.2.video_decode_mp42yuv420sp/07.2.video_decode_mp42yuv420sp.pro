TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

include(../ffmpeg.pri)

DESTDIR     = ../bin
TARGET      = 7_2_decode_mp42yuv420sp
OBJECTS_DIR = obj
MOC_DIR     = moc
RCC_DIR     = rcc
UI_DIR      = ui

SOURCES += \
        main.c
