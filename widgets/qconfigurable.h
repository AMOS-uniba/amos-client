#ifndef QCONFIGURABLE_H
#define QCONFIGURABLE_H

#include <QGroupBox>
#include <QObject>
#include <QSettings>

#include "utils/exception.h"

/*
class QAmosWidgetMixin {
    Q_OBJECT
protected:
    void load_settings(const QSettings * const settings);

    virtual bool is_changed(void) const = 0;

    virtual void load_settings_inner(const QSettings * const settings) = 0;
    virtual void apply_changes_inner(void) = 0;

protected slots:
    void apply_changes(void);

    virtual void load_defaults(void) = 0;
    virtual void save_settings(QSettings * const settings) const = 0;
    virtual void discard_changes(void) = 0;
    virtual void handle_config_changed(void) = 0;

public:
    explicit QAmosWidgetMixin(void);
    ~QAmosWidgetMixin(void);

public slots:
    virtual void initialize(void) = 0;

signals:
    void settings_changed(void) const;
    void settings_discarded(void) const;
};*/

#endif // QCONFIGURABLE_H
