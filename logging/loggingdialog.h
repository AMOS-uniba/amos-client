#ifndef LOGGINGDIALOG_H
#define LOGGINGDIALOG_H

#include <QDialog>
#include <QCheckBox>

namespace Ui {
    class LoggingDialog;
}

class LoggingDialog : public QDialog {
    Q_OBJECT
private:
    Ui::LoggingDialog *ui;
    QVector<QCheckBox*> checkboxes;
public:
    explicit LoggingDialog(QWidget *parent = nullptr);
    ~LoggingDialog();

private slots:

    void on_buttons_accepted();
    void on_buttons_rejected();
    void set_checkbox_all();
    void on_cb_all_clicked(bool checked);
};

#endif // LOGGINGDIALOG_H
