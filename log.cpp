#include "log.h"

Log::Log(QObject *parent, const QString& filename, QListWidget *_display):
    QObject(parent), display(_display) {

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
    { Level::Critical, "CRI" },
};

QString Log::format(Level level, const QString& message) const {
    return QString("[%1 %2] %3").arg(QDateTime::currentDateTimeUtc().toString()).arg(Log::Levels.find(level).value()).arg(message);
}

void Log::write_to_file(Level level, const QString& message) const {
    QString full = this->format(level, message);
    QTextStream out(this->file);
    out.setCodec("UTF-8");

    if (this->file != nullptr) {
        out << full;
    }
}

void Log::info(const QString& message) const {
    this->write_to_file(Level::Info, message);
}
