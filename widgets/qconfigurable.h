#ifndef QCONFIGURABLE_H
#define QCONFIGURABLE_H

#include <QGroupBox>
#include <QObject>
#include <QSettings>

#include "utils/exception.h"

/*
class QAmosWidget: public QGroupBox {
    Q_OBJECT
protected:
    virtual void load_settings(const QSettings * const settings);
    virtual void load_settings_inner(const QSettings * const settings) = 0;
    virtual void load_defaults(void) = 0;
    virtual void save_settings(void) const = 0;

public:
    explicit QAmosWidget(QWidget * parent = nullptr);
    ~QAmosWidget(void);

    virtual void initialize(void) = 0;
};


class QConfigurable: public QAmosWidget {
    Q_OBJECT
protected:
    virtual bool is_config_changed(void) = 0;
    virtual void load_settings(const QSettings * const settings) override;
protected slots:
    void handle_config_changed(void);

public:
    explicit QConfigurable(QWidget * parent = nullptr);
    ~QConfigurable(void);

public slots:

    void apply_changes(void);
    virtual void apply_changes_inner(void) = 0;
    virtual void discard_changes(void) = 0;

signals:
    void settings_changed(void) const;
    void settings_discarded(void) const;
};*/

#endif // QCONFIGURABLE_H
