#include "include.h"

EventLogger::EventLogger(QObject *parent, const QString &filename): BaseLogger(parent, filename), logging_level(Level::Info) {}

EventLogger::~EventLogger(void) {}

const QMap<Level, LevelInfo> EventLogger::Levels = {
    {Level::DebugDetail, {"DTL", "debug detail", Qt::gray}},
    {Level::Debug, {"DBG", "debug", Qt::darkGray}},
    {Level::Info, {"INF", "info", Qt::black}},
    {Level::Warning, {"WAR", "warning", Qt::magenta}},
    {Level::Error, {"ERR", "error", Qt::red}},
    {Level::Fatal, {"FTL", "fatal", Qt::darkRed}},
};

void EventLogger::set_display_widget(QTableWidget *widget) {
    this->m_display = widget;
}

QString EventLogger::format(const QDateTime &timestamp, Level level, const QString &message) const {
    return QString("%1 [%2] %3").arg(timestamp.toString(Qt::ISODate)).arg(EventLogger::Levels[level].code).arg(message);
}

void EventLogger::write(Level level, const QString &message) const {
    if (level > this->logging_level) {
        return;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    QString full = this->format(now, level, message);
    QTextStream out(this->m_file);
    out.setCodec("UTF-8");

    if (this->m_file != nullptr) {
        out << full << Qt::endl;
    }

    if (this->m_display != nullptr) {
        this->m_display->insertRow(this->m_display->rowCount());

        QTableWidgetItem *timestamp = new QTableWidgetItem(now.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        timestamp->setTextAlignment(Qt::AlignCenter);
        this->m_display->setItem(this->m_display->rowCount() - 1, 0, timestamp);

        QTableWidgetItem *level_ = new QTableWidgetItem(EventLogger::Levels[level].name);
        level_->setForeground(EventLogger::Levels[level].colour);
        level_->setTextAlignment(Qt::AlignCenter);
        this->m_display->setItem(this->m_display->rowCount() - 1, 1, level_);

        QTableWidgetItem *text = new QTableWidgetItem(message);
        text->setForeground(EventLogger::Levels[level].colour);
        this->m_display->setItem(this->m_display->rowCount() - 1, 2, text);
    }
}

void EventLogger::set_level(Level new_level) {
    this->logging_level = new_level;
}

void EventLogger::detail(const QString &message) const {
    this->write(Level::DebugDetail, message);
}

void EventLogger::debug(const QString &message) const {
    this->write(Level::Debug, message);
}

void EventLogger::info(const QString &message) const {
    this->write(Level::Info, message);
}

void EventLogger::warning(const QString &message) const {
    this->write(Level::Warning, message);
}

void EventLogger::error(const QString &message) const {
    this->write(Level::Error, message);
}

void EventLogger::fatal(const QString &message) const {
    this->write(Level::Fatal, message);
}
