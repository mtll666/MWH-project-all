QT       += core gui opengl openglwidgets multimedia multimediawidgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 console # 确保调试日志可见

# FFmpeg 配置
INCLUDEPATH += C:/ffmpeg/include
LIBS += -LC:/ffmpeg/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lopengl32

SOURCES += \
    Widget/VideoFrameDisplay.cpp \
    Widget/videowidget.cpp \
    Widget/VideoSlider.cpp \
    audioimage.cpp \
    controller.cpp \
    getaudio.cpp \
    main.cpp \
    mainwindow.cpp \
    reversedecode.cpp \
    reversedisplay.cpp \
    seekFrame.cpp \
    utils.cpp

HEADERS += \
    Widget/VideoFrameDisplay.h \
    Widget/videowidget.h \
    Widget/VideoSlider.h \
    audioimage.h \
    controller.h \
    getaudio.h \
    mainwindow.h \
    reversedecode.h \
    reversedisplay.h \
    seekFrame.h \
    utils.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    ImageResource.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
