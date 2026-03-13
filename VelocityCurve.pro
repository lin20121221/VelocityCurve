QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VelocityCurve
TEMPLATE = app

CONFIG += c++11

# Qt 5.12 compatibility
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    velocitycanvas.cpp \
    midimanager.cpp

HEADERS += \
    mainwindow.h \
    velocitycanvas.h \
    midimanager.h

# Platform-specific MIDI support
win32 {
    LIBS += -lwinmm
}
macx {
    LIBS += -framework CoreMIDI -framework CoreFoundation
}

FORMS +=

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
