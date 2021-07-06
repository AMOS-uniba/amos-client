#ifndef QCONFIGURABLE_H
#define QCONFIGURABLE_H

#include <QGroupBox>
#include <QObject>
#include <QSettings>

#include "utils/exception.h"


class QAmosWidget: public QGroupBox {
    Q_OBJECT
protected:
    virtual void load_defaults(void) = 0;
    virtual void connect_slots(void) = 0;
    virtual void load_settings_inner(const QSettings * const settings) = 0;
    virtual void save_settings_inner(QSettings * settings) const = 0;
    virtual void apply_changes_inner(void) = 0;
    virtual void discard_changes_inner(void) = 0;

public:
    explicit QAmosWidget(QWidget * parent = nullptr);
    ~QAmosWidget(void);

    virtual bool is_changed(void) const = 0;

public slots:
    virtual void initialize(void);

    void load_settings(const QSettings * const settings);
    void save_settings(QSettings * settings) const;
    void apply_changes(void);
    void discard_changes(void);

signals:
    void settings_changed(void) const;
    void settings_saved(void) const;
    void settings_discarded(void) const;
};

#endif // QCONFIGURABLE_H
