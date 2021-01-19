#ifndef FORWARD_H
#define FORWARD_H

#include <QtCore>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(MainWindow);
QT_FORWARD_DECLARE_CLASS(Station);


QT_FORWARD_DECLARE_CLASS(Server);
QT_FORWARD_DECLARE_CLASS(Dome);
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

QT_FORWARD_DECLARE_CLASS(FileSystemManager);
QT_FORWARD_DECLARE_CLASS(Storage);
QT_FORWARD_DECLARE_CLASS(FileSystemScanner);

QT_FORWARD_DECLARE_CLASS(UfoManager);

#include "utils/stationstate.h"
#include "utils/domestate.h"
#include "utils/serialbuffer.h"
#include "utils/sighting.h"

#endif // FORWARD_H
