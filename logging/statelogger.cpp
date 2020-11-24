#include "statelogger.h"

StateLogger::StateLogger(QObject *parent, const QString &filename): BaseLogger(parent, filename) {}

StateLogger::~StateLogger(void) {
    if (this->m_file != nullptr) {
        this->m_file->close();
    }
}

QString StateLogger::format(const QDateTime &timestamp, const QString &message) const {
    return QString("%1 %2").arg(timestamp.toString(Qt::ISODate)).arg(message);
}

void StateLogger::log(const QString &message) const {
    QDateTime now = QDateTime::currentDateTimeUtc();
    QString full = this->format(now, message);
    QTextStream out(this->m_file);
    out.setCodec("UTF-8");

    if (this->m_file != nullptr) {
        out << full << Qt::endl;
    }
}
