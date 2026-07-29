cmake_minimum_required(VERSION 3.16)

include(version.cmake)

project(${EXECUTABLE_NAME} VERSION ${CLEAN_APP_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Multimedia Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

set(QT_MEDIA_BACKEND ffmpeg)

add_subdirectory(src/app)
add_subdirectory(src/core)
add_subdirectory(src/lang)
add_subdirectory(src/resources)
add_subdirectory(src/ui)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/app.rc")
target_sources(${EXECUTABLE_NAME} PRIVATE ${app_icon_resource_windows})



set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.NaxStudio.${EXECUTABLE_NAME})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ${EXECUTABLE_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(${EXECUTABLE_NAME})
