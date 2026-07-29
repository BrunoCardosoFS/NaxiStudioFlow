target_sources(${EXECUTABLE_NAME} PRIVATE
    flow.cpp
    flow.h
    flow.ui
)

add_subdirectory(widgets)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)