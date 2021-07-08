#include "qcamera.h"
#include "ui_qcamera.h"

extern EventLogger logger;
extern QSettings * settings;


QCamera::QCamera(QWidget * parent):
    QAmosWidget(parent),
    ui(new Ui::QCamera),
    m_id(""),
    m_darkness_limit(-12.0)
{
    this->ui->setupUi(this);

    this->connect(this->ui->scanner, &QScannerBox::sightings_found, this, &QCamera::sightings_found);
    this->connect(this, &QCamera::sightings_found, this, &QCamera::store_sightings);

    this->ui->sl_dome_close->set_title("Observation ends");
    this->ui->sl_dome_open->set_title("Observation starts");
}

QCamera::~QCamera() {
    delete this->ui;
}

void QCamera::initialize(const QString & id, const QStation * const station) {
    if (!this->m_id.isEmpty()) {
        throw ConfigurationError("QCamera id already set");
    }
    this->m_id = id;
    this->m_station = station;

    QAmosWidget::initialize();

    this->ui->ufo_manager->initialize(this->id());
    this->ui->scanner->initialize("scanner", "C:/Data");
    this->ui->storage_primary->initialize("primary", "C:/Data");
    this->ui->storage_permanent->initialize("permanent", "C:/Data");
}

void QCamera::connect_slots(void) {
    this->connect(this->ui->dsb_darkness_limit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QCamera::settings_changed);
    this->connect(this, &QCamera::darkness_limit_changed, this, &QCamera::update_clocks);
}

bool QCamera::is_changed(void) const {
    return (abs(this->ui->dsb_darkness_limit->value() - this->darkness_limit()) > 1e-3);
}

QJsonObject QCamera::json(void) const {
    return QJsonObject {
        {"ufo", this->ui->ufo_manager->json()},
        {"dark", this->darkness_limit()},
        {"disk", QJsonObject {
            {"prim", this->ui->storage_primary->json()},
            {"perm", this->ui->storage_permanent->json()},
        }},
    };
}

const QUfoManager * QCamera::ufo_manager(void) const {
    return this->ui->ufo_manager;
}

void QCamera::store_sightings(QVector<Sighting> sightings) {
    logger.debug(Concern::Sightings, "Storing sightings...");

    for (auto && sighting: sightings) {
        this->ui->storage_primary->store_sighting(sighting, true);
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
    this->ui->sl_dome_close->set_value(
        Universe::next_sun_crossing(this->m_station->latitude(), this->m_station->longitude(), this->darkness_limit(), true)
    );
    this->ui->sl_dome_open->set_value(
        Universe::next_sun_crossing(this->m_station->latitude(), this->m_station->longitude(), this->darkness_limit(), false)
    );
}

void QCamera::load_settings_inner(const QSettings * const settings) {
    this->set_darkness_limit(
        settings->value(QString("camera_%1/darkness").arg(this->id()), -12.0).toDouble()
    );
}

void QCamera::load_defaults(void) {
    this->set_darkness_limit(-12.0);
}

void QCamera::save_settings_inner(QSettings * settings) const {
    settings->setValue(QString("camera_%1/darkness").arg(this->id()), this->darkness_limit());
}

void QCamera::apply_changes_inner(void) {
    if (this->is_changed()) {
        this->set_darkness_limit(this->ui->dsb_darkness_limit->value());
    }
}

void QCamera::discard_changes_inner(void) {
    this->ui->dsb_darkness_limit->setValue(this->darkness_limit());
}
