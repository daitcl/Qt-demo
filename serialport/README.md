# Qt6 多线程串口通信 Demo

一个基于 **Qt6** 的多线程串口通信示例项目，展示如何将串口操作封装到独立工作线程，并通过信号槽机制实现线程安全、高内聚低耦合的串口通信架构。同时附带一个简易的串口调试助手界面。

## 功能特点

- **多线程隔离**：串口读写完全运行在独立线程，不阻塞 GUI
- **线程安全接口**：通过 `SerialManager` 提供信号驱动的异步接口，可在任意线程安全调用
- **智能接收聚合**：利用定时器延迟聚合数据包，减少界面刷新频率，解决粘包问题
- **完整状态反馈**：打开/关闭/错误等信号实时通知上层
- **串口调试助手**：
  - 动态检测系统可用串口
  - 支持常用波特率、数据位、校验位、停止位配置
  - 文本与十六进制两种发送/显示模式，实时格式化
  - 时间戳、自动换行等可选显示功能
  - 定时发送功能
  - 清空收发区域

## 架构设计

```text
┌─────────────┐      signals/slots      ┌─────────────────┐
│   Widget     │ ◄──────────────────────► │ SerialManager    │
│  (GUI 线程)  │                         │ (主线程，管理类)  │
└─────────────┘                         └────────┬────────┘
                                                  │
                                        moveToThread + 信号槽连接
                                                  │
                                        ┌─────────▼────────┐
                                        │  SerialWorker    │
                                        │ (工作线程)        │
                                        │  QSerialPort     │
                                        │  QTimer (聚合)   │
                                        └──────────────────┘
```



- **SerialWorker**：封装 `QSerialPort` 全部操作，包括打开/关闭、读写、错误处理及接收延迟聚合。
- **SerialManager**：负责将 `SerialWorker` 移至 `QThread`，对外提供异步接口，并转发工作对象的状态信号。
- **Widget**：简易调试助手界面，展示如何使用 `SerialManager` 进行串口控制及数据收发。

## 构建与运行

### 环境要求

- Qt 6.2 或更高版本
- 支持 C++17 的编译器（如 MSVC 2019、GCC 9、Clang 10 以上）
- CMake 3.16+（推荐）或 qmake

### 构建步骤

```bash
# 克隆仓库（假设仓库名为 Qt-demo，本 Demo 位于 serial 子目录）
git clone https://github.com/daitcl/Qt-demo.git
cd Qt-demo/serial

# 使用 CMake 构建
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build .

# 或使用 qmake
qmake6 serial.pro
make   # 或 nmake / jom
```

运行生成的 `serial_demo` 可执行文件。

### 依赖模块

- Qt6::Core
- Qt6::Widgets
- Qt6::SerialPort

请确保 Qt6 安装时包含了 **Qt Serial Port** 模块。

## 使用说明

1. 启动程序后，点击 **扫描** 按钮或自动检测可用串口，选择目标串口。
2. 配置波特率、数据位等参数（默认 9600/8/N/1）。
3. 点击 **打开串口**，按钮变为 **关闭串口** 即表示连接成功。
4. 在发送区输入文本，或勾选 **Hex 发送** 输入十六进制字节（支持空格分隔）。
5. 点击 **发送**，数据将通过工作线程发出；接收到的数据会显示在接收区。
6. 可启用 **时间戳**、**自动换行**、**Hex 显示** 等选项。
7. 勾选 **定时发送** 并设置间隔，实现周期发送。

## 目录结构（示例）

```text
serial/
├── CMakeLists.txt          // CMake 构建脚本
├── serial.pro              // qmake 工程文件（备选）
├── src/
│   ├── main.cpp
│   ├── widget.h / .cpp     // 主界面
│   ├── serialworker.h / .cpp
│   └── serialmanager.h / .cpp
├── ui/
│   └── widget.ui           // Qt Designer 界面文件
└── README.md
```

## 核心代码片段

### 接收聚合（SerialWorker）

```cpp
void SerialWorker::onReadyRead() {
    m_buffer.append(m_serial->readAll());
    if (m_buffer.size() > MAX_BUFFER_SIZE) flushBuffer();
    if (m_receiveDelay == 0) flushBuffer();
    else m_delayTimer->start(m_receiveDelay);
}

void SerialWorker::onTimeout() { flushBuffer(); }

void SerialWorker::flushBuffer() {
    if (m_buffer.isEmpty()) return;
    emit dataReceived(m_buffer);
    m_buffer.clear();
}
```

### 异步接口（SerialManager）

```cpp
void SerialManager::open(const SerialConfig &config) {
    emit requestOpen(config); // 跨线程信号触发 SerialWorker::open
}

void SerialManager::sendData(const QByteArray &data) {
    emit requestSendData(data);
}
```

## 注意事项

- 关闭程序时，`SerialManager` 析构会安全退出工作线程并释放资源。
- 十六进制发送模式下，请确保输入偶数个有效十六进制字符（可含空格）。
- 如果运行时提示找不到串口库，请检查 Qt Serial Port 模块是否正确安装。

## 许可证

>
>协议：[MIT](../License) — 允许任何人自由使用、复制、修改、合并、发布、分发、再授权和/或销售软件的副本，前提是保留原始版权声明和许可声明。
>
>免责声明：软件按“原样”提供，不附带任何明示或暗示的保证，包括但不限于适销性、特定用途适用性和非侵权的保证。在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，无论是在合同诉讼、侵权诉讼或其他诉讼中，由于软件或软件的使用或其他交易引起的。
>

## 来找我

- **CSDN**：[daitcl的博客](https://blog.csdn.net/qq_39538318) — csdn、技术文章和教程
- **GitHub**：[@daitcl](https://github.com/daitcl) —  个人代码仓库（**gitee/gitcode**同名同步）
- **爱发电**：[爱发电主页](https://ifdian.net/a/daitcc) — 欢迎支持、分享或合作
- **个人主页**：[daitcc.top](https://www.daitcc.top) — 个人主页、技术文章和教程
- **邮箱**：daitcctop@163.com — 欢迎交流、指正或闲聊
- **微信公众号**：扫一扫下方二维码，关注公众号获取更新推送  
  <img src="https://raw.gitcode.com/daitcl/picgo/raw/main/wechat_qrcode.jpg" alt="微信公众号二维码" width="150" />

## 支持与鼓励

>
> 如果代码对你有所启发，或者单纯想请我喝杯咖啡，可以通过下方的 **爱发电** 或者 **支付宝/微信赞赏码** 来支持我创作。每一份鼓励都是我持续创作的动力。
>

- **爱发电**：[赞助作者](https://ifdian.net/a/daitcc) — 点击链接即可支持  
  <img src="https://raw.gitcode.com/daitcl/picgo/raw/main/ifdian.png" alt="爱发电" width="150" />
  
- **支付宝赞赏**：扫一扫下方二维码  
  <img src="https://raw.gitcode.com/daitcl/picgo/raw/main/alipay.png" alt="支付宝收款码" width="150" />      

- **微信赞赏**：扫一扫下方二维码  
  <img src="https://raw.gitcode.com/daitcl/picgo/raw/main/wechatpay.png" alt="微信赞赏码" width="150" />

>
>“独行快，众行远。” 感谢你的每一次阅读、点赞和分享。  
>期待在技术道路上，与你并肩前行。