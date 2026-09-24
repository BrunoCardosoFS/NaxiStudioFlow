target_sources(${EXECUTABLE_NAME} PRIVATE
    catalog/CatalogItemDelegate.cpp
    catalog/CatalogItemDelegate.h

    playlist/PlaylistItemDelegate.h
    playlist/PlaylistItemDelegate.cpp

    cartwall/CartWallArea.cpp
    cartwall/CartWallArea.h
    cartwall/CartWallArea.ui
    cartwall/CartWallItem.cpp
    cartwall/CartWallItem.h
    cartwall/CartWallItem.ui

    editor/MixEditor.cpp
    editor/MixEditor.h
    editor/MixEditor.ui
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)