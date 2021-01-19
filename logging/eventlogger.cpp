#include "include.h"

EventLogger::EventLogger(QObject *parent, const QString &filename):
    BaseLogger(parent, filename),
    logging_level(Level::Info) {}

const QMap<Level, LevelInfo> EventLogger::Levels = {
    {Level::DebugDetail, {"DTL", "debug detail", Qt::gray}},
    {Level::Debug, {"DBG", "debug", Qt::darkGray}},
    {Level::DebugError, {"DBE", "debug error", Qt::red}},
    {Level::Info, {"INF", "info", Qt::black}},
    {Level::Warning, {"WAR", "warning", Qt::magenta}},
    {Level::Error, {"ERR", "error", Qt::red}},
    {Level::Fatal, {"FTL", "fatal", Qt::darkRed}},
};

const QMap<Concern, ConcernInfo> EventLogger::Concerns = {
    {Concern::Automatic, {"AUT", "auto"}},
    {Concern::Configuration, {"CFG", "config"}},
    {Concern::Generic, {"---", "-"}},
    {Concern::Heartbeat, {"HBT", "heartbeat"}},
    {Concern::SerialPort, {"SRP", "serial"}},
    {Concern::Server, {"SRV", "server"}},
    {Concern::Sightings, {"SGH", "sightings"}},
    {Concern::UFO, {"UFO", "UFO"}},
    {Concern::Operation, {"OPE", "operation"}},
    {Concern::Storage, {"STO", "storage"}}
};

void EventLogger::set_display_widget(QTableWidget *widget) {
    this->m_display = widget;
}

QString EventLogger::format(const QDateTime &timestamp, Level level, const QString &concern, const QString &message) const {
    return QString("%1 %2 | %3: %4")
            .arg(timestamp.toString(Qt::ISODate))
            .arg(EventLogger::Levels[level].code)
            .arg(concern)
            .arg(message);
}

void EventLogger::write(Level level, Concern concern, const QString &message) const {
    if (level > this->logging_level) {
        return;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    QString full = this->format(now, level, EventLogger::Concerns[concern].code, message);
    QTextStream out(this->m_file);
    out.setCodec("UTF-8");

    if (this->m_file != nullptr) {
        out << full << Qt::endl;
    }

    if (this->m_display != nullptr) {
        this->m_display->insertRow(this->m_display->rowCount());

        QTableWidgetItem *item_timestamp = new QTableWidgetItem(now.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        item_timestamp->setTextAlignment(Qt::AlignCenter);
        this->m_display->setItem(this->m_display->rowCount() - 1, 0, item_timestamp);

        QTableWidgetItem *item_level = new QTableWidgetItem(EventLogger::Levels[level].name);
        item_level->setForeground(EventLogger::Levels[level].colour);
        item_level->setTextAlignment(Qt::AlignCenter);
        this->m_display->setItem(this->m_display->rowCount() - 1, 1, item_level);

        QTableWidgetItem *item_concern = new QTableWidgetItem(EventLogger::Concerns[concern].name);
        item_concern->setForeground(EventLogger::Levels[level].colour);
        item_concern->setTextAlignment(Qt::AlignCenter);
        this->m_display->setItem(this->m_display->rowCount() - 1, 2, item_concern);

        QTableWidgetItem *item_message = new QTableWidgetItem(message);
        item_message->setForeground(EventLogger::Levels[level].colour);
        this->m_display->setItem(this->m_display->rowCount() - 1, 3, item_message);

        if (this->m_display->rowCount() > 256) {
            this->m_display->removeRow(0);
        }
    }
}

void EventLogger::set_level(Level new_level) {
    this->logging_level = new_level;
}

void EventLogger::detail(Concern concern, const QString &message) const { this->write(Level::DebugDetail, concern, message); }
void EventLogger::debug_error(Concern concern, const QString &message) const { this->write(Level::DebugError, concern, message); }
void EventLogger::debug(Concern concern, const QString &message) const { this->write(Level::Debug, concern, message); }
void EventLogger::info(Concern concern, const QString &message) const { this->write(Level::Info, concern, message); }
void EventLogger::warning(Concern concern, const QString &message) const { this->write(Level::Warning, concern, message); }
void EventLogger::error(Concern concern, const QString &message) const { this->write(Level::Error, concern, message); }
void EventLogger::fatal(Concern concern, const QString &message) const { this->write(Level::Fatal, concern, message); }

