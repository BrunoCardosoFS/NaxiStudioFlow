target_sources(${EXECUTABLE_NAME} PRIVATE
    models.h
    
    catalog/FilesList.cpp
    catalog/FilesList.h
    catalog/FoldersList.cpp
    catalog/FoldersList.h

    playlist/PlaylistManager.cpp
    playlist/PlaylistManager.h
    playlist/player/PlayerManager.cpp
    playlist/player/PlayerManager.h

    utils/TimeUtils.cpp
    utils/TimeUtils.h
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)