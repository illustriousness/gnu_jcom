cmake_minimum_required(VERSION 3.21)

project(gnu_jcom VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml SerialPort)

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
endif()

qt_policy(SET QTP0004 NEW)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(gnu_jcom
    src/main.cpp
    src/appcontroller.cpp
    src/logmodel.cpp
    src/payloadcodec.cpp
    src/sendlistmodel.cpp
)

target_sources(gnu_jcom PRIVATE
    src/appcontroller.h
    src/logmodel.h
    src/payloadcodec.h
    src/sendlistmodel.h
)

qt_add_qml_module(gnu_jcom
    URI GnuJCom
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(gnu_jcom PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::SerialPort
)

target_include_directories(gnu_jcom PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS gnu_jcom
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
    qt_add_executable(gnu_jcom_payloadcodec_test
        tests/payloadcodec_test.cpp
        src/payloadcodec.cpp
        src/payloadcodec.h
    )

    target_include_directories(gnu_jcom_payloadcodec_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    target_link_libraries(gnu_jcom_payloadcodec_test PRIVATE
        Qt6::Core
        Qt6::Test
    )

    add_test(NAME payloadcodec COMMAND gnu_jcom_payloadcodec_test)
endif()
