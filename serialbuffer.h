#ifndef SERIALBUFFER_H
#define SERIALBUFFER_H

#include <QObject>
#include <QQueue>

class SerialBuffer: public QObject {
    Q_OBJECT
private:
    QByteArray m_data;
    QQueue<QByteArray> m_messages;
public:
    SerialBuffer(void);
    void insert(const QByteArray &bytes);
    QByteArray pop(void);
signals:
    void message_complete(void) const;
    void full(void) const;
};

#endif // SERIALBUFFER_H
