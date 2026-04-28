cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CLEAN_APP_VERSION OR CLEAN_APP_VERSION STREQUAL "")
    set(CLEAN_APP_VERSION "0.0.1")
endif()

if(NOT DEFINED APP_VERSION OR APP_VERSION STREQUAL "")
    set(APP_VERSION "v0.0.1")
endif()

add_compile_definitions(APP_VERSION="${APP_VERSION}")

project(Flow VERSION ${CLEAN_APP_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Multimedia Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

set(QT_MEDIA_BACKEND ffmpeg)

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
        backend/playlist/playlistcontroller.h backend/playlist/playlistcontroller.cpp
        backend/catalog/fileslist.h backend/catalog/fileslist.cpp
        backend/catalog/folderslist.h

        frontend/widgets/cartwall/cartwallarea.h frontend/widgets/cartwall/cartwallarea.cpp frontend/widgets/cartwall/cartwallarea.ui
        frontend/widgets/cartwall/cartwallitem.h frontend/widgets/cartwall/cartwallitem.cpp frontend/widgets/cartwall/cartwallitem.ui
        backend/playlist/mastercontroller.h backend/playlist/mastercontroller.cpp
)

qt_add_executable(Flow
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

set(TS_FILES
    lang/pt_BR.ts
    lang/en_US.ts
)

qt_add_translations(Flow
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

foreach(qm_file ${QM_FILES})
    get_filename_component(qm_name ${qm_file} NAME)

    set_source_files_properties(${qm_file} PROPERTIES
        QT_RESOURCE_ALIAS ${qm_name}
    )
endforeach()

qt_add_resources(Flow "translations"
    PREFIX "/i18n"
    FILES ${QM_FILES}
)


set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/frontend/resources/Flow.rc")
target_sources(Flow PRIVATE ${app_icon_resource_windows})

target_link_libraries(Flow PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(Flow PRIVATE Qt6::Multimedia Qt6::Core)
target_link_libraries(Flow PRIVATE Qt6::Core)
target_link_libraries(Flow PRIVATE Qt6::Core)
target_link_libraries(Flow PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.NaxStudio.Flow)
endif()
set_target_properties(Flow PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Flow
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Flow)
endif()
