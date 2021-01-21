#include "include.h"


extern EventLogger logger;

FileSystemManager::FileSystemManager(const QDir &directory) {
    this->set_directory(directory);
}

QStorageInfo FileSystemManager::info(void) const {
    return QStorageInfo(this->m_directory);
}

QDir FileSystemManager::directory(void) const { return this->m_directory; }

void FileSystemManager::set_directory(const QDir &dir) {
    this->m_directory = dir;

    emit this->directory_set(dir.path());
}

void FileSystemManager::open_in_explorer(void) const {
    QDesktopServices::openUrl(QUrl::fromLocalFile(this->directory().path()));
}
