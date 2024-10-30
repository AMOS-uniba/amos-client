#include <QTimer>

#include "qsightingbuffer.h"
#include "ui_qsightingbuffer.h"
#include "logging/eventlogger.h"
#include "models/qsightingmodel.h"


extern EventLogger logger;

QSightingBuffer::QSightingBuffer(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QSightingBuffer)
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
}

QSightingBuffer::~QSightingBuffer() {
    delete this->ui;
}
