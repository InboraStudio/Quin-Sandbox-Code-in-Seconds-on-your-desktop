cmake_minimum_required(VERSION 3.22)
project(QuinSandbox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/CodeEditor.cpp
    src/OutputPanel.cpp
    src/Runner.cpp
    src/LanguageConfig.cpp
    src/SyntaxHighlighter.cpp
    src/TitleBar.cpp
)

set(HEADERS
    src/MainWindow.h
    src/CodeEditor.h
    src/OutputPanel.h
    src/Runner.h
    src/LanguageConfig.h
    src/SyntaxHighlighter.h
    src/TitleBar.h
)

set(RESOURCES
    resources/resources.qrc
)

# (App icon for exe removed due to MinGW windres bug with spaces in paths)

qt_add_executable(QuinSandbox
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    ${APP_ICON}
)

target_link_libraries(QuinSandbox PRIVATE Qt6::Widgets)

if(APPLE)
    set_target_properties(QuinSandbox PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.quinsandbox.app
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
endif()

if(WIN32)
    set_target_properties(QuinSandbox PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
