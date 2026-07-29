set(PROJECT_SOURCES
    main.cpp
)

qt_add_executable(${EXECUTABLE_NAME}
    WIN32 MACOSX_BUNDLE
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Core
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}

    ${PROJECT_SOURCE_DIR}/src
)