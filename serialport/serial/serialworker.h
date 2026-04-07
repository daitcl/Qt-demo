#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>

// 串口配置结构体
struct SerialConfig {
    QString portName;
    qint32 baudRate = 115200;
    QSerialPort::DataBits dataBits = QSerialPort::Data8;
    QSerialPort::Parity parity = QSerialPort::NoParity;
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;

};

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

public slots:
    void open(const SerialConfig &config);
    void close();
    void sendData(const QByteArray &data);
    void setReceiveDelay(int ms);

signals:
    void opened();                          // 打开成功
    void closed();                          // 关闭成功
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void configApplied(const QString &configStr);

private slots:
    void onReadyRead();
    void onTimeout();

private:
    QSerialPort *m_serial = nullptr;
    QTimer *m_delayTimer = nullptr;
    QByteArray m_buffer;
    int m_receiveDelay = 100;   // 默认100ms
    bool m_isOpen = false;

    void applyConfig(const SerialConfig &config);
    void flushBuffer();
};

#endif // SERIALWORKER_H