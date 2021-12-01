#ifndef SNIFFER_H
#define SNIFFER_H
/*
#include <QObject>
#include <QDebug>
//#include <QTcpServer>
//#include <QTcpSocket>
#include <QThreadPool>
#include <QThread>
#include <QSharedPointer>
#include <QMap>*/
#include "manager.h"

class Sniffer : public QTcpServer
{
    Q_OBJECT
public:

    static Sniffer* Instance()
    {
        if (!Sniffer::sniffer)
        {
            Sniffer::sniffer=new Sniffer(nullptr);
        }
        return Sniffer::sniffer;
    }

private:

    explicit Sniffer(QObject *parent = nullptr);

signals:

public slots:
    void start(quint16 port);
    void quit();

protected:
    //Not version friendly!!!
    virtual void incomingConnection(qintptr handle) Q_DECL_OVERRIDE;

    static void newSniffer(qintptr handle);

private:
    static Sniffer* sniffer;
};

#endif // SNIFFER_H
