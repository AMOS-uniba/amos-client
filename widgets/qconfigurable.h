#ifndef QCONFIGURABLE_H
#define QCONFIGURABLE_H

#include <QGroupBox>
#include <QObject>
#include <QSettings>


class QAmosWidget: public QGroupBox {
    Q_OBJECT
protected:
    QSettings * m_settings;

    virtual void load_defaults(void) = 0;
    virtual void connect_slots(void) = 0;
    virtual void load_settings_inner(void) = 0;
    virtual void save_settings_inner(void) const = 0;
    virtual void apply_changes_inner(void) = 0;
    virtual void discard_changes_inner(void) = 0;

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
