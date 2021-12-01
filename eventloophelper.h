#ifndef EVENTLOOPHELPER_H
#define EVENTLOOPHELPER_H

#include <QObject>
#include <QEventLoop>
#include <QMutex>

class EventLoopHelper : public QObject
{
    Q_OBJECT

public:
    EventLoopHelper(QObject *parent, int _count);

    void exec();
    int count();

public slots:
    void inc();

private:

    QEventLoop eventloop;
    int m_count;
    QMutex m;

};

#endif // EVENTLOOPHELPER_H
