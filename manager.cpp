
#include "manager.h"
#include <QtConcurrent/QtConcurrent>
#include <QMutexLocker>
#include <QSharedPointer>
#include "eventloophelper.h"
#include "endpoint.h"

Manager* Manager::manager;

Manager::Manager(QObject *parent)
    : QTcpServer(parent)
{
    qRegisterMetaType<QAbstractSocket::SocketError>();
}

void Manager::start(quint16 port)
{
    qInfo() << this << " start " << QThread::currentThread();
    if(this->listen(QHostAddress::Any, port))
    {
        qInfo() << "Server started on " << port;
    }
    else
    {
        qCritical() << this->errorString();
    }
}

void Manager::quit()
{
    this->close();
    qInfo() << "Server Stopped!";
}

void Manager::newBridge(qintptr handle)
{
    QList<QByteArray> dataClient;
    QList<QByteArray> dataServer;
    QMutex mutexClient;
    QMutex mutexServer;
    QSemaphore sema;

    QSharedPointer<Endpoint> endpoint(new Endpoint(nullptr));
    QSharedPointer<Client> client(new Client(nullptr, handle));

    connect(client.get(), &Client::dataRead, endpoint.get(), &Endpoint::dataWrite, Qt::ConnectionType::QueuedConnection);
    connect(endpoint.get(), &Endpoint::dataRead, client.get(), &Client::dataWrite, Qt::ConnectionType::QueuedConnection);

    connect(client.get(), &Client::closed, endpoint.get(), &Endpoint::stop, Qt::ConnectionType::QueuedConnection);
    connect(endpoint.get(), &Endpoint::closed, client.get(), &Client::stop, Qt::ConnectionType::QueuedConnection);

    endpoint->setDataIn(&dataServer);
    endpoint->setDataOut(&dataClient);
    endpoint->setMutexIn(&mutexServer);
    endpoint->setMutexOut(&mutexClient);
    endpoint->setSemaphore(&sema);

    client->setDataIn(&dataClient);
    client->setDataOut(&dataServer);
    client->setMutexIn(&mutexClient);
    client->setMutexOut(&mutexServer);
    client->setSemaphore(&sema);

    endpoint->setAutoDelete(false); //!!
    client->setAutoDelete(false);

    QThreadPool pool;// = new QThreadPool(nullptr);
    pool.setMaxThreadCount(2);

    addEndPoint(handle, endpoint);

    pool.start(client.get());
    pool.start(endpoint.get());

    //std::atomic<int> i;
    EventLoopHelper* eventLoop = new EventLoopHelper(nullptr,2);

    connect(client.get(), &Client::closed,eventLoop,&EventLoopHelper::inc,Qt::ConnectionType::QueuedConnection);
    connect(endpoint.get(), &Endpoint::closed,eventLoop,&EventLoopHelper::inc,Qt::ConnectionType::QueuedConnection);

    eventLoop->exec();

    pool.waitForDone();    

    eventLoop->deleteLater();
    eventLoop=nullptr;

    removeEndPoint(handle);

    endpoint->deleteLater(); //
    client->deleteLater();

    //endpoint=nullptr;
    client=nullptr;

    qInfo() << "                 Bridge Exit!";
}

void Manager::incomingConnection(qintptr handle)
{
    QFuture<void> future = QtConcurrent::run(Manager::newBridge,handle);
}
