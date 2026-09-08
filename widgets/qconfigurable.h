#ifndef QCONFIGURABLE_H
#define QCONFIGURABLE_H

#include <QGroupBox>
#include <QObject>
#include <QSettings>

#include "utils/exceptions.h"


class QAmosWidget: public QGroupBox {
    Q_OBJECT
protected:
    QSettings * m_settings;

    // Report one unusable setting. Split out so that load_one() below can stay in the header
    // without dragging the logger in with it.
    void report_setting_error(const QString & what, const QString & why) const;

    /** Load one setting, falling back to its own default when the stored value cannot be used.
     *
     *  Every setting read in a load_settings_inner() wants this. Without it a single unusable value
     *  throws out of load_settings_inner(), and load_settings() answers that by calling
     *  load_defaults() -- which discards every *other* setting in the same widget. One mistyped
     *  humidity limit therefore reset the dome's serial port as well and took the station offline;
     *  a station id of the wrong length sent the client's reports to 127.0.0.1. Neither has
     *  anything to do with the value that was actually wrong.
    **/
    template <typename Stored, typename Fallback>
    void load_one(const QString & what, Stored stored, Fallback fallback) {
        try {
            stored();
        } catch (ConfigurationError & e) {
            this->report_setting_error(what, e.what());
            fallback();
        }
    }

    virtual void load_defaults(void) = 0;
    virtual void connect_slots(void) = 0;
    virtual void load_settings_inner(void) = 0;
    virtual void save_settings_inner(void) const = 0;
    virtual void apply_changes_inner(void) = 0;
    virtual void discard_changes_inner(void) = 0;

    void display_changed(QWidget * widget, QVariant new_value, QVariant old_value);

public:
    explicit QAmosWidget(QWidget * parent = nullptr);
    ~QAmosWidget(void);

    virtual bool is_changed(void) const = 0;

public slots:
    virtual void initialize(QSettings * settings);

    void load_settings(void);
    void save_settings(void) const;
    void apply_changes(void);
    void discard_changes(void);

signals:
    void settings_changed(void);
    void settings_saved(void);
    void settings_discarded(void);
};

#endif // QCONFIGURABLE_H
