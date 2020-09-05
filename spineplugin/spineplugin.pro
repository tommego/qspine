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

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) "$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)" "$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

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

RESOURCES += \
    spineresource.qrc
