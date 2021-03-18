#ifndef QCONFIGURABLE_H
#define QCONFIGURABLE_H

#include <QGroupBox>
#include <QObject>
#include <QSettings>

#include "utils/exception.h"

namespace Ui {
    class QConfigurable;
}

class QConfigurable: public QGroupBox {
    Q_OBJECT
protected:
    Ui::QConfigurable *ui;

    virtual bool is_changed(void) = 0;
protected slots:
    void load_settings(const QSettings * const settings);
    virtual void load_settings_inner(const QSettings * const settings) = 0;
    virtual void save_settings(void) = 0;

    void handle_settings_changed(void);

public:
    explicit QConfigurable(QWidget * parent = nullptr);
    ~QConfigurable(void);

public slots:
    void apply_settings(void);
    virtual void apply_settings_inner(void) = 0;
    virtual void discard_settings(void) = 0;

signals:
    void settings_changed(void) const;
};

#endif // QCONFIGURABLE_H
