#include <QJsonObject>
#include <QRandomGenerator>

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
    this->connect(this->ui->pb_generate_ufo, &QPushButton::clicked, this, &QCamera::generate_sighting_ufo);
    this->connect(this->ui->pb_generate_kvant, &QPushButton::clicked, this, &QCamera::generate_sighting_kvant);
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
    if (this->is_enabled()) {
        this->ui->ufo_manager->auto_action(is_dark, open_since);
    } else {
        logger.debug(Concern::UFO, QString("Camera %1: automatic action skipped, camera is disabled").arg(this->id()));
    }
}

void QCamera::process_sightings(QVector<Sighting> sightings) {
    for (auto & sighting: sightings) {
        logger.debug(Concern::Sightings, QString("Found sighting %1").arg(sighting.str()));
        emit this->sighting_found(sighting);
    }
}

void QCamera::generate_sighting_kvant() {
    // Generate a fake sighting at current timestamp and save the corresponding files to the Watcher directory
    logger.info(
        Concern::Sightings,
        QString("Generated a Kvant sighting")
    );
    auto timestamp = QDateTime::currentDateTimeUtc();

    QString prefix = QString("%1/%2-%3_%4").arg(
        this->ui->scanner->directory().canonicalPath(),
        timestamp.toString("yyyyMMdd_HHmmss"),
        this->m_station->server()->station_id()
    ).arg(
        0, 5, 10, QLatin1Char('0')
    );

    QFile yaml_file(QString("%1%2.%3").arg(prefix, "", "yaml"));
    yaml_file.open(QIODevice::WriteOnly | QIODevice::Text);

    // Generate a random number of frames between 10 and 50
    std::uniform_int_distribution<int> dist_count(10, 50);
    int frame_count = dist_count(*QRandomGenerator::global());

    QTextStream yaml_stream(&yaml_file);
    yaml_stream <<
        QString("EventStartTime: \"%1\"\n").arg(timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")) <<
        QString("VideoStartTime: \"%1\"\n").arg(timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")) <<
        QString("Name: %1\n").arg(this->m_station->server()->station_id()) <<
        QString("Latitude: %1\n").arg(this->m_station->latitude(), 0, 'f', 6) <<
        QString("Longitude: %1\n").arg(this->m_station->longitude(), 0, 'f', 6) <<
        QString("Altitude: %1\n").arg(this->m_station->altitude(), 0, 'f', 3) <<
        QString("FieldOfViewCenter: 180 90\n") <<
        QString("FieldOfView: 180 140\n") <<
        QString("Resolution: 1600x1200\n") <<
        QString("FPS: 20\n") <<
        QString("PixelSize: 3.4\n") <<
        QString("IntegrationTime: 1\n") <<
        QString("MultipleObjects: False\n") <<
        QString("ObjectOrientationAlignedWithMovement: True\n") <<
        QString("StableMovement: True\n") <<
        QString("Trail:\n");

    // Generate some random coordinates and velocities in detector pixel space
    std::uniform_real_distribution<double> dist_pos(600, 1000);
    double x0 = dist_pos(*QRandomGenerator::global());
    double y0 = dist_pos(*QRandomGenerator::global());
    std::uniform_real_distribution<double> dist_vel(-5, 5);
    double dx = dist_vel(*QRandomGenerator::global());
    double dy = dist_vel(*QRandomGenerator::global());
    std::normal_distribution<double> dist_brightness(128.0, 10.0);

    // For every frame, make up something and add it to the XML file
    for (int frame = 0; frame < frame_count; ++frame) {
        int brightness = static_cast<int>(dist_brightness(*QRandomGenerator::global()));
        yaml_stream << QString("  - {fno: %1, xc: %2, yc: %3, intensity: %4, GaussianHeight: %5, GaussianWidth: %6, GaussianBaseline: %7, NumOversaturatedPixels: %8}\n")
            .arg(frame)
            .arg(x0 + frame * dx, 0, 'f', 3)
            .arg(y0 + frame * dy, 0, 'f', 3)
            .arg(brightness)
            .arg(brightness / 10)
            .arg(1.0)
            .arg(40)
            .arg(2);
    }

    yaml_file.close();
}

void QCamera::generate_sighting_ufo() {
    // Generate a fake sighting at current timestamp and save the corresponding files to the Watcher directory
    logger.info(
        Concern::Sightings,
        QString("Generated a UFO sighting")
    );
    auto timestamp = QDateTime::currentDateTimeUtc();

    QString prefix = QString("%1/M%2_%3_").arg(
        this->ui->scanner->directory().canonicalPath(),
        timestamp.toString("yyyyMMdd_HHmmss"),
        this->m_station->server()->station_id()
    );

    QFile xml_file(QString("%1%2.%3")
                       .arg(prefix, "", "xml"));
    xml_file.open(QIODevice::WriteOnly | QIODevice::Text);

    // Generate a random number of frames between 10 and 50
    std::uniform_int_distribution<int> dist_count(10, 50);
    int frame_count = dist_count(*QRandomGenerator::global());

    QTextStream xml_stream(&xml_file);
    xml_stream
        << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        << "<ufocapture_record version=\"215\" "
        << QString("y=\"%1\" mo=\"%2\" d=\"%3\" h=\"%4\" m=\"%5\" s=\"%6\" ").arg(
                timestamp.toString("yyyy"),
                timestamp.toString("MM"),
                timestamp.toString("dd"),
                timestamp.toString("hh"),
                timestamp.toString("mm"),
                timestamp.toString("ss")
            )
        << QString("trig=\"1\" frames=\"%1\" ").arg(frame_count)
        << QString("lng=\"%1\" lat=\"%2\" alt=\"%3\" ")
            .arg(this->m_station->longitude(), 0, 'f', 6)
            .arg(this->m_station->latitude(), 0, 'f', 6)
            .arg(this->m_station->altitude(), 0, 'f', 3)
        << "tz=\"0\" u2=\"224\" cx=\"1600\" cy=\"1200\" fps=\"20.000\" head=\"20\" tail=\"20\" diff=\"1\" sipos=\"9\" sisize=\"15\" dlev=\"91\" dsize=\"3\" "
        << QString("lid=\"AGO-ALLSKY\" observer=\"%1\" ").arg(this->m_station->server()->station_id())
        << "sid=\"\" cam=\"\" lens=\"\" cap=\"\" comment=\"\" interlace=\"0\" bbf=\"0\" dropframe=\"0\">";
    xml_stream << "<ufocapture_paths hit=\"45\">";

    // Generate some random coordinates and velocities in detector pixel space
    std::uniform_real_distribution<double> dist_pos(600, 1000);
    double x0 = dist_pos(*QRandomGenerator::global());
    double y0 = dist_pos(*QRandomGenerator::global());
    std::uniform_real_distribution<double> dist_vel(-5, 5);
    double dx = dist_vel(*QRandomGenerator::global());
    double dy = dist_vel(*QRandomGenerator::global());
    std::normal_distribution<double> dist_brightness(128.0, 10.0);

    // For every frame, make up something and add it to the XML file
    for (int frame = 0; frame < frame_count; ++frame) {
        int brightness = static_cast<int>(dist_brightness(*QRandomGenerator::global()));
        xml_stream << QString("        <uc_path fno=\"%1\" ono=\"11\" pixel=\"3\" bmax=\"%2\" x=\"%3\" y=\"%4\"></uc_path>\n")
            .arg(frame)
            .arg(brightness)
            .arg(x0 + frame * dx, 0, 'f', 3)
            .arg(y0 + frame * dy, 0, 'f', 3);
    }
    xml_stream << "    </ufocapture_paths>\n</ufocapture_record>";

    xml_file.close();
}

bool QCamera::is_sighting_valid(const Sighting & sighting) const {
    if (sighting.is_spectral() != this->is_spectral()) {
        logger.debug_error(Concern::Sightings,
                     QString("Sighting '%1' does not match the type of this '%2' camera, ignoring")
                         .arg(sighting.prefix(), this->id()));
        return false;
    }

    if (!sighting.station().isEmpty() &&
        (sighting.station() != this->m_station->server()->station_id())) {
        const QString pair = QString("%1|%2").arg(sighting.station(), this->m_station->server()->station_id());
        if (!this->m_station_mismatches.contains(pair)) {
            this->m_station_mismatches.insert(pair);
            logger.warning(Concern::Sightings,
                           QString("Camera '%1': files are named for station '%2' but this client is "
                                   "configured as '%3'. Harmless if the detector simply names itself "
                                   "differently; a misconfiguration if not.")
                               .arg(this->id(), sighting.station(), this->m_station->server()->station_id()));
        }
    }

    if (sighting.dir() != this->ui->scanner->directory().absolutePath()) {
        logger.debug_error(Concern::Sightings,
                           QString("Sighting '%1' dir '%2' does not match scanner directory '%3'!").arg(
                                   sighting.prefix(),
                                   sighting.dir().canonicalPath(),
                                   this->ui->scanner->directory().absolutePath())
        );
        return false;
    }
    return true;
}

void QCamera::store_sighting(Sighting & sighting) {
    if (this->is_sighting_valid(sighting)) {
        try {
            logger.debug(Concern::Sightings,
                         QString("Camera '%1' about to store sighting '%2'").arg(this->id(), sighting.prefix()));
            this->ui->storage_primary->store_sighting(sighting);
            emit this->sighting_stored(sighting);
        } catch (RuntimeException & exc) {
            logger.error(Concern::Sightings, exc.what());
        }
    }
}

void QCamera::quarantine_sighting(Sighting & sighting) {
    // Quarantine the sighting, even if it is invalid
    if (this->is_sighting_valid(sighting)) {
        try {
            logger.debug(Concern::Sightings,
                         QString("Camera '%1' about to quarantine sighting '%2'").arg(this->id(), sighting.prefix()));
            this->ui->storage_primary->quarantine_sighting(sighting);
            emit this->sighting_quarantined(sighting);
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
    this->load_one("darkness limit",
        [this](void) {
            this->set_darkness_limit(
                this->m_settings->value(this->darkness_key(), QCamera::DefaultDarknessLimit).toDouble()
            );
        },
        [this](void) { this->set_darkness_limit(QCamera::DefaultDarknessLimit); }
    );
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
    emit this->ui->dsb_darkness_limit->valueChanged(this->darkness_limit());
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

void QCamera::on_dsb_darkness_limit_valueChanged(double value) {
    Q_UNUSED(value);
    this->display_changed(this->ui->dsb_darkness_limit, this->darkness_limit(), value);
}

