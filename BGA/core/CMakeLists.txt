add_library(BGACore SHARED
    include/core/gameanalyzer.h
    include/core/analyzerfactory.h
    include/core/engines/rpgm/rpganalyzer.h
    include/core/engines/unity/unityanalyzer.h
    include/core/engines/renpy/renpyanalyzer.h
    include/core/engines/godot/godotanalyzer.h
    src/analyzerfactory.cpp
    src/engines/rpgm/rpganalyzer.cpp
    src/engines/unity/unityanalyzer.cpp
    src/engines/renpy/renpyanalyzer.cpp
    src/engines/godot/godotanalyzer.cpp
)

target_include_directories(BGACore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(BGACore
    PUBLIC
        Qt6::Core
)


install(TARGETS BGACore
    EXPORT BGACoreTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
)
