target_sources(${EXECUTABLE_NAME} PRIVATE
    Flow.cpp
    Flow.h
    Flow.ui
)

add_subdirectory(widgets)

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)