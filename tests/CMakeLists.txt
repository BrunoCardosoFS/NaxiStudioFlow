qt_add_executable(test_playlist_manager
    test_playlist_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/models.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogItem.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogListModel.cpp
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogListModel.h
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistListModel.cpp
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistListModel.h
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistManager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistManager.h
    ${PROJECT_SOURCE_DIR}/src/core/playlist/player/PlayerManager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/playlist/player/PlayerManager.h
    ${PROJECT_SOURCE_DIR}/src/core/utils/TimeUtils.cpp
    ${PROJECT_SOURCE_DIR}/src/core/utils/TimeUtils.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistItemDelegate.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistItemDelegate.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistView.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistView.h
    ${PROJECT_SOURCE_DIR}/src/resources/app.qrc
)

target_include_directories(test_playlist_manager PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(test_playlist_manager PRIVATE
    Qt6::Core
    Qt6::Multimedia
    Qt6::Test
    Qt6::Widgets
)

add_test(NAME PlaylistManagerTests COMMAND test_playlist_manager)

qt_add_executable(test_mix_editor
    test_mix_editor.cpp
    ${PROJECT_SOURCE_DIR}/src/core/models.h
    ${PROJECT_SOURCE_DIR}/src/core/audio/WaveformExtractor.cpp
    ${PROJECT_SOURCE_DIR}/src/core/audio/WaveformExtractor.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogItem.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogListModel.cpp
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogListModel.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogRepository.cpp
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogRepository.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogScanner.cpp
    ${PROJECT_SOURCE_DIR}/src/core/catalog/CatalogScanner.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/FolderItem.h
    ${PROJECT_SOURCE_DIR}/src/core/catalog/FolderListModel.cpp
    ${PROJECT_SOURCE_DIR}/src/core/catalog/FolderListModel.h
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistListModel.cpp
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistListModel.h
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistManager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/playlist/PlaylistManager.h
    ${PROJECT_SOURCE_DIR}/src/core/playlist/player/PlayerManager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/playlist/player/PlayerManager.h
    ${PROJECT_SOURCE_DIR}/src/core/utils/TimeUtils.cpp
    ${PROJECT_SOURCE_DIR}/src/core/utils/TimeUtils.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/catalog/CatalogItemDelegate.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/catalog/CatalogItemDelegate.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/catalog/CatalogWidget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/catalog/CatalogWidget.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/catalog/CatalogWidget.ui
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/catalog/FolderItemDelegate.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/catalog/FolderItemDelegate.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/editor/MixEditor.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/editor/MixEditor.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/editor/MixEditor.ui
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/editor/WaveformWidget.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/editor/WaveformWidget.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistItemDelegate.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistItemDelegate.h
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistView.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/widgets/playlist/PlaylistView.h
    ${PROJECT_SOURCE_DIR}/src/resources/app.qrc
)

target_include_directories(test_mix_editor PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(test_mix_editor PRIVATE
    Qt6::Core
    Qt6::Multimedia
    Qt6::Test
    Qt6::Widgets
)

add_test(NAME MixEditorTests COMMAND test_mix_editor)
