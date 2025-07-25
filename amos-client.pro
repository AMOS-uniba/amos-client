QT     += core gui serialport network widgets

CONFIG += c++20
CONFIG += static

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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
    models/qsightingmodel.cpp \
    utils/domestate.cpp \
    utils/exceptions.cpp \
    utils/formatters.cpp \
    utils/qserialbuffer.cpp \
    utils/qserialportmanager.cpp \
    utils/request.cpp \
    utils/sighting.cpp \
    utils/state/serialportstate.cpp \
    utils/state/state.cpp \
    utils/state/stationstate.cpp \
    utils/state/ufostate.cpp \
    utils/telegram.cpp \
    utils/universe.cpp \
    widgets/lines/qbooleanline.cpp \
    widgets/lines/qcontrolline.cpp \
    widgets/lines/qdatetimeline.cpp \
    widgets/lines/qdisplayline.cpp \
    widgets/lines/qfloatline.cpp \
    widgets/qaboutdialog.cpp \
    widgets/qcamera.cpp \
    widgets/qconfigurable.cpp \
    widgets/qdome.cpp \
    widgets/qdomewidget.cpp \
    widgets/qserver.cpp \
    widgets/qsightingbuffer.cpp \
    widgets/qstation.cpp \
    widgets/qsuninfo.cpp \
    widgets/qufomanager.cpp \
    widgets/storage/qfilesystembox.cpp \
    widgets/storage/qscannerbox.cpp \
    widgets/storage/qstoragebox.cpp

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
    logging/include.h \
    logging/loggingdialog.h \
    logging/statelogger.h \
    forward.h \
    mainwindow.h \
    models/qsightingmodel.h \
    utils/domestate.h \
    utils/exceptions.h \
    utils/formatters.h \
    utils/qserialbuffer.h \
    utils/qserialportmanager.h \
    utils/request.h \
    utils/sighting.h \
    utils/state/serialportstate.h \
    utils/state/state.h \
    utils/state/stationstate.h \
    utils/state/ufostate.h \
    utils/telegram.h \
    utils/universe.h \
    widgets/lines/qbooleanline.h \
    widgets/lines/qcontrolline.h \
    widgets/lines/qdatetimeline.h \
    widgets/lines/qdisplayline.h \
    widgets/lines/qfloatline.h \
    widgets/qaboutdialog.h \
    widgets/qcamera.h \
    widgets/qconfigurable.h \
    widgets/qdome.h \
    widgets/qdomewidget.h \
    widgets/qserver.h \
    widgets/qsightingbuffer.h \
    widgets/qstation.h \
    widgets/qsuninfo.h \
    widgets/qufomanager.h \
    widgets/storage/qfilesystembox.h \
    widgets/storage/qscannerbox.h \
    widgets/storage/qstoragebox.h

FORMS += \
    logging/loggingdialog.ui \
    mainwindow.ui \
    widgets/lines/qdisplayline.ui \
    widgets/qaboutdialog.ui \
    widgets/qcamera.ui \
    widgets/qdome.ui \
    widgets/qserver.ui \
    widgets/qsightingbuffer.ui \
    widgets/qstation.ui \
    widgets/qsuninfo.ui \
    widgets/qufomanager.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images.qrc

RC_FILE = amos-client.rc
RC_ICONS = images/blue.ico

QT_FATAL_WARNINGS = 1

VERSION = 1.3.5
DEFINES += VERSION_STRING=\\\"$${VERSION}\\\"
# Magic values: 2020 for new, 2015 for old, otherwise WILL NOT compile!
# This is to ensure that the selection is consistent and you do not get a half-old, half-new monster
DEFINES += PROTOCOL=2020
DEFINES += HEARTBEAT_PROTOCOL_VERSION=2
TARGET = "AMOS client"

QMAKE_TARGET_COMPANY = AMOS
QMAKE_TARGET_PRODUCT = AMOS client
