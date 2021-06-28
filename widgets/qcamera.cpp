#include "qcamera.h"
#include "ui_qcamera.h"

extern EventLogger logger;
extern QSettings * settings;


QCamera::QCamera(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QCamera),
    m_id(""),
    m_darkness_limit(-12.0)
{
    this->ui->setupUi(this);

    this->connect(this->ui->dsb_darkness_limit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QCamera::handle_config_changed);
    this->connect(this->ui->bt_apply, &QPushButton::clicked, this, &QCamera::apply_changes);
    this->connect(this->ui->bt_discard, &QPushButton::clicked, this, &QCamera::discard_changes);

    this->connect(this, &QCamera::settings_changed, this, &QCamera::discard_changes);
    this->connect(this, &QCamera::settings_changed, this, &QCamera::save_settings);

    this->connect(this->ui->scanner, &QScannerBox::sightings_found, this, &QCamera::sightings_found);
    this->connect(this, &QCamera::sightings_found, this, &QCamera::store_sightings);
}

QCamera::~QCamera() {
    delete this->ui;
}

void QCamera::initialize(const QString & id) {
    if (!this->m_id.isEmpty()) {
        throw ConfigurationError("QCamera id already set");
    }
    this->m_id = id;

    this->load_settings(settings);

    this->ui->ufo_manager->initialize(this->id());

    this->ui->scanner->initialize("scanner", "C:/Data");
    this->ui->storage_primary->initialize("primary", "C:/Data");
    this->ui->storage_permanent->initialize("permanent", "C:/Data");
}

bool QCamera::is_darkness_limit_changed(void) const {
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
    logger.info(Concern::Configuration, QString("Station's darkness limit set to %1°").arg(this->m_darkness_limit, 1, 'f', 1));

    emit this->darkness_limit_changed(new_darkness_limit);
}

void QCamera::load_settings(const QSettings * const settings) {
    try {
        this->load_settings_inner(settings);
    } catch (ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
        this->load_defaults();
    }
    this->discard_changes();
}

void QCamera::load_settings_inner(const QSettings * const settings) {
    this->set_darkness_limit(
        settings->value(QString("camera_%1/darkness").arg(this->id()), -12.0).toDouble()
    );
}

void QCamera::load_defaults(void) {
    this->set_darkness_limit(-12.0);
}

void QCamera::save_settings(void) const {
    settings->setValue(QString("camera_%1/darkness").arg(this->id()), this->darkness_limit());
    settings->sync();
}

void QCamera::apply_changes(void) {
    try {
        this->apply_changes_inner();
        emit this->settings_changed();
    } catch (ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
    }
    this->handle_config_changed();
}

void QCamera::apply_changes_inner(void) {
    if (this->is_darkness_limit_changed()) {
        this->set_darkness_limit(this->ui->dsb_darkness_limit->value());
    }
}

void QCamera::discard_changes(void) {
    this->ui->dsb_darkness_limit->setValue(this->darkness_limit());
}

bool QCamera::is_changed(void) const {
    return (abs(this->ui->dsb_darkness_limit->value() - this->darkness_limit()) > 1e-3);
}

void QCamera::handle_config_changed(void) {
    bool changed = this->is_changed();

    this->ui->bt_apply->setText(QString("%1 changes").arg(changed ? "Apply" : "No"));
    this->ui->bt_apply->setEnabled(changed);
    this->ui->bt_discard->setEnabled(changed);
}
