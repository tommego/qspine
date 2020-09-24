TEMPLATE = lib
TARGET = spineplugin
QT += qml quick concurrent
CONFIG += plugin c++11

TARGET = $$qtLibraryTarget($$TARGET)
uri = Beelab.SpineItem

# Input
SOURCES += \
        rendercmdscache.cpp \
        skeletonrenderer.cpp \
        spineplugin_plugin.cpp \
        spineitem.cpp \
        spinevertexeffect.cpp \
        texture.cpp

HEADERS += \
        rendercmdscache.h \
        skeletonrenderer.h \
        spineplugin_plugin.h \
        spineitem.h \
        spinevertexeffect.h \
        texture.h

DISTFILES = qmldir

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../spine-cpp/release/ -lspine-cpp
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../spine-cpp/debug/ -lspine-cpp
else:unix: LIBS += -L$$OUT_PWD/../spine-cpp/ -lspine-cpp

INCLUDEPATH += $$PWD/../spine-cpp/include

qmldir.files = qmldir
unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}

ARCH = win32

linux-aarch64-gnu-g++:{
    ARCH = linux_aarch64
}

linux-arm-gnueabi-g++:{
    ARCH = linux_armhf
}

linux-g++:{
    ARCH = linux_x86_64
}

DESTDIR = $$PWD/../bin/$$ARCH/Beelab/SpineItem
!equals(_PRO_FILE_PWD_, $$DESTDIR) {
    copy_qmldir.target = $$DESTDIR/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

RESOURCES += \
    spineresource.qrc
