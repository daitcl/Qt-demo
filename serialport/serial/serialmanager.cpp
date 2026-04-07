#include "serialmanager.h"

SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
{
    m_worker = new SerialWorker();
    m_worker->moveToThread(&m_workerThread);

    // 连接请求信号到工作对象的槽
    connect(this, &SerialManager::requestOpen, m_worker, &SerialWorker::open);
    connect(this, &SerialManager::requestClose, m_worker, &SerialWorker::close);
    connect(this, &SerialManager::requestSendData, m_worker, &SerialWorker::sendData);
    connect(this, &SerialManager::requestSetReceiveDelay, m_worker, &SerialWorker::setReceiveDelay);

    // 连接工作对象的信号到转发信号
    connect(m_worker, &SerialWorker::opened, this, &SerialManager::opened);
    connect(m_worker, &SerialWorker::closed, this, &SerialManager::closed);
    connect(m_worker, &SerialWorker::errorOccurred, this, &SerialManager::errorOccurred);
    connect(m_worker, &SerialWorker::dataReceived, this, &SerialManager::dataReceived);
    connect(m_worker, &SerialWorker::configApplied, this, &SerialManager::configApplied);

    m_workerThread.start();
}

SerialManager::~SerialManager()
{
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_worker;
}

void SerialManager::open(const SerialConfig &config)
{
    emit requestOpen(config);
}

void SerialManager::close()
{
    emit requestClose();
}

void SerialManager::sendData(const QByteArray &data)
{
    emit requestSendData(data);
}

void SerialManager::setReceiveDelay(int ms)
{
    emit requestSetReceiveDelay(ms);
}