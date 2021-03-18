QT      += core gui serialport network widgets uiplugin

CONFIG += c++11
CONFIG += static

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    APC/APC_Cheb.cpp \
    APC/APC_DE.cpp \
    APC/APC_IO.cpp \
    APC/APC_Kepler.cpp \
    APC/APC_Math.cpp \
    APC/APC_Moon.cpp \
    APC/APC_Phys.cpp \
    APC/APC_Planets.cpp \
    APC/APC_PrecNut.cpp \
    APC/APC_Spheric.cpp \
    APC/APC_Sun.cpp \
    APC/APC_Time.cpp \
    APC/APC_VecMat3D.cpp \
    logging/baselogger.cpp \
    logging/eventlogger.cpp \
    logging/loggingdialog.cpp \
    logging/statelogger.cpp \
    main.cpp \
    mainwindow/display.cpp \
    mainwindow/settings.cpp \
    mainwindow/timers.cpp \
    mainwindow/tray.cpp \
    mainwindow.cpp \
    station.cpp \
    utils/domestate.cpp \
    utils/exception.cpp \
    utils/request.cpp \
    utils/serialbuffer.cpp \
    utils/sighting.cpp \
    utils/state/serialportstate.cpp \
    utils/state/state.cpp \
    utils/state/stationstate.cpp \
    utils/telegram.cpp \
    utils/ufomanager.cpp \
    utils/universe.cpp \
    widgets/lines/qbooleanline.cpp \
    widgets/lines/qcontrolline.cpp \
    widgets/lines/qdatetimeline.cpp \
    widgets/lines/qdisplayline.cpp \
    widgets/lines/qfloatline.cpp \
    widgets/qconfigurable.cpp \
    widgets/qdome.cpp \
    widgets/qdomewidget.cpp \
    widgets/qfilesystembox.cpp \
    widgets/qscannerbox.cpp \
    widgets/qserver.cpp \
    widgets/qstoragebox.cpp \
    widgets/qsuninfo.cpp

HEADERS += \
    APC/APC_Cheb.h \
    APC/APC_Const.h \
    APC/APC_DE.h \
    APC/APC_IO.h \
    APC/APC_Kepler.h \
    APC/APC_Math.h \
    APC/APC_Moon.h \
    APC/APC_Phys.h \
    APC/APC_Planets.h \
    APC/APC_PrecNut.h \
    APC/APC_Spheric.h \
    APC/APC_Sun.h \
    APC/APC_Time.h \
    APC/APC_VecMat3D.h \
    logging/baselogger.h \
    logging/eventlogger.h \
    logging/loggingdialog.h \
    logging/statelogger.h \
    forward.h \
    include.h \
    mainwindow.h \
    station.h \
    utils/domestate.h \
    utils/exception.h \
    utils/request.h \
    utils/serialbuffer.h \
    utils/sighting.h \
    utils/state/serialportstate.h \
    utils/state/state.h \
    utils/state/stationstate.h \
    utils/telegram.h \
    utils/ufomanager.h \
    utils/universe.h \
    widgets/lines/qbooleanline.h \
    widgets/lines/qcontrolline.h \
    widgets/lines/qdatetimeline.h \
    widgets/lines/qdisplayline.h \
    widgets/lines/qfloatline.h \
    widgets/qconfigurable.h \
    widgets/qdome.h \
    widgets/qdomewidget.h \
    widgets/qfilesystembox.h \
    widgets/qscannerbox.h \
    widgets/qserver.h \
    widgets/qstoragebox.h \
    widgets/qsuninfo.h

FORMS += \
    logging/loggingdialog.ui \
    mainwindow.ui \
    widgets/lines/qdisplayline.ui \
    widgets/qconfigurable.ui \
    widgets/qdome.ui \
    widgets/qserver.ui \
    widgets/qsuninfo.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images.qrc

RC_FILE = amos-client.rc
