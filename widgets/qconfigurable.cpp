#include "include.h"
#include "qconfigurable.h"

extern EventLogger logger;
extern QSettings * settings;


QAmosWidget::QAmosWidget(QWidget * parent):
    QGroupBox(parent) {}

QAmosWidget::~QAmosWidget(void) {}

void QAmosWidget::load_settings(const QSettings * const settings) {
    try {
        this->load_settings_inner(settings);
    } catch (ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
        this->load_defaults();
    }
}

void QAmosWidget::save_settings(QSettings * settings) const {
    this->save_settings_inner(settings);
}

void QAmosWidget::apply_changes(void) {
    try {
        this->apply_changes_inner();
        emit this->settings_changed();
    } catch (ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
    }
}

void QAmosWidget::discard_changes(void) {
    this->discard_changes_inner();
}

void QAmosWidget::initialize(void) {
    this->connect_slots();
    this->load_settings(settings);
}
