QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    UniversalFunction.cpp \
    filerecvdialog.cpp \
    filesavedialog.cpp \
    filetransferwidget.cpp \
    friendwidget.cpp \
    main.cpp \
    messagewidget.cpp \
    onlineusr.cpp \
    opewidget.cpp \
    protocol.cpp \
    searchdialog.cpp \
    sharefilewidget.cpp \
    tcpclient.cpp \
    webdiskwiget.cpp

HEADERS += \
    UniversalFunction.h \
    UniversalFunction.h \
    filerecvdialog.h \
    filesavedialog.h \
    filetransferwidget.h \
    friendwidget.h \
    messagewidget.h \
    onlineusr.h \
    opewidget.h \
    protocol.h \
    searchdialog.h \
    sharefilewidget.h \
    tcpclient.h \
    webdiskwiget.h

FORMS += \
    filerecvdialog.ui \
    filesavedialog.ui \
    filetransferwidget.ui \
    onlineusr.ui \
    tcpclient.ui \
    webdiskwiget.ui

RC_ICONS +=DXDisk.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Recourse.qrc \
    config.qrc
