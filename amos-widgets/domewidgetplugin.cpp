#include "domewidget.h"
#include "domewidgetplugin.h"

#include <QtPlugin>

DomeWidgetPlugin::DomeWidgetPlugin(QObject *parent): QObject(parent) {
    m_initialized = false;
}

void DomeWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */) {
    if (m_initialized) {
        return;
    }

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool DomeWidgetPlugin::isInitialized() const {
    return m_initialized;
}

QWidget *DomeWidgetPlugin::createWidget(QWidget *parent) {
    return new DomeWidget(parent);
}

QString DomeWidgetPlugin::name() const {
    return QLatin1String("DomeWidget");
}

QString DomeWidgetPlugin::group() const {
    return QLatin1String("AMOS");
}

QIcon DomeWidgetPlugin::icon() const {
    return QIcon(":/images/blue.ico");
}

QString DomeWidgetPlugin::toolTip() const {
    return QLatin1String("AMOS dome display");
}

QString DomeWidgetPlugin::whatsThis() const {
    return QLatin1String("");
}

bool DomeWidgetPlugin::isContainer() const {
    return false;
}

QString DomeWidgetPlugin::domXml() const {
    return QLatin1String("<widget class=\"DomeWidget\" name=\"domeWidget\">\n</widget>\n");
}

QString DomeWidgetPlugin::includeFile() const {
    return QLatin1String("amos-domewidget.h");
}
