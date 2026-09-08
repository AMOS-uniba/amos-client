#include "statelogger.h"

StateLogger::StateLogger(QObject * parent, const QString & filename):
    BaseLogger(parent, filename) {}

QString StateLogger::format(const QDateTime & timestamp, const QString & message) const {
    return QString("%1 %2").arg(timestamp.toString(Qt::ISODate), message);
}

void StateLogger::log(const QString & message) const {
    QDateTime now = QDateTime::currentDateTimeUtc();
    QString full = this->format(now, message);

    if (this->m_file != nullptr) {
        QTextStream out(this->m_file);
        out << full << Qt::endl;
        out.flush();
        this->rotate_if_oversized();
    }
}
