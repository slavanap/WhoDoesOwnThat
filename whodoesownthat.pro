#-------------------------------------------------
#
# Project created by QtCreator 2015-02-19T10:39:29
#
#-------------------------------------------------

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = whodoesownthat
TEMPLATE = app

win32 {
    LIBS += -L"C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib"
    INCLUDEPATH += .. "C:/Program Files/Microsoft SDKs/Windows/v6.0A/include"
    RC_FILE += "whodoesownthat.rc"
    DEFINES += WINDOWS

    LIBS += -lAdvapi32 -lShlwapi -lMpr
}


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    getowner.cpp \

HEADERS += \
    mainwindow.h \

FORMS += \
    mainwindow.ui
