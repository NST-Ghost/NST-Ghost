cmake_minimum_required(VERSION 3.16)
project(NST VERSION 0.5.4 LANGUAGES CXX)
add_definitions(-DAPP_VERSION=\"${PROJECT_VERSION}\")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# Fix Linux RPATH to find libraries in same directory
if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_RPATH "$ORIGIN")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Concurrent Network)
find_package(Lua REQUIRED)
message(STATUS "LUA_INCLUDE_DIR: ${LUA_INCLUDE_DIR}")
message(STATUS "LUA_LIBRARIES: ${LUA_LIBRARIES}")

# Use qt_standard_project_setup for Qt 6.3+
if(Qt6_VERSION_MAJOR EQUAL 6 AND Qt6_VERSION_MINOR GREATER_EQUAL 3)
    qt_standard_project_setup()
endif()

# Python integration for AI features (Image/Realtime translation)
# Defaults to ON; auto-disables if pybind11 is not found
option(NST_ENABLE_PYTHON "Enable Python integration for Image/Realtime translation" ON)

if(NST_ENABLE_PYTHON)
    find_package(pybind11 QUIET)
    if(NOT pybind11_FOUND)
        message(WARNING "pybind11 not found — disabling Python integration. Install pybind11 for image translation support.")
        set(NST_ENABLE_PYTHON OFF)
    else()
        message(STATUS "Python integration ENABLED")
    endif()
else()
    message(STATUS "Python integration DISABLED (explicitly set OFF)")
endif()

set(TS_FILES src/NST_en_US.ts)

set(PROJECT_SOURCES
    src/core/main.cpp
    src/ui/mainwindow.cpp
    src/ui/menubar.cpp
    src/dialogs/loadprojectdialog.cpp
    src/dialogs/searchdialog.cpp
    src/dialogs/settingsdialog.cpp
    src/controllers/searchcontroller.cpp
    src/controllers/shortcutcontroller.cpp
    src/managers/bgadatamanager.cpp
    src/managers/translationservicemanager.cpp
    src/ui/realtimetranslationwidget.cpp
    src/ui/realtimetranslationwidget.h
    src/ui/imagetranslationwidget.cpp
    src/ui/imagetranslationwidget.h
    src/ui/imagetranslationwidget.ui
    src/ui/imageprocessorworker.cpp
    src/ui/imageprocessorworker.h
    
    src/managers/smartfiltermanager.cpp
    src/ui/translationcontextmenu.cpp
    src/controllers/updatecontroller.cpp
    src/dialogs/fontmanagerdialog.cpp
    src/dialogs/pluginmanagerdialog.cpp
    src/dialogs/plugindebuggerdialog.cpp
    src/plugins/LuaTranslationPlugin/luatranslationservice.cpp
    src/managers/projectdatamanager.cpp
    src/ui/mainwindow.h
    src/ui/menubar.h
    src/dialogs/scripteditordialog.cpp
    src/dialogs/scripteditordialog.h
    src/dialogs/loadprojectdialog.h
    src/dialogs/searchdialog.h
    src/dialogs/settingsdialog.h
    src/dialogs/fontmanagerdialog.h
    src/dialogs/pluginmanagerdialog.h
    src/dialogs/plugindebuggerdialog.h
    src/dialogs/featuremanagerdialog.cpp
    src/dialogs/featuremanagerdialog.h
    src/plugins/LuaTranslationPlugin/luatranslationservice.h
    src/controllers/searchcontroller.h
    src/controllers/shortcutcontroller.h
    src/managers/bgadatamanager.h
    src/managers/translationservicemanager.h
    src/ui/translationcontextmenu.h
    src/controllers/updatecontroller.h
    src/dialogs/customprogressdialog.cpp
    src/dialogs/customprogressdialog.h
    src/managers/projectdatamanager.h
    src/ui/customtitlebar.cpp
    src/ui/customtitlebar.h
    src/ui/filetranslationwidget.cpp
    src/ui/filetranslationwidget.h
    src/ui/filetranslationwidget.ui
    src/core/translationserver.cpp
    src/core/translationserver.h
    src/dialogs/processselectordialog.cpp
    src/dialogs/processselectordialog.h
    src/resources.qrc
)


# Lua plugin sources (optional)
if(LUA_FOUND)
    list(APPEND PROJECT_SOURCES
        src/plugins/LuaPlugin.cpp
        src/plugins/LuaPlugin.h
        src/plugins/LuaScriptManager.cpp
        src/plugins/LuaScriptManager.h
        src/plugins/PluginManager.cpp
        src/plugins/PluginManager.h
    )
endif()

# Add BGA as a subdirectory
add_subdirectory(BGA)
# Add QtLingo as a subdirectory
add_subdirectory(QtLingo)
# Add plugins subdirectory
add_subdirectory(src/plugins)

    qt_add_executable(NST
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${PROJECT_SOURCES} ${TS_FILES})

target_link_libraries(NST PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    BGACore
    QtLingo
)

# Conditionally link Python (only when enabled)
if(NST_ENABLE_PYTHON)
    target_link_libraries(NST PRIVATE pybind11::embed)
    target_compile_definitions(NST PRIVATE HAS_PYTHON=1)
    
    # Windows: Define expected Python paths for build-time validation
    if(WIN32)
        target_compile_definitions(NST PRIVATE 
            NST_WIN_PYTHON_SUBDIR="python"
            NST_WIN_PYTHON_DLL="python311.dll"
        )
        message(STATUS "Windows Python paths: subdir=python, dll=python311.dll")
    endif()
endif()



message(STATUS "DEBUG: Reached line 120 (before if check)")
message(STATUS "DEBUG: Lua_FOUND: ${Lua_FOUND}")
message(STATUS "DEBUG: LUA_FOUND: ${LUA_FOUND}")
message(STATUS "DEBUG: LUA_INCLUDE_DIR: ${LUA_INCLUDE_DIR}")

if(LUA_FOUND OR Lua_FOUND)
    message(STATUS "Lua found (LUA_FOUND or Lua_FOUND is true), adding include directories: ${LUA_INCLUDE_DIR}")
    target_link_libraries(NST PRIVATE ${LUA_LIBRARIES})
    target_include_directories(NST PRIVATE ${LUA_INCLUDE_DIR})
    target_compile_definitions(NST PRIVATE HAS_LUA=1)
else()
    message(WARNING "Lua not found (LUA_FOUND and Lua_FOUND are false)")
endif()

target_include_directories(NST PRIVATE 
    ${CMAKE_SOURCE_DIR}/BGA/core/include 
    ${CMAKE_SOURCE_DIR}/QtLingo/include 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/dialogs
    ${CMAKE_SOURCE_DIR}/src/managers
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/controllers
)

# Copy scripts to build directory on every build
add_custom_command(TARGET NST POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/pylib
        ${CMAKE_BINARY_DIR}/pylib
    COMMENT "Syncing Python library (pylib) to build directory..."
)
# Also copy scripts (Lua) as before, just in case
add_custom_command(TARGET NST POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/scripts
        ${CMAKE_BINARY_DIR}/scripts
    COMMENT "Syncing Lua scripts to build directory..."
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${Qt6_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.NST)
endif()

set_target_properties(NST PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

# Install executable
install(TARGETS NST
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install BGACore and QtLingo libraries
if(UNIX AND NOT APPLE)
    install(TARGETS BGACore QtLingo
        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(WIN32)
    install(TARGETS BGACore QtLingo
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

install(FILES packaging/debian/nst.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/icons/nst.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

# Option to skip Qt deployment (useful for AppImage/custom packaging)
option(SKIP_QT_DEPLOY "Skip Qt deployment during install" OFF)
if(DEFINED ENV{SKIP_QT_DEPLOY})
    set(SKIP_QT_DEPLOY ON)
endif()

# Use Qt's official deployment API (Qt 6.3+)
if(QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 3 AND NOT SKIP_QT_DEPLOY)
    qt_generate_deploy_app_script(
        TARGET NST
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
else()
    # Fallback for older Qt versions or when SKIP_QT_DEPLOY is ON
    if(WIN32 AND NOT SKIP_QT_DEPLOY)
        # Find windeployqt
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
        
        if(WINDEPLOYQT_EXECUTABLE)
            message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
            
            # Install Qt DLLs with CPack
            install(CODE "
                execute_process(
                    COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                        --release
                        --no-translations
                        --no-system-d3d-compiler
                        --no-opengl-sw
                        --compiler-runtime
                        --dir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
                        \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/NST.exe\"
                    RESULT_VARIABLE windeployqt_result
                    OUTPUT_VARIABLE windeployqt_output
                    ERROR_VARIABLE windeployqt_error
                )
                
                if(NOT windeployqt_result EQUAL 0)
                    message(WARNING \"windeployqt failed: \${windeployqt_error}\")
                else()
                    message(STATUS \"windeployqt output: \${windeployqt_output}\")
                endif()
            " COMPONENT Runtime)
            
            # Create qt.conf to ensure Qt finds plugins
            install(CODE "
                set(qt_conf_content \"[Paths]\\nPlugins = plugins\\nPrefix = .\\n\")
                file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/qt.conf\" \"\${qt_conf_content}\")
                message(STATUS \"Created qt.conf in \${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
            " COMPONENT Runtime)
        else()
            message(WARNING "windeployqt not found. Qt dependencies will not be deployed automatically.")
        endif()
        
        # Install MinGW Runtime (if using MinGW)
        if(MINGW)
            get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
            
            set(MINGW_DLLS
                "libgcc_s_seh-1.dll"
                "libstdc++-6.dll"
                "libwinpthread-1.dll"
            )
            
            foreach(dll ${MINGW_DLLS})
                set(dll_path "${MINGW_BIN_DIR}/${dll}")
                if(EXISTS "${dll_path}")
                    install(FILES "${dll_path}"
                        DESTINATION ${CMAKE_INSTALL_BINDIR}
                        COMPONENT Runtime
                    )
                    message(STATUS "Will install MinGW runtime: ${dll}")
                else()
                    message(WARNING "MinGW runtime not found: ${dll_path}")
                endif()
            endforeach()
        endif()
        
        # Install Visual C++ Runtime (if using MSVC)
        if(MSVC)
            set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
            include(InstallRequiredSystemLibraries)
            install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT Runtime
            )
        endif()
    endif()
    
    # macOS: Bundle Qt frameworks
    if(APPLE AND NOT SKIP_QT_DEPLOY)
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
        
        if(MACDEPLOYQT_EXECUTABLE)
            message(STATUS "Found macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")
            
            add_custom_command(TARGET NST POST_BUILD
                COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                    "$<TARGET_BUNDLE_DIR:NST>"
                    -always-overwrite
                COMMENT "Deploying Qt frameworks with macdeployqt..."
            )
        else()
            message(WARNING "macdeployqt not found. Qt frameworks will not be bundled automatically.")
        endif()
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NST)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "NST")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool for translating video games.")
set(CPACK_PACKAGE_VENDOR "NST Ghost")
set(CPACK_PACKAGE_CONTACT "you@example.com")

# Default generator
set(CPACK_GENERATOR "ZIP")

# Linux
if(UNIX AND NOT APPLE)
    set(CPACK_PACKAGE_FILE_NAME "NST-${PROJECT_VERSION}-Linux")
endif()

# macOS (DMG)
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_FILE_NAME "NST-${PROJECT_VERSION}-macOS")
endif()

# Windows (ZIP)
if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "NST-${PROJECT_VERSION}-win64")
endif()

include(CPack)
add_subdirectory(Injection)
