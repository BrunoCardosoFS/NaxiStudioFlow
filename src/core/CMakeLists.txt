target_sources(${EXECUTABLE_NAME} PRIVATE
    catalog/FilesList.cpp
    catalog/FilesList.h
    catalog/FoldersList.cpp
    catalog/FoldersList.h

    utils/TimeUtils.cpp
    utils/TimeUtils.h

    player/PlayerPlaylistItem.cpp
    player/PlayerPlaylistItem.h

    playlist/MasterController.cpp
    playlist/MasterController.h
    playlist/PlaylistController.cpp
    playlist/PlaylistController.h
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)