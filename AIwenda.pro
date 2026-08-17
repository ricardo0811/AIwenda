QT += widgets network

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    chatclient.cpp \
    apiconfig.cpp \
    tcpchatserver.cpp \
    tcpchatclient.cpp \
    translationservice.cpp \
    sentimentanalyzer.cpp \
    replysuggester.cpp \
    messagemodel.cpp

HEADERS += \
    mainwindow.h \
    chatclient.h \
    apiconfig.h \
    tcpchatserver.h \
    tcpchatclient.h \
    translationservice.h \
    sentimentanalyzer.h \
    replysuggester.h \
    messagemodel.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
