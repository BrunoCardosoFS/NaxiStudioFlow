target_sources(${EXECUTABLE_NAME} PRIVATE
    catalog/FileWidget.cpp
    catalog/FileWidget.h

    cartwall/CartWallArea.cpp
    cartwall/CartWallArea.h
    cartwall/CartWallArea.ui
    cartwall/CartWallItem.cpp
    cartwall/CartWallItem.h
    cartwall/CartWallItem.ui

    editor/MixEditor.cpp
    editor/MixEditor.h
    editor/MixEditor.ui

    playlist/PlaylistItem.cpp
    playlist/PlaylistItem.h
    playlist/PlaylistItem.ui
    playlist/ProgramBlock.cpp
    playlist/ProgramBlock.h
    playlist/ProgramBlock.ui
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)