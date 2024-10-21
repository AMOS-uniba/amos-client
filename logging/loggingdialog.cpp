#include <QCheckBox>

#include "loggingdialog.h"
#include "ui_loggingdialog.h"

#include "eventlogger.h"

extern EventLogger logger;
extern QSettings * settings;

LoggingDialog::LoggingDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::LoggingDialog)
{
    this->ui->setupUi(this);

    for (auto concern = EventLogger::Concerns.keyBegin(); concern != EventLogger::Concerns.keyEnd(); ++concern) {
        const ConcernInfo & info = EventLogger::Concerns[*concern];
        QCheckBox *checkbox = new QCheckBox(info.name, this);
        this->checkboxes.append(checkbox);
        this->ui->vl_checkboxes->addWidget(checkbox);
        checkbox->setObjectName(info.name);
        checkbox->setText(info.caption);
        checkbox->setCheckState(logger.is_debug_visible(*concern) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

        this->connect(checkbox, &QCheckBox::checkStateChanged, this, &LoggingDialog::set_checkbox_all);
    }
    this->set_checkbox_all();
}

LoggingDialog::~LoggingDialog() {
    delete ui;
}

void LoggingDialog::on_buttons_rejected() {
    this->close();
}

void LoggingDialog::set_checkbox_all() {
    bool all = true;
    for (auto && checkbox: this->checkboxes) {
        all &= (checkbox->checkState() == Qt::CheckState::Checked);
    }
    this->ui->cb_all->setCheckState(all ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}

void LoggingDialog::on_buttons_accepted() {
    auto concerns = EventLogger::Concerns;
    for (auto && checkbox: this->checkboxes) {
        for (auto concern = concerns.cbegin(); concern != concerns.cend(); ++concern) {
            if (concern.value().name == checkbox->objectName()) {
                logger.set_debug_visible(concern.key(), (checkbox->checkState() == Qt::CheckState::Checked));
            }
        }
    }
    logger.save_settings(settings);
}

void LoggingDialog::on_cb_all_clicked(bool checked) {
    for (auto && checkbox: this->checkboxes) {
        checkbox->setCheckState(checked ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    }
}
