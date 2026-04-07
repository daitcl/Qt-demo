#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QThread>
#include "serialworker.h"

class SerialManager : public QObject
{
    Q_OBJECT
public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();

    // 对外公开接口（可从任意线程安全调用）
    Q_INVOKABLE void open(const SerialConfig &config);
    Q_INVOKABLE void close();
    Q_INVOKABLE void sendData(const QByteArray &data);
    Q_INVOKABLE void setReceiveDelay(int ms);

signals:
    // 请求信号（用于转发给 SerialWorker）
    void requestOpen(const SerialConfig &config);
    void requestClose();
    void requestSendData(const QByteArray &data);
    void requestSetReceiveDelay(int ms);

    // 转发信号（来自 SerialWorker 的事件）
    void opened();
    void closed();
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void configApplied(const QString &configStr);

private:
    QThread m_workerThread;
    SerialWorker *m_worker = nullptr;
};

#endif // SERIALMANAGER_H