#include <QJsonObject>

#include "qcamera.h"
#include "ui_qcamera.h"

#include "widgets/qstation.h"
#include "utils/exceptions.h"


extern EventLogger logger;

QCamera::QCamera(QWidget * parent):
    QAmosWidget(parent),
    ui(new Ui::QCamera),
    m_id(""),
    m_darkness_limit(QCamera::DefaultDarknessLimit)
{
    this->ui->setupUi(this);

    this->ui->sl_dome_open->set_title("Observation begins");
    this->ui->sl_dome_close->set_title("Observation ends");
}

QCamera::~QCamera() {
    delete this->ui;
}

void QCamera::initialize(QSettings * settings, const QString & id, const QStation * const station, bool spectral) {
    if (!this->m_id.isEmpty()) {
        throw ConfigurationError("QCamera id already set");
    }
    this->m_id = id;
    this->m_station = station;
    this->m_spectral = spectral;

    QAmosWidget::initialize(settings);

    this->ui->ufo_manager->initialize(this->id());
    this->ui->scanner->initialize(this->id(), "scanner", QString("C:/Data/%1").arg(spectral ? "Spectral" : "AllSky"));
    this->ui->storage_primary->initialize(this->id(), "primary", QString("C:/Data/%1").arg(spectral ? "Spectral" : "AllSky"));
    this->ui->storage_permanent->initialize(this->id(), "permanent", QString("D:/Data/%1").arg(spectral ? "Spectral" : "AllSky"));
}

void QCamera::connect_slots(void) {
    this->connect(this->ui->cb_enabled, &QCheckBox::checkStateChanged, this, &QCamera::set_enabled);
    this->connect(this->ui->cb_enabled, &QCheckBox::checkStateChanged, this, &QCamera::update_clocks);
    this->connect(this->ui->scanner, &QScannerBox::sightings_found, this, &QCamera::process_sightings);
    this->connect(this->ui->scanner, &QScannerBox::sightings_scanned, this, &QCamera::sightings_scanned);
    this->connect(this->ui->dsb_darkness_limit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QCamera::settings_changed);
    this->connect(this, &QCamera::darkness_limit_changed, this, &QCamera::update_clocks);
}

bool QCamera::is_changed(void) const {
    return (abs(this->ui->dsb_darkness_limit->value() - this->darkness_limit()) > 1e-3);
}

QJsonObject QCamera::json(void) const {
    return QJsonObject {
        {"on", this->is_enabled()},
        {"ufo", this->ui->ufo_manager->json()},
        {"dark", this->darkness_limit()},
        {"disk", QJsonObject {
            {"prim", this->ui->storage_primary->json()},
            {"perm", this->ui->storage_permanent->json()},
        }},
    };
}

void QCamera::auto_action(bool is_dark, const QDateTime & open_since) {
    if (!this->is_enabled()) {
        logger.debug(Concern::UFO, QString("Camera %1: automatic action skipped, camera is disabled").arg(this->id()));
    } else {
        this->ui->ufo_manager->auto_action(is_dark, open_since);
    }
}

void QCamera::process_sightings(QVector<Sighting> sightings) {
    for (auto & sighting: sightings) {
        logger.debug(Concern::Sightings, QString("Found sighting %1").arg(sighting.str()));
        emit this->sighting_found(sighting);
    }
}

bool QCamera::is_sighting_valid(const Sighting & sighting) const {
    if (sighting.is_spectral() != this->is_spectral()) {
        logger.debug_error(Concern::Sightings,
                     QString("Sighting '%1' is not of correct kind for this camera")
                         .arg(sighting.prefix()));
        return false;
    }
    if (sighting.dir() != this->ui->scanner->directory().absolutePath()) {
        logger.debug_error(Concern::Sightings,
                     QString("Sighting '%1' dir '%2' does not match scanner directory '%3'!")
                         .arg(sighting.prefix())
                         .arg(sighting.dir().canonicalPath())
                         .arg(this->ui->scanner->directory().absolutePath()));
        return false;
    }
    return true;
}

void QCamera::discard_sighting(Sighting & sighting) {
    if (this->is_sighting_valid(sighting)) {
        try {
            logger.debug(Concern::Sightings, QString("About to discard sighting '%1'").arg(sighting.prefix()));
            this->ui->storage_primary->discard_sighting(sighting);
            emit this->sighting_discarded(sighting);
        } catch (RuntimeException & exc) {
            logger.error(Concern::Sightings, exc.what());
        }
    }
}

void QCamera::store_sighting(Sighting & sighting) {
    if (this->is_sighting_valid(sighting)) {
        try {
            logger.debug(Concern::Sightings, QString("About to store sighting '%1'").arg(sighting.prefix()));
            this->ui->storage_primary->store_sighting(sighting);
            emit this->sighting_stored(sighting);
        } catch (RuntimeException & exc) {
            logger.error(Concern::Sightings, exc.what());
        }
    }
}

void QCamera::set_darkness_limit(double new_darkness_limit) {
    if ((new_darkness_limit < -18) || (new_darkness_limit > 0)) {
        throw ConfigurationError(QString("Darkness limit out of admissible range: %1°").arg(new_darkness_limit, 1, 'f', 1));
    }

    this->m_darkness_limit = new_darkness_limit;
    logger.info(Concern::Configuration, QString("Camera %1: darkness limit set to %2°").arg(this->id()).arg(this->m_darkness_limit, 1, 'f', 1));

    emit this->darkness_limit_changed(new_darkness_limit);
}

void QCamera::update_clocks(void) {
    this->ui->sl_dome_close->set_value(this->m_station->next_sun_crossing(this->darkness_limit(), true));
    this->ui->sl_dome_open->set_value(this->m_station->next_sun_crossing(this->darkness_limit(), false));
}

void QCamera::load_settings_inner(void) {
    this->set_enabled(this->m_settings->value(this->enabled_key(), QCamera::DefaultEnabled).toBool());
    this->set_darkness_limit(this->m_settings->value(this->darkness_key(), QCamera::DefaultDarknessLimit).toDouble());
}

void QCamera::load_defaults(void) {
    this->set_enabled(QCamera::DefaultEnabled);
    this->set_darkness_limit(QCamera::DefaultDarknessLimit);
}

void QCamera::save_settings_inner(void) const {
    this->m_settings->setValue(this->enabled_key(), this->is_enabled());
    this->m_settings->setValue(this->darkness_key(), this->darkness_limit());
}

void QCamera::apply_changes_inner(void) {
    if (this->is_changed()) {
        this->set_darkness_limit(this->ui->dsb_darkness_limit->value());
    }
}

void QCamera::discard_changes_inner(void) {
    this->ui->dsb_darkness_limit->setValue(this->darkness_limit());
}

void QCamera::set_enabled(int enable) {
    this->m_enabled = (bool) enable;
    logger.info(Concern::Operation, QString("Camera %1: %2abled").arg(this->id(), enable ? "en" : "dis"));

    this->ui->lx_darkness_limit->setEnabled(enable);
    this->ui->dsb_darkness_limit->setEnabled(enable);
    this->ui->sl_dome_open->set_valid(enable);
    this->ui->sl_dome_close->set_valid(enable);
    this->ui->ufo_manager->setEnabled(enable);
    this->ui->scanner->setEnabled(enable);
    this->ui->storage_primary->setEnabled(enable);
    this->ui->storage_permanent->setEnabled(enable);

    this->m_settings->setValue(this->enabled_key(), this->is_enabled());

    const QSignalBlocker blocker(this->ui->cb_enabled);
    this->ui->cb_enabled->setCheckState(enable ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}
