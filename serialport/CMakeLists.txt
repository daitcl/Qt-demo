cmake_minimum_required(VERSION 3.16)
project(serialport VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt 模块
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort)
# 如果找不到 Qt6，回退到 Qt5
if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets SerialPort)
endif()

# 源文件列表
set(SOURCES
    main.cpp
    serial/serialmanager.cpp
    serial/serialworker.cpp
    widget.cpp
)

set(HEADERS
    serial/serialmanager.h
    serial/serialworker.h
    widget.h
)

set(FORMS
    widget.ui
)

# 添加可执行文件
add_executable(serialport ${SOURCES} ${HEADERS} ${FORMS})

# 处理翻译文件
set(TS_FILES serialport_zh_CN.ts)
qt_add_translations(serialport
    TS_FILES ${TS_FILES}
)
# 启用 lrelease 并嵌入翻译
if(TARGET serialport_update_translations)
    add_dependencies(serialport serialport_update_translations)
endif()

# 链接 Qt 库
target_link_libraries(serialport
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::SerialPort
)

# 部署规则 (Windows 下可选)
if(WIN32)
    set(INSTALL_DEPLOY_TARGETS serialport)
    include(GNUInstallDirs)
    install(TARGETS serialport RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()