#include "include.h"

Log::Log(QObject *parent, const QString& filename):
    QObject(parent) {

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

QMap<Level, QString> Log::Levels = {
    { Level::Debug, "DBG" },
    { Level::Info, "INF" },
    { Level::Warning, "WAR" },
    { Level::Error, "ERR" },
    { Level::Fatal, "FTL" },
};

QMap<Level, Qt::GlobalColor> Log::Colours = {
    { Level::Debug, Qt::gray },
    { Level::Info, Qt::black },
    { Level::Warning, Qt::yellow },
    { Level::Error, Qt::red },
    { Level::Fatal, Qt::darkRed },
};

void Log::set_display_widget(QListWidget* widget) {
    this->display = widget;
}

QString Log::format(Level level, const QString& message) const {
    return QString("%1 [%2] %3").arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)).arg(Log::Levels.find(level).value()).arg(message);
}

void Log::write(Level level, const QString& message) const {
    QString full = this->format(level, message);
    QTextStream out(this->file);
    out.setCodec("UTF-8");

    if (this->file != nullptr) {
        out << full << Qt::endl;
    }

    if (this->display != nullptr) {
        QListWidgetItem *item = new QListWidgetItem(full);
        item->setForeground(Log::Colours.find(level).value());
        this->display->addItem(item);
    }
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
