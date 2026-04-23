cmake_minimum_required(VERSION 3.21)

project(gnu_jcom VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GNU_JCOM_ENABLE_QT_DEPLOYMENT "Bundle Qt runtime dependencies during install." OFF)

include(GNUInstallDirs)
include(CTest)

if(WIN32 AND MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR})
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    include(InstallRequiredSystemLibraries)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml SerialPort Charts)

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
endif()

qt_policy(SET QTP0004 NEW)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(gnu_jcom
    src/main.cpp
    src/appcontroller.cpp
    src/logmodel.cpp
    src/packetfieldmodel.cpp
    src/packetschema.cpp
    src/payloadcodec.cpp
    src/sendlistmodel.cpp
)

target_sources(gnu_jcom PRIVATE
    src/appcontroller.h
    src/logmodel.h
    src/packetfieldmodel.h
    src/packetschema.h
    src/payloadcodec.h
    src/sendlistmodel.h
)

qt_add_qml_module(gnu_jcom
    URI GnuJCom
    VERSION 1.0
    IMPORTS
        QtCharts
    QML_FILES
        qml/Main.qml
)

target_link_libraries(gnu_jcom PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::SerialPort
    Qt6::Charts
)

target_include_directories(gnu_jcom PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS gnu_jcom
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    examples/linear_demo_schema.json
    examples/soc_proto_mcu_report_schema.json
    examples/sine_demo_schema.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/gnu_jcom/examples
)

qt_add_executable(gnu_jcom_demo_device
    tools/demo_device_main.cpp
    src/packetschema.cpp
)

target_sources(gnu_jcom_demo_device PRIVATE
    src/packetschema.h
)

target_include_directories(gnu_jcom_demo_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(gnu_jcom_demo_device PRIVATE
    Qt6::Core
    Qt6::SerialPort
)

install(TARGETS gnu_jcom_demo_device
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(GNU_JCOM_ENABLE_QT_DEPLOYMENT)
    qt_generate_deploy_qml_app_script(
        TARGET gnu_jcom
        OUTPUT_SCRIPT gnu_jcom_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${gnu_jcom_deploy_script})

    qt_generate_deploy_app_script(
        TARGET gnu_jcom_demo_device
        OUTPUT_SCRIPT gnu_jcom_demo_device_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${gnu_jcom_demo_device_deploy_script})
endif()

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

    qt_add_executable(gnu_jcom_packetschema_test
        tests/packetschema_test.cpp
        src/packetschema.cpp
    )

    target_sources(gnu_jcom_packetschema_test PRIVATE
        src/packetschema.h
    )

    target_include_directories(gnu_jcom_packetschema_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    target_link_libraries(gnu_jcom_packetschema_test PRIVATE
        Qt6::Core
        Qt6::Test
    )

    add_test(NAME packetschema COMMAND gnu_jcom_packetschema_test)
endif()
