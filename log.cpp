#include "include.h"

Log::Log(QObject *parent, const QString& filename):
    QObject(parent), logging_level(Level::Info) {

    if (!filename.isEmpty()) {
        this->file = new QFile();
        this->file->setFileName(filename);
        this->file->open(QIODevice::Append | QIODevice::Text);
    }
}

Log::~Log(void) {
    if (this->file != nullptr) {
        this->file->close();
    }
}

const QMap<Level, LevelInfo> Log::Levels = {
    {Level::DebugDetail, {"DTL", "debug detail", Qt::gray}},
    {Level::Debug, {"DBG", "debug", Qt::darkGray}},
    {Level::Info, {"INF", "info", Qt::black}},
    {Level::Warning, {"WAR", "warning", Qt::darkYellow}},
    {Level::Error, {"ERR", "error", Qt::red}},
    {Level::Fatal, {"FTL", "fatal", Qt::darkRed}},
};

void Log::set_display_widget(QTableWidget* widget) {
    this->display = widget;
}

QString Log::format(const QDateTime& timestamp, Level level, const QString& message) const {
    return QString("%1 [%2] %3").arg(timestamp.toString(Qt::ISODate)).arg(Log::Levels[level].code).arg(message);
}

void Log::write(Level level, const QString& message) const {
    if (level > this->logging_level) {
        return;
    }

    QDateTime now = QDateTime::currentDateTimeUtc();
    QString full = this->format(now, level, message);
    QTextStream out(this->file);
    out.setCodec("UTF-8");

    if (this->file != nullptr) {
        out << full << Qt::endl;
    }

    if (this->display != nullptr) {
        this->display->insertRow(0);

        QTableWidgetItem *timestamp = new QTableWidgetItem(now.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        timestamp->setTextAlignment(Qt::AlignCenter);
        this->display->setItem(0, 0, timestamp);

        QTableWidgetItem *level_ = new QTableWidgetItem(Log::Levels[level].name);
        level_->setForeground(Log::Levels[level].colour);
        level_->setTextAlignment(Qt::AlignCenter);
        this->display->setItem(0, 1, level_);

        QTableWidgetItem *text = new QTableWidgetItem(message);
        text->setForeground(Log::Levels[level].colour);
        this->display->setItem(0, 2, text);
    }
}

void Log::set_level(Level new_level) {
    this->logging_level = new_level;
}

void Log::detail(const QString& message) const {
    this->write(Level::DebugDetail, message);
}

void Log::debug(const QString& message) const {
    this->write(Level::Debug, message);
}

void Log::info(const QString& message) const {
    this->write(Level::Info, message);
}

void Log::warning(const QString& message) const {
    this->write(Level::Warning, message);
}

void Log::error(const QString& message) const {
    this->write(Level::Error, message);
}

void Log::fatal(const QString& message) const {
    this->write(Level::Fatal, message);
}
