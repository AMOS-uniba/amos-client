#include "qufomanager.h"
#include "ui_qufomanager.h"

QUfoManager::QUfoManager(QWidget * parent):
    QGroupBox(parent),
    ui(new Ui::QUfoManager)
{
    ui->setupUi(this);
}

QUfoManager::~QUfoManager() {
    delete ui;
}

void QUfoManager::on_cb_auto_clicked(bool checked) {
    this->set_autostart(checked);
}

bool QUfoManager::autostart(void) const { return this->m_autostart; }
void QUfoManager::set_autostart(bool enable) { this->m_autostart = enable; }
