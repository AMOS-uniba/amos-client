#include "include.h"

extern EventLogger logger;

Sighting::Sighting(const QString &jpg, const QString &xml):
    m_jpg(jpg),
    m_xml(xml) {
    logger.info(QString("Created a new sighting (%1 %2)").arg(jpg).arg(xml));

    QFileInfo info(jpg);
    this->m_timestamp = info.birthTime();
}
