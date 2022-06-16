cmake_minimum_required(VERSION 3.16)

project(QNetPd VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS Quick Network REQUIRED)

qt_add_executable(QNetPd
    main.cpp
    osc.cpp
)

qt_add_qml_module(QNetPd
    URI QNetPd
    VERSION 1.0
    QML_FILES main.qml
)

set_target_properties(QNetPd PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.netpd
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(QNetPd
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(QNetPd
    PRIVATE Qt6::Quick)
