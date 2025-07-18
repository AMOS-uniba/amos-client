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

void QAmosWidget::display_changed(QWidget * widget, QVariant new_value, QVariant old_value) {
    widget->setStyleSheet(
        QString("background-color: %1; font-weight: %2;").arg(
            new_value != old_value ? "hsl(25, 100%, 70%)" : "white",
            new_value != old_value ? "bold" : "normal"
        )
    );
}

void QAmosWidget::initialize(QSettings * settings) {
    this->m_settings = settings;
    this->connect_slots();
    this->load_settings();
}
