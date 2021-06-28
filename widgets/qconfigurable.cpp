#include "include.h"
#include "qconfigurable.h"

extern EventLogger logger;

/*
QAmosWidgetMixin::QAmosWidgetMixin(void) {
}

void QAmosWidgetMixin::load_settings(const QSettings * const settings) {
    try {
        this->load_settings_inner(settings);
    } catch (ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
        this->load_defaults();
    }
    this->discard_changes();
}

void QAmosWidgetMixin::apply_changes(void) {
    try {
        this->apply_changes_inner();
        emit this->settings_changed();
    } catch (ConfigurationError & e) {
        logger.error(Concern::Configuration, e.what());
    }
    this->handle_config_changed();
}

void QAmosWidgetMixin::handle_config_changed(void) {
  //  bool changed = this->is_changed();

 //   this->ui->bt_apply->setText(QString("%1 changes").arg(changed ? "Apply" : "No"));
 //   this->ui->bt_apply->setEnabled(changed);
 //   this->ui->bt_discard->setEnabled(changed);
}
*/
