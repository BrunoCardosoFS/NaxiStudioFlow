target_sources(${EXECUTABLE_NAME} PRIVATE
    catalog/CatalogItemDelegate.cpp
    catalog/CatalogItemDelegate.h
    catalog/FolderItemDelegate.cpp
    catalog/FolderItemDelegate.h
    catalog/CatalogWidget.cpp
    catalog/CatalogWidget.h
    catalog/CatalogWidget.ui

    playlist/PlaylistItemDelegate.h
    playlist/PlaylistItemDelegate.cpp
    playlist/PlaylistView.h
    playlist/PlaylistView.cpp

    cartwall/CartWallView.cpp
    cartwall/CartWallView.h
    cartwall/CartWallItemDelegate.cpp
    cartwall/CartWallItemDelegate.h

    editor/MixEditor.cpp
    editor/MixEditor.h
    editor/MixEditor.ui
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)