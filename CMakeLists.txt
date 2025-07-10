cmake_minimum_required(VERSION 3.16)

project(NaxiStudioFlow VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Multimedia Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

set(QT_MEDIA_BACKEND ffmpeg)

set(TS_FILES
    lang/pt_BR.ts
    lang/en_US.ts
)

set(PROJECT_QRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/lang/translations.qrc
)

set(PROJECT_SOURCES
        main.cpp
        frontend/flow.cpp
        frontend/flow.h
        frontend/flow.ui
        frontend/resources/flow.qrc
        frontend/widgets/filewidget.h frontend/widgets/filewidget.cpp
        frontend/widgets/playlist/programblock.h frontend/widgets/playlist/programblock.cpp frontend/widgets/playlist/programblock.ui
        frontend/widgets/playlist/programblock.h
        frontend/widgets/playlist/playlistitem.h frontend/widgets/playlist/playlistitem.cpp frontend/widgets/playlist/playlistitem.ui
        backend/functions/timeFunctions.h
        frontend/widgets/editor/mixeditor.h frontend/widgets/editor/mixeditor.cpp frontend/widgets/editor/mixeditor.ui
        backend/player/playerplaylistitem.h backend/player/playerplaylistitem.cpp
        backend/playlist/playlistcontoller.h backend/playlist/playlistcontoller.cpp
        backend/catalog/fileslist.h backend/catalog/fileslist.cpp
        backend/catalog/folderslist.h
)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/frontend/resources/NaxiStudioFlow.rc")

qt_add_lupdate(
    SOURCE_TARGETS NaxiStudioFlow
    TS_FILES ${TS_FILES}
)

qt_add_lrelease(
    TS_FILES ${TS_FILES}
    QM_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lang
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(NaxiStudioFlow
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${TS_FILES}
        ${PROJECT_QRC_FILES}
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(NaxiStudioFlow
        ${PROJECT_SOURCES}
    )

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_sources(NaxiStudioFlow PRIVATE ${app_icon_resource_windows})
target_link_libraries(NaxiStudioFlow PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(NaxiStudioFlow PRIVATE Qt6::Multimedia Qt6::Core)
target_link_libraries(NaxiStudioFlow PRIVATE Qt6::Core)
target_link_libraries(NaxiStudioFlow PRIVATE Qt6::Core)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER org.NaxiStudio.NaxiStudioFlow)
endif()
set_target_properties(NaxiStudioFlow PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS NaxiStudioFlow
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NaxiStudioFlow)
endif()
