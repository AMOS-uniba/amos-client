#include "logging/eventlogger.h"

extern QSettings * settings;

EventLogger::EventLogger(QObject *parent, const QString &filename):
    BaseLogger(parent, filename),
    logging_level(Level::Info),
    debug_visible({
        {Concern::Automatic,        true},
        {Concern::Configuration,    true},
        {Concern::Heartbeat,        true},
        {Concern::SerialPort,       true},
        {Concern::Server,           true},
        {Concern::Sightings,        true},
        {Concern::UFO,              true},
        {Concern::Operation,        true},
        {Concern::Storage,          true},
    })
{}

const QMap<Level, LevelInfo> EventLogger::Levels = {
    {Level::DebugDetail,    {"DTL", "debug detail", Qt::gray}},
    {Level::Debug,          {"DBG", "debug",        Qt::darkGray}},
    {Level::DebugError,     {"DBE", "debug error",  Qt::red}},
    {Level::Info,           {"INF", "info",         Qt::black}},
    {Level::Warning,        {"WAR", "warning",      Qt::blue}},
    {Level::Error,          {"ERR", "error",        Qt::red}},
    {Level::Fatal,          {"FTL", "fatal",        Qt::darkRed}},
};

const QMap<Concern, ConcernInfo> EventLogger::Concerns = {
    {Concern::Automatic,        {"AUT", "auto",         "auto",             "Automatic actions"}},
    {Concern::Configuration,    {"CFG", "config",       "configuration",    "Configuration"}},
    {Concern::Heartbeat,        {"HBT", "heartbeat",    "heartbeats",       "Heartbeats"}},
    {Concern::SerialPort,       {"SRP", "serial",       "serial port",      "Serial port communiation"}},
    {Concern::Server,           {"SRV", "server",       "server",           "Server connection"}},
    {Concern::Sightings,        {"SGH", "sightings",    "sightings",        "Sightings"}},
    {Concern::UFO,              {"UFO", "ufo",          "UFO",              "UFO management"}},
    {Concern::Operation,        {"OPE", "operation",    "operation",        "Operation"}},
    {Concern::Storage,          {"STO", "storage",      "storage",          "Storage management"}}
};

void EventLogger::set_display_widget(QTableWidget * widget) {
    this->m_display = widget;
}

QString EventLogger::format(const QDateTime & timestamp, Level level, const QString & concern, const QString & message) const {
    return QString("%1 %2|%3: %4")
        .arg(timestamp.toString(Qt::ISODate), EventLogger::Levels[level].code, concern, message);
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

        QTableWidgetItem *item_concern = new QTableWidgetItem(EventLogger::Concerns[concern].full_name);
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

void EventLogger::fatal(Concern concern, const QString & message) const { this->write(Level::Fatal, concern, message); }
void EventLogger::error(Concern concern, const QString & message) const { this->write(Level::Error, concern, message); }
void EventLogger::warning(Concern concern, const QString & message) const { this->write(Level::Warning, concern, message); }
void EventLogger::info(Concern concern, const QString & message) const { this->write(Level::Info, concern, message); }

void EventLogger::debug(Concern concern, const QString & message) const {
    if (this->debug_visible[concern]) {
        this->write(Level::Debug, concern, message);
    }
}

void EventLogger::debug_error(Concern concern, const QString & message) const {
    if (this->debug_visible[concern]) {
        this->write(Level::DebugError, concern, message);
    }
}

void EventLogger::detail(Concern concern, const QString & message) const {
    if (this->debug_visible[concern]) {
        this->write(Level::DebugDetail, concern, message);
    }
}

void EventLogger::set_debug_visible(Concern concern, bool visible) {
    this->debug_visible[concern] = visible;
}

bool EventLogger::is_debug_visible(Concern concern) const {
    return (this->debug_visible[concern]);
}

void EventLogger::load_settings(void) {
    for (auto concern = this->debug_visible.cbegin(); concern != this->debug_visible.cend(); ++concern) {
        this->set_debug_visible(
            concern.key(),
            settings->value("logging/" + EventLogger::Concerns[concern.key()].name, false).toBool()
        );
    }
}

void EventLogger::save_settings(void) const {
    for (auto concern = this->debug_visible.cbegin(); concern != this->debug_visible.cend(); ++concern) {
        settings->setValue("logging/" + EventLogger::Concerns[concern.key()].name, concern.value());
    }
}
