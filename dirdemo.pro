#-------------------------------------------------
#
# Project created by QtCreator 2018-10-21T16:36:58
#
#-------------------------------------------------

QT       += core gui

#QT += qml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dirdemo
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix:DEFINES += LINUX


win32:DEFINES += WINDOWS


debug {
    DEFINES += DEBUG
}

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    treewidget.cpp \
    filedigger.cpp \
    statusbar.cpp


HEADERS += \
        mainwindow.h \
    treewidget.h \
    filedigger.h \
    statusbar.h

FORMS += \
        mainwindow.ui

LIBS += -L -lchilkat-9.5.0 -lpthread

QMAKE_CXXFLAGS += -std=c++17
