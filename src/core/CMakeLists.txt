target_sources(${EXECUTABLE_NAME} PRIVATE
    catalog/fileslist.cpp
    catalog/fileslist.h
    catalog/folderslist.h

    functions/timeFunctions.h

    player/playerplaylistitem.cpp
    player/playerplaylistitem.h

    playlist/mastercontroller.cpp
    playlist/mastercontroller.h
    playlist/playlistcontroller.cpp
    playlist/playlistcontroller.h
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)