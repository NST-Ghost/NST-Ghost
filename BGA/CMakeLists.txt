cmake_minimum_required(VERSION 3.16)

project(BGACore LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

# =============================================================================
# Rust Libraries Build
# =============================================================================
set(RPGM_RS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../rpgm_rs)
set(RENPY_RS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../renpy_rs)

if(MSVC)
    set(RPGM_RS_LIBRARY ${RPGM_RS_DIR}/target/release/rpgm_rs.lib)
    set(RENPY_RS_LIBRARY ${RENPY_RS_DIR}/target/release/renpy_rs.lib)
else()
    set(RPGM_RS_LIBRARY ${RPGM_RS_DIR}/target/release/librpgm_rs.a)
    set(RENPY_RS_LIBRARY ${RENPY_RS_DIR}/target/release/librenpy_rs.a)
endif()

# Custom commands to build Rust libraries
add_custom_command(
    OUTPUT ${RPGM_RS_LIBRARY}
    COMMAND cargo build --release
    WORKING_DIRECTORY ${RPGM_RS_DIR}
    COMMENT "Building Rust rpgm_rs library..."
    VERBATIM
)

add_custom_command(
    OUTPUT ${RENPY_RS_LIBRARY}
    COMMAND cargo build --release
    WORKING_DIRECTORY ${RENPY_RS_DIR}
    COMMENT "Building Rust renpy_rs library..."
    VERBATIM
)

# Custom target for Rust builds
add_custom_target(rust_engines ALL DEPENDS ${RPGM_RS_LIBRARY} ${RENPY_RS_LIBRARY})

# =============================================================================
# Source files (C++ Analyzers)
# =============================================================================
set(CORE_SOURCES
    core/src/analyzerfactory.cpp
    core/src/engines/rpgm/rpganalyzer.cpp
    core/src/engines/renpy/renpyanalyzer.cpp
    core/src/engines/unity/unityanalyzer.cpp
)

set(CORE_HEADERS
    core/include/core/analyzerfactory.h
    core/include/core/gameanalyzer.h
    core/include/core/engines/rpgm/rpganalyzer.h
    core/include/core/engines/renpy/renpyanalyzer.h
    core/include/core/engines/unity/unityanalyzer.h
)

# =============================================================================
# Create library
# =============================================================================
add_library(BGACore SHARED ${CORE_SOURCES} ${CORE_HEADERS})

# Ensure Rust is built before BGACore
add_dependencies(BGACore rust_engines)

# Export symbols on Windows
set_target_properties(BGACore PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# =============================================================================
# Linking
# =============================================================================
target_link_libraries(BGACore PRIVATE 
    Qt6::Core
    ${RPGM_RS_LIBRARY}
    ${RENPY_RS_LIBRARY}
)

# Platform-specific linking for Rust static library
if(UNIX AND NOT APPLE)
    target_link_libraries(BGACore PRIVATE pthread dl m)
elseif(APPLE)
    target_link_libraries(BGACore PRIVATE pthread dl)
elseif(WIN32)
    target_link_libraries(BGACore PRIVATE ws2_32 userenv bcrypt ntdll)
endif()

target_include_directories(BGACore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

add_subdirectory(tests)
