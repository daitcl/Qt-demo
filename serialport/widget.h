#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

#include "serial/serialmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private slots:
    // 步骤 3/4: 打开/关闭串口
    void onOpenCloseButtonClicked();
    // 步骤 6: 发送数据
    void onSendButtonClicked();
    // 步骤 8: 定时发送相关
    void onTRBoxToggled(bool checked);
    void onTimeoutSend();
    // 步骤 9: 接收数据
    void handleDataReceived(const QByteArray &data);
    // 串口状态变化
    void handleOpened();
    void handleClosed();
    void handleErrorOccurred(const QString &error);

private:
    // 步骤 1: 初始化参数框
    void initUIConfig();
    // 步骤 2: 获取可用串口列表
    void getportInfo();
    // 步骤 10: 发送时的 Hex 模式相关
    void initCheckbox();
    void formatTxEditForHex();
    void convertTextToHex();
    void convertHexToText();

private:
    Ui::Widget *ui;
    SerialManager *manager = nullptr;
    QTimer *m_sendTimer = nullptr;
    bool isSerialOpen = false;
};

#endif // WIDGET_H