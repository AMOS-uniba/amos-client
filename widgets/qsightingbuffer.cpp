#include "qsightingbuffer.h"
#include "ui_qsightingbuffer.h"

QSightingBuffer::QSightingBuffer(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QSightingBuffer)
{
    ui->setupUi(this);
}

QSightingBuffer::~QSightingBuffer() {
    delete ui;
}

void QSightingBuffer::remove(const QString & sighting_id) {

}
