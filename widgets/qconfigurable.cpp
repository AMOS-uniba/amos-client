#include "widgets/qconfigurable.h"
#include "utils/exceptions.h"

#include "logging/eventlogger.h"


extern EventLogger logger;

QAmosWidget::QAmosWidget(QWidget * parent):
    QGroupBox(parent) {}

QAmosWidget::~QAmosWidget(void) {}

void QAmosWidget::load_settings(void) {
    try {
        this->load_settings_inner();
    } catch (ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
        this->load_defaults();
    }
}

void QAmosWidget::save_settings(void) const {
    this->save_settings_inner();
    this->m_settings->sync();
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

void QAmosWidget::initialize(QSettings * settings) {
    this->m_settings = settings;
    this->connect_slots();
    this->load_settings();
}
