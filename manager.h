#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThreadPool>
#include <QThread>
#include <QSharedPointer>
#include <QMap>
#include "client.h"

class Manager : public QTcpServer
{
    Q_OBJECT
public:
    static Manager* Instance()
    {
        if (!Manager::manager)
        {
            Manager::manager=new Manager(nullptr);
        }
        return Manager::manager;
    }    

private:
    explicit Manager(QObject *parent = nullptr);

signals:

public slots:
    void start(quint16 port);
    void quit();

    // QTcpServer interface
protected:
    //Not version friendly!!!
    virtual void incomingConnection(qintptr handle) Q_DECL_OVERRIDE;

    static void newBridge(qintptr handle);

private:
    static Manager* manager;
};

#endif // MANAGER_H
