#include "qstoragebox.h"

extern EventLogger logger;
extern QSettings *settings;

QFileSystemBox::QFileSystemBox(QWidget *parent):
    QGroupBox(parent),
    m_enabled(true)
{
    this->m_timer = new QTimer(this);
    this->m_timer->setInterval(2000);
    this->connect(this->m_timer, &QTimer::timeout, this, &QFileSystemBox::scan_info);
    this->m_timer->start();

    this->m_layout = new QGridLayout(this);

    this->m_cb_enabled = new QCheckBox(this);

    this->m_bt_change = new QPushButton("Change", this);
    this->m_bt_change->setMaximumWidth(50);
    this->m_bt_open = new QPushButton("Open", this);
    this->m_bt_open->setMaximumWidth(50);
    this->m_le_path = new QLineEdit("undefined", this);

    this->m_pb_capacity = new QProgressBar(this);
    this->m_pb_capacity->setFormat("%v/%m GB");
    this->m_pb_capacity->setStyleSheet(
        "QProgressBar { border: 1px solid black; border-radius: 0px; text-align: center; } \
            QProgressBar::chunk {background-color: #2acd26; width: 1px; }"
    );

    this->m_layout->setHorizontalSpacing(3);
    this->m_layout->addWidget(this->m_cb_enabled, 0, 0, 2, 1);
    this->m_layout->addWidget(this->m_le_path, 0, 1, 1, 3);
    this->m_layout->addWidget(this->m_bt_change, 1, 1, 1, 1);
    this->m_layout->addWidget(this->m_bt_open, 1, 2, 1, 1);
    this->m_layout->addWidget(this->m_pb_capacity, 1, 3, 1, 1);

    this->connect(this->m_bt_change, &QPushButton::clicked, this, &QFileSystemBox::select_directory);
    this->connect(this->m_bt_open, &QPushButton::clicked, this, &QFileSystemBox::open_in_explorer);
    this->connect(this->m_cb_enabled, &QCheckBox::clicked, this, &QFileSystemBox::set_enabled);

    this->scan_info();
}

QStorageInfo QFileSystemBox::info(void) const {
    return QStorageInfo(this->m_directory);
}

bool QFileSystemBox::is_enabled(void) const {
    return this->m_enabled;
}

void QFileSystemBox::set_enabled(bool enabled) {
    this->m_enabled = enabled;
    this->m_le_path->setEnabled(enabled);
    this->m_pb_capacity->setEnabled(enabled);
    this->m_cb_enabled->setCheckState(enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    if (enabled != this->m_enabled) {
        emit this->toggled(this->m_enabled);
    }
}

void QFileSystemBox::scan_info(void) {
    auto info = this->info();
    double total = (double) info.bytesTotal() / (1 << 30);
    double used = (double) info.bytesAvailable() / (1 << 30);

    this->m_pb_capacity->setRange(0, (unsigned int) total);
    this->m_pb_capacity->setValue((unsigned int) (total - used));
    this->m_pb_capacity->setStyleSheet(
        QString("QProgressBar { border: 1px solid black; border-radius: 0px; text-align: center; } \
            QProgressBar::chunk {background-color: hsv(%1, 100%, 100%); width: 1px; }").arg((unsigned int) ((1 - ((total - used) / total)) * 120))
    );
}

void QFileSystemBox::set_directory(const QDir &new_directory) {
    this->m_directory = new_directory;
    this->m_le_path->setText(this->m_directory.path());
    emit this->directory_changed(this->m_directory.path());
}

void QFileSystemBox::select_directory(void) {
    QString new_dir = QFileDialog::getExistingDirectory(
        this,
        this->DialogTitle(),
        this->m_directory.path(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (new_dir == "") {
        logger.debug(Concern::Storage, this->AbortMessage());
    } else {
        this->set_directory(new_dir);
        this->scan_info();
        emit this->directory_changed(this->m_directory);
    }
}

void QFileSystemBox::open_in_explorer(void) const {
    QDesktopServices::openUrl(QUrl::fromLocalFile(this->m_directory.path()));
}
