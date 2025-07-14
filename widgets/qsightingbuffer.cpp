#include <QTimer>

#include "qsightingbuffer.h"
#include "ui_qsightingbuffer.h"
#include "logging/eventlogger.h"
#include "models/qsightingmodel.h"


extern EventLogger logger;

QSightingBuffer::QSightingBuffer(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QSightingBuffer),
    m_last_data()
{
    ui->setupUi(this);
    this->m_sighting_model = new QSightingModel(this);
    this->ui->tv_sightings->setModel(this->m_sighting_model);
    this->ui->tv_sightings->setColumnWidth(0, 250);
    this->ui->tv_sightings->setColumnWidth(1, 100);
    this->ui->tv_sightings->setColumnWidth(2, 150);
    this->ui->tv_sightings->setColumnWidth(3, 100);
    this->ui->tv_sightings->setColumnWidth(4, 100);
    this->ui->tv_sightings->setColumnWidth(5, 100);
    this->ui->tv_sightings->setColumnWidth(6, 150);
    this->ui->tv_sightings->setColumnWidth(7, 150);
    this->ui->tv_sightings->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed);

    this->m_reload_timer = new QTimer(this);
    this->m_reload_timer->setInterval(100);
    this->connect(this->m_reload_timer, &QTimer::timeout, this, &QSightingBuffer::display_time);
    this->m_reload_timer->start();

    this->connect(this->ui->pb_clear, &QPushButton::clicked, this->m_sighting_model, &QSightingModel::clear);
    this->connect(this->ui->pb_send, &QPushButton::clicked, this->m_sighting_model, &QSightingModel::force_send_sightings);
}

QSightingBuffer::~QSightingBuffer() {
    delete this->ui;
}

void QSightingBuffer::handle_sightings_scanned(void) {
    this->m_last_data = QDateTime::currentDateTimeUtc();
}

void QSightingBuffer::display_time(void) {
    this->ui->lb_reloaded->setText(
        QString("reloaded <b>%1 s</b> ago")
            .arg(static_cast<double>((QDateTime::currentDateTimeUtc() - this->m_last_data).count()) / 1000.0, 0, 'f', 1)
    );
}
