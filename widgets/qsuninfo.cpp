#include "qsuninfo.h"
#include "ui_qsuninfo.h"

QSunInfo::QSunInfo(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::QSunInfo)
{
    ui->setupUi(this);

    this->ui->sl_altitude->set_colour_formatter(&Universe::altitude_colour);

    this->m_timer_short = new QTimer(this);
    this->m_timer_short->setInterval(20);
    this->connect(this->m_timer_short, &QTimer::timeout, this, &QSunInfo::update_short_term);
    this->m_timer_short->start();

    this->m_timer_long = new QTimer(this);
    this->m_timer_long->setInterval(60000);
    this->connect(this->m_timer_long, &QTimer::timeout, this, &QSunInfo::update_long_term);
    this->m_timer_long->start();

    ValueFormatter<double> angle_formatter = [=](double angle) {
        return QString("%1°").arg(fmod(angle + 360.0, 360.0), 0, 'f', 3);
    };

    this->ui->sl_altitude->set_title("Altitude θ");
    this->ui->sl_altitude->set_valid(true);
    this->ui->sl_altitude->set_value_formatter(angle_formatter);

    this->ui->sl_azimuth->set_title("Azimuth a");
    this->ui->sl_azimuth->set_valid(true);
    this->ui->sl_azimuth->set_value_formatter(angle_formatter);

    this->ui->sl_dec->set_title("Declination δ");
    this->ui->sl_dec->set_valid(true);
    this->ui->sl_dec->set_value_formatter(angle_formatter);

    this->ui->sl_ra->set_title("Right ascension α");
    this->ui->sl_ra->set_valid(true);
    this->ui->sl_ra->set_value_formatter(angle_formatter);

    this->ui->sl_ecl_lon->set_title("Ecliptical longitude λ");
    this->ui->sl_ecl_lon->set_valid(true);
    this->ui->sl_ecl_lon->set_value_formatter(angle_formatter);

    this->ui->sl_moon_altitude->set_title("Moon altitude");
    this->ui->sl_moon_altitude->set_valid(true);
    this->ui->sl_moon_altitude->set_value_formatter(angle_formatter);

    this->ui->sl_moon_azimuth->set_title("Moon azimuth");
    this->ui->sl_moon_azimuth->set_valid(true);
    this->ui->sl_moon_azimuth->set_value_formatter(angle_formatter);

    this->ui->sl_dome_close->set_title("Dome closing");
    this->ui->sl_sunrise->set_title("Sunrise");
    this->ui->sl_sunset->set_title("Sunset");
    this->ui->sl_dome_open->set_title("Dome opening");
}

QSunInfo::~QSunInfo() {
    delete this->ui;
}

void QSunInfo::update_short_term(void) {
    auto hor = this->m_station->sun_position();
    this->ui->sl_altitude->set_value(hor.theta * Deg);
    this->ui->sl_azimuth->set_value(hor.phi * Deg);

    auto moon_hor = this->m_station->moon_position();
    this->ui->sl_moon_altitude->set_value(moon_hor.theta * Deg);
    this->ui->sl_moon_azimuth->set_value(moon_hor.phi * Deg);

    QColor colour = QColor();
    if (hor.theta > 0) {
        this->ui->lb_sun_status->setText("day");
        this->ui->lb_sun_status->setToolTip("Sun is above the horizon");
        colour = Universe::altitude_colour(hor.theta * Deg);
    } else {
        if (this->m_station->is_dark()) {
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
//    this->setTitle(QString("Fucking %1").arg(this->height()));
}

void QSunInfo::set_station(const Station *station) { this->m_station = station; }

void QSunInfo::update_long_term(void) {
    auto equ = Universe::compute_sun_equ();
    this->ui->sl_dec->set_value(equ[theta] * Deg);
    this->ui->sl_ra->set_value(equ[phi] * Deg);

    auto ecl = Universe::compute_sun_ecl();
    this->ui->sl_ecl_lon->set_value(ecl[phi] * Deg);

    this->ui->sl_dome_close->set_value(this->m_station->next_sun_crossing(this->m_station->darkness_limit(), true));
    this->ui->sl_sunrise->set_value(this->m_station->next_sun_crossing(-0.5, true));
    this->ui->sl_sunset->set_value(this->m_station->next_sun_crossing(-0.5, false));
    this->ui->sl_dome_open->set_value(this->m_station->next_sun_crossing(this->m_station->darkness_limit(), false));
}
