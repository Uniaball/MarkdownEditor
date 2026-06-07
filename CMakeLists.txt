cmake_minimum_required(VERSION 3.20)
project(MarkdownEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(MarkdownEditor
    src/main.cpp
    src/MainWindow.cpp
    src/MarkdownEditor.cpp
    src/FormatToolBar.cpp
    src/PreviewWidget.cpp
    src/MarkdownHighlighter.cpp
    src/parser/Core.cpp
    src/parser/HtmlGen.cpp
    src/parser/CodeHighlight.cpp
    src/parser/FoldBlock.cpp
    src/parser/InlineProc.cpp
    src/parser/Utils.cpp
    src/parser/Table.cpp
    src/parser/MarkdownParser.cpp
    resources.qrc
)

target_include_directories(MarkdownEditor PRIVATE src)
target_link_libraries(MarkdownEditor PRIVATE Qt6::Widgets)

if(ANDROID)
    qt_android_add_apk_target(MarkdownEditor)
endif()