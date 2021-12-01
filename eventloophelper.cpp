#include "eventloophelper.h"

#include <QDebug>
#include <QMutexLocker>

EventLoopHelper::EventLoopHelper(QObject *parent, int _count)
    : QObject(parent)
    , eventloop(this)
    , m_count(_count)
{

}

void EventLoopHelper::exec()
{
    eventloop.exec();

    if (count()>0)
    {
        qInfo() << "COUNT > 0, but EXEC FINISHED!";
    }
}

int EventLoopHelper::count()
{
    QMutexLocker lock(&m);
    return m_count;
}

void EventLoopHelper::inc()
{
    QMutexLocker lock(&m);
    m_count--;

    if (m_count<1)
    {
        eventloop.quit();
    }
}
