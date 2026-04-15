cmake_minimum_required(VERSION 3.16)

project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    set(MAPLIBRE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Android/arm64-v8a")
else()
    set(MAPLIBRE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux")
endif()

if(ANDROID)
    list(APPEND CMAKE_PREFIX_PATH "${MAPLIBRE_SDK_DIR}/lib/cmake/QMapLibre")
else()
    list(APPEND CMAKE_PREFIX_PATH "${MAPLIBRE_SDK_DIR}/lib64/cmake/QMapLibre")
endif()

find_package(QMapLibre REQUIRED COMPONENTS Core Location)
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Positioning Location)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appuntitled
    main.cpp
    hxgisserver.h
    hxgisserver.cpp
)

if(ANDROID)
    target_compile_definitions(appuntitled PRIVATE IS_ANDROID)
else()
    set(SDK_PLUGINS_PATH "${MAPLIBRE_SDK_DIR}/plugins")
    set(SDK_QML_PATH "${MAPLIBRE_SDK_DIR}/qml")
    target_compile_definitions(appuntitled PRIVATE
        SDK_PLUGINS_PATH="${SDK_PLUGINS_PATH}"
        SDK_QML_PATH="${SDK_QML_PATH}"
    )
endif()

if(NOT ANDROID)
    set(HXGISSERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-linux/x86_64")
    set(HXGISSERVER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-linux")
    target_include_directories(appuntitled PRIVATE ${HXGISSERVER_INCLUDE_DIR})
    target_link_directories(appuntitled PRIVATE ${HXGISSERVER_DIR})
    target_link_libraries(appuntitled PRIVATE plugin-HXGISServer)
    set_target_properties(appuntitled PROPERTIES BUILD_RPATH "${HXGISSERVER_DIR}")
endif()

qt_add_qml_module(appuntitled
    URI untitled
    QML_FILES Main.qml
)

target_link_libraries(appuntitled PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Positioning
    Qt6::Location
    QMapLibre::Core
    QMapLibre::Location
)

qmaplibre_location_setup_plugins(appuntitled)

if(NOT ANDROID)
    set_target_properties(appuntitled PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
    )
endif()
