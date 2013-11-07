load(qt_build_config)
TARGET = CBus

QT = core network websockets
CONFIG += c++11

TEMPLATE = lib

DEFINES += CBUS_LIBRARY

#QMAKE_DOCS = $$PWD/doc/cbus.qdocconfig
OTHER_FILES += doc/src/*.qdoc   # show .qdoc files in Qt Creator
OTHER_FILES += doc/snippets/*.cpp

PUBLIC_HEADERS += \
    $$PWD/cbus.h \
    $$PWD/pendingreply.h \
    $$PWD/utility.h \
    $$PWD/functor.h \
    $$PWD/objectadaptor.h \
    $$PWD/error.h \
    $$PWD/object.h \
    $$PWD/callhelper.h \
    $$PWD/signalwaiter.h \
    $$PWD/global.h \
    $$PWD/objectproxy.h \
    $$PWD/socketioclient.h \
    $$PWD/dynamicqobject.h

#PRIVATE_HEADERS += \
PRIVATE_HEADERS =

SOURCES += \
    $$PWD/cbus.cpp \
    $$PWD/pendingreply.cpp \
    $$PWD/objectadaptor.cpp \
    $$PWD/object.cpp \
    $$PWD/callhelper.cpp \
    $$PWD/signalwaiter.cpp \
    $$PWD/objectproxy.cpp \
    $$PWD/socketioclient.cpp \
    $$PWD/dynamicqobject.cpp

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

load(qt_module)
