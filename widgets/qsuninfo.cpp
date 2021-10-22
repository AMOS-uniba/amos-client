#include "include.h"

#include "qsuninfo.h"
#include "ui_qsuninfo.h"
#include "APC/APC_VecMat3D.h"

#include "widgets/qstation.h"

extern EventLogger logger;

QSunInfo::QSunInfo(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::QSunInfo)
{
    ui->setupUi(this);

    this->ui->sl_altitude->set_colour_formatter(&Universe::altitude_colour);

    this->m_timer_short = new QTimer(this);
    this->m_timer_short->setInterval(200);
    this->connect(this->m_timer_short, &QTimer::timeout, this, &QSunInfo::update_short_term);
    this->m_timer_short->start();

    this->m_timer_long = new QTimer(this);
    this->m_timer_long->setInterval(60000);
    this->connect(this->m_timer_long, &QTimer::timeout, this, &QSunInfo::update_long_term);
    this->m_timer_long->start();

    ValueFormatter<double> altitude_formatter = [](double altitude) {
        return QString("%1°").arg(altitude, 0, 'f', 3);
    };

    ValueFormatter<double> azimuth_formatter = [](double azimuth) {
        return QString("%1°").arg(fmod(azimuth + 360.0, 360.0), 0, 'f', 3);
    };

    ValueFormatter<double> illumination_formatter = [](double illumination) {
        return QString("%1").arg(illumination, 3, 'f', 3);
    };

    this->ui->sl_altitude->set_title("Altitude");
    this->ui->sl_altitude->set_valid(true);
    this->ui->sl_altitude->set_value_formatter(altitude_formatter);

    this->ui->sl_azimuth->set_title("Azimuth");
    this->ui->sl_azimuth->set_valid(true);
    this->ui->sl_azimuth->set_value_formatter(azimuth_formatter);

    this->ui->sl_dec->set_title("δ");
    this->ui->sl_dec->set_tooltip("Declination");
    this->ui->sl_dec->set_valid(true);
    this->ui->sl_dec->set_value_formatter(altitude_formatter);

    this->ui->sl_ra->set_title("α");
    this->ui->sl_ra->set_tooltip("Right ascension");
    this->ui->sl_ra->set_valid(true);
    this->ui->sl_ra->set_value_formatter(azimuth_formatter);

    this->ui->dtl_sunrise->set_title("Sunrise");
    this->ui->dtl_sunset->set_title("Sunset");

    this->ui->sl_moon_altitude->set_title("Altitude");
    this->ui->sl_moon_altitude->set_valid(true);
    this->ui->sl_moon_altitude->set_value_formatter(altitude_formatter);

    this->ui->sl_moon_azimuth->set_title("Azimuth");
    this->ui->sl_moon_azimuth->set_valid(true);
    this->ui->sl_moon_azimuth->set_value_formatter(azimuth_formatter);

    this->ui->sl_moon_elongation->set_title("Elongation");
    this->ui->sl_moon_elongation->set_valid(true);
    this->ui->sl_moon_elongation->set_value_formatter(azimuth_formatter);

    this->ui->sl_moon_illumination->set_title("Illumination");
    this->ui->sl_moon_illumination->set_valid(true);
    this->ui->sl_moon_illumination->set_value_formatter(illumination_formatter);

    this->ui->dtl_moonrise->set_title("Moonrise");
    this->ui->dtl_moonset->set_title("Moonset");
}

QSunInfo::~QSunInfo() {
    delete this->ui;
    delete this->m_timer_short;
    delete this->m_timer_long;
}

void QSunInfo::update_short_term(void) {
    auto sun_hor = this->m_station->sun_position();
    double alt = sun_hor.theta * Deg;
    this->ui->sl_altitude->set_value(alt);
    this->ui->sl_azimuth->set_value(sun_hor.phi * Deg);

    auto moon_hor = this->m_station->moon_position();
    this->ui->sl_moon_altitude->set_value(moon_hor.theta * Deg);
    this->ui->sl_moon_azimuth->set_value(moon_hor.phi * Deg);
    auto sun_xyz = Vec3D(sun_hor);
    auto moon_xyz = Vec3D(moon_hor);
    double dot = (sun_xyz * moon_xyz) / (sun_hor.r * moon_hor.r);
    double elongation = acos(dot) * Deg;
    double illumination = (1.0 - dot) / 2.0;

    this->ui->sl_moon_elongation->set_value(elongation);
    this->ui->sl_moon_illumination->set_value(illumination);

    QColor colour = QColor();
    if (sun_hor.theta > 0) {
        this->ui->lb_sun_status->setText("day");
        this->ui->lb_sun_status->setToolTip("Sun is above the horizon");
        colour = Universe::altitude_colour(sun_hor.theta * Deg);
    } else {
        if (this->m_station->is_dark_allsky()) {
            this->ui->lb_sun_status->setText("dark");
            this->ui->lb_sun_status->setToolTip("Sun is below the horizon and below the darkness limit");
            colour = Qt::black;
        } else {
            this->ui->lb_sun_status->setText("dusk");
            this->ui->lb_sun_status->setToolTip("Sun is below the horizon, but above the darkness limit");
            colour = Qt::blue;
        }
    }

    this->ui->lb_sun_status->setStyleSheet(QString("QLabel { color: %1; }").arg(colour.name()));
}

void QSunInfo::set_station(const QStation * const station) { this->m_station = station; }

void QSunInfo::update_long_term(void) {
    auto equ = Universe::compute_sun_equ();
    this->ui->sl_dec->set_value(equ[theta] * Deg);
    this->ui->sl_ra->set_value(equ[phi] * Deg);

    this->ui->dtl_sunrise->set_value(this->m_station->next_sun_crossing(-0.5, true));
    this->ui->dtl_sunset->set_value(this->m_station->next_sun_crossing(-0.5, false));

    this->ui->dtl_moonrise->set_value(this->m_station->next_moon_crossing(-0.5, true));
    this->ui->dtl_moonset->set_value(this->m_station->next_moon_crossing(-0.5, false));
}
