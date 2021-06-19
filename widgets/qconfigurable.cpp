#include "include.h"
#include "qconfigurable.h"
//#include "ui_qconfigurable.h"

extern EventLogger logger;

/*
void QAmosWidget::load_settings(const QSettings * const settings) {
    try {
        this->load_settings_inner(settings);
    } catch (ConfigurationError &e) {
        this->load_defaults();
    }
//    this->refresh();
}




QConfigurable::QConfigurable(QWidget * parent):
    QAmosWidget(parent)
{

}

QConfigurable::~QConfigurable(void) {}


void QConfigurable::load_settings(const QSettings * const settings) {
    QAmosWidget::load_settings(settings);
    this->discard_changes();
}



void QConfigurable::apply_settings(void) {
    try {
        this->apply_settings_inner();
        emit this->settings_changed();
    } catch (ConfigurationError &e) {
        logger.error(Concern::Configuration, e.what());
    }
    this->handle_settings_changed();
}

void QConfigurable::handle_settings_changed(void) {
    bool changed = this->is_changed();

 //   this->ui->bt_apply->setText(QString("%1 changes").arg(changed ? "Apply" : "No"));
 //   this->ui->bt_apply->setEnabled(changed);
 //   this->ui->bt_discard->setEnabled(changed);
}
*/
