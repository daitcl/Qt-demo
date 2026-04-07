#include "serialworker.h"
#include <QDebug>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
    , m_delayTimer(new QTimer(this))
{
    m_delayTimer->setSingleShot(true);
    connect(m_serial, &QSerialPort::readyRead, this, &SerialWorker::onReadyRead);
    connect(m_delayTimer, &QTimer::timeout, this, &SerialWorker::onTimeout);
}

SerialWorker::~SerialWorker()
{
    // m_serial 和 m_delayTimer 由对象树自动清理，无需手动 delete
}

void SerialWorker::open(const SerialConfig &config)
{
    if (m_isOpen) {
        close();
    }

    applyConfig(config);
    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("打开串口失败: %1").arg(m_serial->errorString()));
        return;
    }
    m_isOpen = true;
    emit opened();

}

void SerialWorker::close()
{
    if (m_serial->isOpen()) {
        m_serial->clear();
        m_serial->close();
    }
    m_isOpen = false;
    emit closed();
}

void SerialWorker::sendData(const QByteArray &data)
{
    if (!m_isOpen) {
        emit errorOccurred(tr("串口未打开，无法发送数据"));
        return;
    }
    if (data.isEmpty()) return;

    QByteArray outData = data;

    qint64 written = m_serial->write(outData);
    if (written != outData.size()) {
        emit errorOccurred(tr("数据发送不完整"));
    }
}

void SerialWorker::setReceiveDelay(int ms)
{
    m_receiveDelay = ms;
}

void SerialWorker::applyConfig(const SerialConfig &config)
{
    m_serial->setPortName(config.portName);
    m_serial->setBaudRate(config.baudRate);
    m_serial->setDataBits(config.dataBits);
    m_serial->setParity(config.parity);
    m_serial->setStopBits(config.stopBits);
    m_serial->setFlowControl(config.flowControl);
}

void SerialWorker::onReadyRead()
{
    QByteArray chunk = m_serial->readAll();
    m_buffer.append(chunk);

    // 防止缓冲区无限增长（10MB 上限）
    constexpr int MAX_BUFFER_SIZE = 10 * 1024 * 1024; // 10 MiB
    if (m_buffer.size() > MAX_BUFFER_SIZE) {
        flushBuffer();
    }

    if (m_receiveDelay == 0) {
        flushBuffer(); // 立即发送
    } else {
        m_delayTimer->start(m_receiveDelay); // 重启定时器
    }
}

void SerialWorker::onTimeout()
{
    flushBuffer();
}

void SerialWorker::flushBuffer()
{
    if (m_buffer.isEmpty()) return;
    emit dataReceived(m_buffer);
    m_buffer.clear();
}