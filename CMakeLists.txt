# 要求 CMake 版本；Qt6 的 CMake API 建议使用较新的 CMake。
cmake_minimum_required(VERSION 3.21)

# 定义工程名、版本号、启用的语言。
project(qcom VERSION 0.1.0 LANGUAGES C CXX )

# 指定 C++17；Qt 6.5 要求至少 C++17。
set(CMAKE_CXX_STANDARD 17)
# 强制使用上面指定的 C++ 标准，避免编译器静默降级。
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 导出 compile_commands.json，方便 clangd / VS Code 等工具做代码跳转。
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GNU_JCOM_ENABLE_QT_DEPLOYMENT "Bundle Qt runtime dependencies during install." OFF)

include(GNUInstallDirs)
include(CTest)

# 旧的普通 C++ 可执行文件入口，当前工程改用 Qt 的 qt_add_executable。
# add_executable(quectel_com src/main.cpp)

# 先查找并加载 Qt6 组件；否则下面的 qt_standard_project_setup 等命令不存在。
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 SerialPort)

if(WIN32 AND MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR})
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    include(InstallRequiredSystemLibraries)
endif()

# 使用 Qt 6.5 推荐的 QML 模块目录策略，避免 QTP0004 开发者 warning。
qt_policy(SET QTP0004 NEW)

# 启用 Qt 标准工程设置，例如 AUTOMOC / AUTORCC 等 Qt 构建辅助逻辑。
qt_standard_project_setup(REQUIRES 6.5)


# 创建 Qt 可执行目标 qcom，入口源码是 src/main.cpp。
qt_add_executable(qcom
    src/main.cpp
    src/McuReportModel.h
    src/McuReportModel.cpp
    src/SerialPortController.h
    src/SerialPortController.cpp
    src/SerialPortControllerCommands.cpp
    src/SerialPortControllerTerminal.cpp
    protocol_c/src/gnu_soc_proto.c
)

set_target_properties(qcom PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 把 QML 文件打包进 qcom 的 Qt 资源系统，并声明 QML 模块名 Qcom。
qt_add_qml_module(qcom
    URI Qcom
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/MiniField.qml
        qml/ProtocolCommandItem.qml
        qml/TerminalPane.qml
        qml/TerminalPasswordDialog.qml
)

# 把源码根目录传入 C++，让 main.cpp 能定位 QML 文件。
target_compile_definitions(qcom PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# 链接 Qt Quick/QML 运行所需库。
target_link_libraries(qcom
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickControls2
        Qt6::SerialPort
)

# 头文件路径：让 src/ 能找到 protocol_c/include/
target_include_directories(qcom PRIVATE protocol_c/include)

install(TARGETS qcom
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(GNU_JCOM_ENABLE_QT_DEPLOYMENT AND (WIN32 OR APPLE))
    qt_generate_deploy_qml_app_script(
        TARGET qcom
        OUTPUT_SCRIPT qcom_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${qcom_deploy_script})
endif()
