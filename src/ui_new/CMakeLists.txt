# ui_new CMakeLists.txt - New UI Architecture
# This is designed to be included alongside the existing UI
# during the migration period.

set(UI_NEW_SOURCES
    # App shell
    app/App.cpp
    app/App.h
    app/AppNavigation.cpp
    app/AppNavigation.h
    
    # Pages
    pages/FileTranslationPage.cpp
    pages/FileTranslationPage.h
    pages/ImageTranslationPage.cpp
    pages/ImageTranslationPage.h
    pages/SettingsPage.cpp
    pages/SettingsPage.h
    
    # ViewModels
    viewmodels/BaseViewModel.cpp
    viewmodels/BaseViewModel.h
    viewmodels/FileTranslationViewModel.cpp
    viewmodels/FileTranslationViewModel.h
    viewmodels/ImageTranslationViewModel.cpp
    viewmodels/ImageTranslationViewModel.h
    
    # Components - Translation
    components/translation/FileListView.cpp
    components/translation/FileListView.h
    components/translation/TranslationTable.cpp
    components/translation/TranslationTable.h
    components/translation/ProgressIndicator.cpp
    components/translation/ProgressIndicator.h
    
    # Components - Image
    components/image/ImageViewer.cpp
    components/image/ImageViewer.h
    components/image/ImageQueue.cpp
    components/image/ImageQueue.h
    
    # Styles
    styles/Theme.cpp
    styles/Theme.h
)

# Prepend source directory path
list(TRANSFORM UI_NEW_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

# Export as a variable that parent CMakeLists.txt can use
set(UI_NEW_SOURCES ${UI_NEW_SOURCES} PARENT_SCOPE)

# Include directories for new UI
set(UI_NEW_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/pages
    ${CMAKE_CURRENT_SOURCE_DIR}/viewmodels
    ${CMAKE_CURRENT_SOURCE_DIR}/components
    ${CMAKE_CURRENT_SOURCE_DIR}/components/translation
    ${CMAKE_CURRENT_SOURCE_DIR}/components/image
    ${CMAKE_CURRENT_SOURCE_DIR}/styles
)

set(UI_NEW_INCLUDE_DIRS ${UI_NEW_INCLUDE_DIRS} PARENT_SCOPE)
