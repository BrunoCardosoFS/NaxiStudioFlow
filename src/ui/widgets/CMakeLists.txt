target_sources(${EXECUTABLE_NAME} PRIVATE
    filewidget.cpp
    filewidget.h

    cartwall/cartwallarea.cpp
    cartwall/cartwallarea.h
    cartwall/cartwallarea.ui
    cartwall/cartwallitem.cpp
    cartwall/cartwallitem.h
    cartwall/cartwallitem.ui

    editor/mixeditor.cpp
    editor/mixeditor.h
    editor/mixeditor.ui

    playlist/playlistitem.cpp
    playlist/playlistitem.h
    playlist/playlistitem.ui
    playlist/programblock.cpp
    playlist/programblock.h
    playlist/programblock.ui
)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)