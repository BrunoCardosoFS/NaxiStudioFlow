target_sources(${EXECUTABLE_NAME} PRIVATE
    models.h
    
    catalog/CatalogItem.h
    catalog/CatalogListModel.cpp
    catalog/CatalogListModel.h
    catalog/CatalogRepository.cpp
    catalog/CatalogRepository.h
    catalog/CatalogScanner.cpp
    catalog/CatalogScanner.h
    catalog/FolderItem.h
    catalog/FolderListModel.cpp
    catalog/FolderListModel.h

    playlist/PlaylistManager.cpp
    playlist/PlaylistManager.h
    playlist/PlaylistListModel.cpp
    playlist/PlaylistListModel.h
    playlist/player/PlayerManager.cpp
    playlist/player/PlayerManager.h

    utils/TimeUtils.cpp
    utils/TimeUtils.h
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)