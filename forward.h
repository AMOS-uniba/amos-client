#ifndef FORWARD_H
#define FORWARD_H

#include <QtCore>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(MainWindow);

QT_FORWARD_DECLARE_CLASS(Universe);
QT_FORWARD_DECLARE_CLASS(Request);
QT_FORWARD_DECLARE_CLASS(Command);
QT_FORWARD_DECLARE_CLASS(CommThread);

QT_FORWARD_DECLARE_CLASS(DomeStateS);
QT_FORWARD_DECLARE_CLASS(DomeStateT);
QT_FORWARD_DECLARE_CLASS(DomeStateZ);

QT_FORWARD_DECLARE_CLASS(BaseLogger);
QT_FORWARD_DECLARE_CLASS(EventLogger);
QT_FORWARD_DECLARE_CLASS(StateLogger);

QT_FORWARD_DECLARE_CLASS(QUfoManager);
QT_FORWARD_DECLARE_CLASS(QStation);
QT_FORWARD_DECLARE_CLASS(QDome);
QT_FORWARD_DECLARE_CLASS(QServer);
QT_FORWARD_DECLARE_CLASS(QFileSystemBox);
QT_FORWARD_DECLARE_CLASS(QScannerBox);
QT_FORWARD_DECLARE_CLASS(QStorageBox);

#include "utils/state/state.h"
#include "utils/state/stationstate.h"
#include "utils/state/serialportstate.h"
#include "utils/domestate.h"
#include "utils/serialbuffer.h"
#include "utils/sighting.h"

#endif // FORWARD_H
