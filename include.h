//#define OLD_PROTOCOL 1

#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdexcept>

#include "utils/request.h"
#include "utils/universe.h"
#include "utils/telegram.h"
#include "utils/exception.h"
#include "utils/state/state.h"
#include "utils/state/stationstate.h"
#include "utils/state/serialportstate.h"
#include "utils/domestate.h"
#include "utils/sighting.h"
#include "utils/ufomanager.h"

#include "amos-widgets/domewidget.h"

#include "widgets/qfilesystembox.h"
#include "widgets/qscannerbox.h"
#include "widgets/qstoragebox.h"

#include "mainwindow.h"
#include "server.h"
#include "station.h"
#include "logging/eventlogger.h"
#include "logging/statelogger.h"
#include "dome.h"

#include <QString>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QDateTime>

#endif // INCLUDE_H
