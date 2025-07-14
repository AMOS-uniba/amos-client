#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

namespace Ui {
    class AboutDialog;
    }

class QAboutDialog: public QDialog {
    Q_OBJECT

public:
    explicit QAboutDialog(QWidget * parent = nullptr);
    ~QAboutDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::AboutDialog * ui;
};

#endif // ABOUTDIALOG_H
