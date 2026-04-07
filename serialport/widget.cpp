#include "widget.h"
#include "ui_widget.h"
#include <QSerialPortInfo>
#include <QDateTime>
#include <QMessageBox>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , manager(new SerialManager(this))
    , m_sendTimer(new QTimer(this))
{
    ui->setupUi(this);

    // 步骤 1: 初始化参数框（波特率、数据位、校验位、停止位）
    initUIConfig();
    initCheckbox();          // 初始化 Hex 模式相关信号槽（步骤 10 的一部分）

    // 步骤 2: 获取可用串口列表
    getportInfo();

    // 步骤 3/4: 打开/关闭串口按钮
    connect(ui->OpenComBtn, &QPushButton::clicked, this, &Widget::onOpenCloseButtonClicked);

    // 步骤 5: 清除接收界面
    connect(ui->ClearRxBtn, &QPushButton::clicked, this, [=]() { ui->RxEdit->clear(); });

    // 步骤 6: 发送数据按钮
    connect(ui->TxBtn, &QPushButton::clicked, this, &Widget::onSendButtonClicked);

    // 步骤 7: 清除发送界面
    connect(ui->ClearTxBtn, &QPushButton::clicked, this, [=]() { ui->TxEdit->clear(); });

    // 步骤 8: 定时发送
    m_sendTimer->setSingleShot(false);
    connect(m_sendTimer, &QTimer::timeout, this, &Widget::onTimeoutSend);
    connect(ui->TRBox, &QCheckBox::toggled, this, &Widget::onTRBoxToggled);
    connect(ui->TVBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        if (m_sendTimer->isActive())
            m_sendTimer->setInterval(value);
    });

    // 串口管理器信号连接
    connect(manager, &SerialManager::opened, this, &Widget::handleOpened);
    connect(manager, &SerialManager::closed, this, &Widget::handleClosed);
    connect(manager, &SerialManager::errorOccurred, this, &Widget::handleErrorOccurred);
    connect(manager, &SerialManager::dataReceived, this, &Widget::handleDataReceived);
}

Widget::~Widget()
{
    delete ui;
}

// ==================== 步骤 1: 初始化参数框 ====================
void Widget::initUIConfig()
{
    // 波特率
    ui->comBaudRate->addItems({"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"});
    ui->comBaudRate->setEditable(true);
    ui->comBaudRate->setValidator(new QIntValidator(0, 1000000, this));
    ui->comBaudRate->setCurrentIndex(3);   // 默认 9600

    // 数据位
    ui->comDataBits->addItems({"5", "6", "7", "8"});
    ui->comDataBits->setCurrentIndex(3);   // 默认 8

    // 校验位
    ui->comParity->addItems({"Even", "Mark", "None", "Odd", "Space"});
    ui->comParity->setCurrentIndex(2);     // 默认 None

    // 停止位
    ui->comStopBits->addItems({"1", "1.5", "2"});
    ui->comStopBits->setCurrentIndex(0);   // 默认 1
}

// ==================== 步骤 2: 获取可用串口 ====================
void Widget::getportInfo()
{
    ui->comPortName->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        ui->comPortName->addItem("无可用串口");
        ui->OpenComBtn->setEnabled(false);
        return;
    }
    ui->OpenComBtn->setEnabled(true);
    for (const QSerialPortInfo &qspinfo : ports)   // 同样安全
        ui->comPortName->addItem(qspinfo.portName());

}

// ==================== 步骤 3/4: 打开/关闭串口 ====================
void Widget::onOpenCloseButtonClicked()
{
    if (!isSerialOpen) {
        // 准备配置
        SerialConfig config;
        config.portName = ui->comPortName->currentText();
        config.baudRate = ui->comBaudRate->currentText().toInt();

        // 数据位映射
        switch (ui->comDataBits->currentIndex()) {
        case 0: config.dataBits = QSerialPort::Data5; break;
        case 1: config.dataBits = QSerialPort::Data6; break;
        case 2: config.dataBits = QSerialPort::Data7; break;
        default: config.dataBits = QSerialPort::Data8; break;
        }
        // 校验位映射
        switch (ui->comParity->currentIndex()) {
        case 0: config.parity = QSerialPort::EvenParity; break;
        case 1: config.parity = QSerialPort::MarkParity; break;
        case 2: config.parity = QSerialPort::NoParity; break;
        case 3: config.parity = QSerialPort::OddParity; break;
        default: config.parity = QSerialPort::SpaceParity; break;
        }
        // 停止位映射
        switch (ui->comStopBits->currentIndex()) {
        case 0: config.stopBits = QSerialPort::OneStop; break;
        case 1: config.stopBits = QSerialPort::OneAndHalfStop; break;
        default: config.stopBits = QSerialPort::TwoStop; break;
        }

        manager->open(config);
        ui->OpenComBtn->setEnabled(false);
        ui->OpenComBtn->setText("正在打开...");
    } else {
        manager->close();   // 界面状态在 handleClosed 中更新
    }
}

void Widget::handleOpened()
{
    isSerialOpen = true;
    ui->OpenComBtn->setEnabled(true);
    ui->OpenComBtn->setText("关闭串口");
}

void Widget::handleClosed()
{
    isSerialOpen = false;
    ui->OpenComBtn->setEnabled(true);
    ui->OpenComBtn->setText("打开串口");
}

void Widget::handleErrorOccurred(const QString &error)
{
    QMessageBox::critical(this, "串口错误", error);
    isSerialOpen = false;
    ui->OpenComBtn->setEnabled(true);
    ui->OpenComBtn->setText("打开串口");
}

// ==================== 步骤 6: 发送数据 ====================
void Widget::onSendButtonClicked()
{
    QString text = ui->TxEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    bool hexFlag = ui->HexTxBox->isChecked();
    QByteArray data;

    if (hexFlag) {
        QString hexStr = text;
        hexStr.remove(' ');
        if (hexStr.length() % 2 != 0) {
            QMessageBox::warning(this, "错误", "十六进制数据必须为偶数个字符，请补零后重试");
            return;
        }
        data = QByteArray::fromHex(hexStr.toUtf8());
        if (data.isEmpty() && !hexStr.isEmpty()) {
            QMessageBox::warning(this, "错误", "无效的十六进制数据");
            return;
        }
    } else {
        data = text.toUtf8();
    }
    manager->sendData(data);   // 第二个参数默认为 false，因为 data 已是原始字节
}

// ==================== 步骤 8: 定时发送 ====================
void Widget::onTRBoxToggled(bool checked)
{
    if (checked) {
        int interval = ui->TVBox->value();
        if (interval <= 0) {
            QMessageBox::warning(this, "警告", "定时发送间隔必须大于0");
            ui->TRBox->setChecked(false);
            return;
        }
        m_sendTimer->setInterval(interval);
        m_sendTimer->start();
    } else {
        m_sendTimer->stop();
    }
}

void Widget::onTimeoutSend()
{
    onSendButtonClicked();   // 复用发送逻辑
}

// ==================== 步骤 9: 接收时的 Hex 模式和自动换行 ====================
void Widget::handleDataReceived(const QByteArray &data)
{
    if (data.isEmpty()) return;

    // 准备显示内容
    QString display;
    if (ui->HexRxBox->isChecked()) {
        display = data.toHex(' ').toUpper();
        if (!display.isEmpty()) display += ' ';   // 末尾加空格便于分隔
    } else {
        display = QString::fromUtf8(data);
        if (display.isEmpty()) display = QString::fromLatin1(data);
    }

    ui->RxEdit->moveCursor(QTextCursor::End);

    // 时间戳
    if (ui->TimeBox->isChecked()) {
        QString timestamp = QDateTime::currentDateTime().toString("[yyyy/MM/dd hh:mm:ss] ");
        ui->RxEdit->insertPlainText(timestamp);
    }

    ui->RxEdit->insertPlainText(display);

    // 自动换行
    if (ui->AWBox->isChecked())
        ui->RxEdit->insertPlainText("\n");

    ui->RxEdit->moveCursor(QTextCursor::End);
}

// ==================== 步骤 10: 发送时的 Hex 模式 ====================
void Widget::initCheckbox()
{
    // 切换 Hex 发送模式
    connect(ui->HexTxBox, &QCheckBox::checkStateChanged, this, [=](Qt::CheckState state) {
        if (state == Qt::Checked)
            convertTextToHex();   // 文本 -> Hex
        else
            convertHexToText();   // Hex -> 文本
    });

    // Hex 模式下实时格式化（添加空格、过滤非法字符）
    connect(ui->TxEdit, &QTextEdit::textChanged, this, [=]() {
        if (ui->HexTxBox->isChecked())
            formatTxEditForHex();
    });
}

void Widget::formatTxEditForHex()
{
    if (!ui->HexTxBox->isChecked()) return;

    QTextCursor cursor = ui->TxEdit->textCursor();
    int cursorPos = cursor.position();
    QString oldText = ui->TxEdit->toPlainText();

    // 统计光标前有效十六进制字符数
    int validCount = 0;
    for (int i = 0; i < cursorPos && i < oldText.length(); ++i) {
        QChar ch = oldText.at(i);
        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
            ++validCount;
    }

    // 提取所有有效十六进制字符并转大写
    QString hexStr;
    for (QChar ch : std::as_const(oldText)) {
        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
            hexStr.append(ch.toUpper());
    }

    // 每两个字符加一个空格（不补零）
    QStringList parts;
    for (int i = 0; i < hexStr.length(); i += 2)
        parts << hexStr.mid(i, 2);
    QString newText = parts.join(' ');

    if (oldText != newText) {
        ui->TxEdit->blockSignals(true);
        ui->TxEdit->setPlainText(newText);

        int newPos = validCount + (validCount / 2);
        newPos = qMin(newPos, newText.length());
        QTextCursor newCursor = ui->TxEdit->textCursor();
        newCursor.setPosition(newPos);
        ui->TxEdit->setTextCursor(newCursor);

        ui->TxEdit->blockSignals(false);
    }
}

void Widget::convertTextToHex()
{
    QString plainText = ui->TxEdit->toPlainText();
    QByteArray bytes = plainText.toLocal8Bit();   // 使用系统默认编码
    QString hexStr = bytes.toHex(' ').toUpper();
    ui->TxEdit->blockSignals(true);
    ui->TxEdit->setPlainText(hexStr);
    ui->TxEdit->blockSignals(false);
}

void Widget::convertHexToText()
{
    QString hexText = ui->TxEdit->toPlainText();
    QString pureHex = hexText;
    pureHex.remove(' ');
    QByteArray bytes = QByteArray::fromHex(pureHex.toUtf8());
    if (bytes.isEmpty() && !pureHex.isEmpty()) {
        QMessageBox::warning(this, "错误", "无效的十六进制数据，无法转换为文本");
        return;
    }
    QString plainText = QString::fromLocal8Bit(bytes);
    if (plainText.isEmpty() && !bytes.isEmpty())
        plainText = QString::fromLatin1(bytes);
    ui->TxEdit->blockSignals(true);
    ui->TxEdit->setPlainText(plainText);
    ui->TxEdit->blockSignals(false);
}