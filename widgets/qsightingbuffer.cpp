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
}

QSightingBuffer::~QSightingBuffer() {
    delete ui;
}

void QSightingBuffer::insert(const Sighting & sighting) {
    if (this->m_sightings.contains(sighting.prefix())) {
        logger.debug(Concern::Sightings, QString("Sighting '%1' already in buffer").arg(sighting.prefix()));
    }
    this->m_sightings.insert(sighting.prefix(), sighting);
}

void QSightingBuffer::remove(const QString & sighting_id) {
    this->m_sightings.remove(sighting_id);
}

void QSightingBuffer::defer(const QString & sighting_id) {
    if (this->m_sightings.contains(sighting_id)) {
        this->m_sightings[sighting_id].defer(QSightingBuffer::DeferTime);
    } else {
        logger.warning(Concern::Sightings, QString("Cannot defer nonexistent sighting '%1'!").arg(sighting_id));
    }
}


/*
void QCamera::defer_sighting(const QString & sighting_id) {
    try {
        auto sighting = Sighting(this->ui->scanner->directory().path(), sighting_id, this->m_spectral);
        auto until = QDateTime::currentDateTimeUtc().addSecs(QCamera::DeferTime);
        logger.debug(Concern::Sightings,
                     QString("Deferring sighting '%1' until %2")
                         .arg(sighting.prefix())
                         .arg(until.toString(Qt::ISODate)));
        this->m_deferred_sightings.insert(sighting.prefix(), until);
        for (auto & k: this->m_deferred_sightings) {
            logger.debug(Concern::Sightings, QString("Deferred until %1").arg(k.toString()));
        }
    } catch (InvalidSighting & exc) {
        // pass
    } catch (RuntimeException & exc) {
        logger.error(Concern::Sightings, exc.what());
    }
}
*/
