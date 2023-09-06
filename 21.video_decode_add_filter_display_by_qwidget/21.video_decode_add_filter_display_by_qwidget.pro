QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(../ffmpeg.pri)

DESTDIR     = ../bin
TARGET      = 21_filter
OBJECTS_DIR = obj
MOC_DIR     = moc
RCC_DIR     = rcc
UI_DIR      = ui

SOURCES += \
    ffmpegwidget.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ffmpegwidget.h \
    mainwindow.h

FORMS += \
    mainwindow.ui
