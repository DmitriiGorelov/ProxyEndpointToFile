#include "client.h"

Client::Client(QObject *parent, qintptr _handle)
    : QObject(parent)
    , QRunnable ()
    , handle(0)
    , socket()
    , canRun()
    , firstDataIn()
    , dataIn(nullptr)
    , dataOut(nullptr)
    , mutexIn(nullptr)
    , mutexOut(nullptr)
    , semaphore(nullptr)
    , m_name()
    , m_mutexName()
    , eventLoop(nullptr)
    , m_mutexEventLoop()
{
    handle = _handle;
}

void Client::setDataIn(QList<QByteArray>* data)
{
    dataIn=data;
}

void Client::setDataOut(QList<QByteArray>* data)
{
    dataOut=data;
}

void Client::setMutexIn(QMutex* mutex)
{
    mutexIn=mutex;
}

void Client::setMutexOut(QMutex* mutex)
{
    mutexOut=mutex;
}

void Client::setSemaphore(QSemaphore* sema)
{
    semaphore=sema;
}

QString Client::name()
{
    QMutexLocker locker(&m_mutexName);
    return m_name;
}

void Client::name(const QString& value)
{
    QMutexLocker locker(&m_mutexName);
    m_name=value;
}

void Client::addDataOut(QByteArray data)
{
    if (dataOut && mutexOut)
    {
        mutexOut->lock();
        dataOut->append(data);
        mutexOut->unlock();
    }
}

void Client::dataWrite()
{
    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();
    //emit canWrite();
}

void Client::stop()
{
    canRun=false;
    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();
    //emit stopped();
}

void Client::run()
{
        canRun=true;
        firstDataIn=true;

        qInfo() << this << " run " << QThread::currentThread();

        if (semaphore)
        {
            //return;
            semaphore->acquire(1);
        }

        socket = new QTcpSocket(nullptr);

        if(!socket->setSocketDescriptor(handle))
        {
            qCritical() << socket->errorString();
            delete socket;
            return;
        }

        eventLoop = new QEventLoop();

        connect(socket, &QTcpSocket::readyRead,this,&Client::socketReadyRead,Qt::ConnectionType::DirectConnection);
        connect(socket, &QTcpSocket::disconnected,this,&Client::socketDisconnected,Qt::ConnectionType::DirectConnection);
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(soketError(QAbstractSocket::SocketError)));
        /*connect(socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), [&] (QAbstractSocket::SocketError error) {
            //qDebug()<< "ERROR " << socket->errorString();
            //socket->deleteLater();
            soketError(error);
        });*/

        if (dataIn && dataOut && mutexIn && mutexOut)
        {           
            QMetaObject::invokeMethod(this,"socketReadyRead",Qt::QueuedConnection);


            //connect(this, &Client::canWrite,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
            //connect(this, &Client::someError,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
            //connect(this, &Client::stopped,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);

            while (canRun)
            {
                //QEventLoop* eventLoop = new QEventLoop();

                /*connect(this, &Client::canWrite,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
                connect(this, &Client::someError,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
                connect(this, &Client::stopped,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
*/
                eventLoop->exec();
                //eventLoop->deleteLater();
                //eventLoop=nullptr;

                if (canRun)
                {
                    mutexOut->lock();
                    while (dataOut->size()>0)
                    {
                        QByteArray data= dataOut->front();
                        dataOut->pop_front();
                        if (socket->state()==QTcpSocket::SocketState::ConnectedState)
                        {
                            //if (!name().isEmpty())
                                qInfo() << "IN " << name().toStdString().c_str() << " bytes: " << data.size();

                            if (socket->write(data))
                            {
                                if (socket->waitForBytesWritten())
                                {

                                }
                                else
                                {
                                    qInfo() << socket->error();
                                    qCritical() << socket->errorString();
                                    canRun=false;
                                    emit someError();
                                }
                            }
                            else
                            {
                                qInfo() << socket->error();
                                qCritical() << socket->errorString();
                                canRun=false;
                                emit someError();
                                break;
                            }
                        }
                    }
                    mutexOut->unlock();
                }
            }            
        }

        mutexIn->lock();

        socket->close();
        socket->deleteLater();

        mutexIn->unlock();

        m_mutexEventLoop.lock();
        eventLoop->deleteLater();
        eventLoop=nullptr;
        m_mutexEventLoop.unlock();

        emit closed();

        qInfo() << "Client" << name() << " done " << QThread::currentThread();
}

void Client::soketError(QAbstractSocket::SocketError socketError)
{
    qInfo() << "Client Error:" << socketError << " ";// << socket->errorString(); // socket can already be deleted.

    canRun=false;

    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();

    //emit someError();
}

void Client::socketDisconnected()
{
    qInfo() << "Client Disconnected:" << name().toStdString().c_str() << " ";// << socket->errorString(); // socket can already be deleted.

    canRun=false;

    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();

    //emit someError();
}

void Client::socketReadyRead()
{    
    mutexIn->lock();
    if (socket->state()==QTcpSocket::SocketState::ConnectedState && canRun)
    {
        while(socket->bytesAvailable() != 0)
        {
            if (firstDataIn)
            {
                firstDataIn=false;
                QByteArray data=socket->readAll();
                dataIn->append(data);
                QString s(data);
                qInfo() << "FIRST TELEGRAM: " << s;
                size_t a = s.toStdString().find(QString(" HTTP/").toStdString());
                if (a>7)
                {
                    name(s.mid(7,static_cast<int>(a)-7));
                }
                continue;
            }
            dataIn->append(socket->readAll());
        }
    }
    if (dataIn->size()>0)
        emit dataRead(); // before mutex unlock?
    mutexIn->unlock();
}
