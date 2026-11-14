cmake_minimum_required(VERSION 3.24)

project(
    LowLevelKeyMapper
    VERSION 1.0.0
    DESCRIPTION "A Windows keyboard remapping utility"
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "LowLevelKeyMapper currently supports Windows only.")
endif()

# Language configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies
find_package(nlohmann_json 3.11 QUIET)

if(NOT nlohmann_json_FOUND)
    include(FetchContent)

    set(JSON_BuildTests OFF CACHE INTERNAL "")

    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(json)
endif()

# Application
add_executable(
    LowLevelKeyMapper
    WIN32
    src/main.cpp
    src/app/Application.cpp
    src/ui/MainWindow.cpp
    src/ui/TrayIcon.cpp
    src/input/KeyboardHook.cpp
    src/input/KeyStateManager.cpp
    src/mapping/KeyMapper.cpp
)

target_include_directories(
    LowLevelKeyMapper
    PRIVATE
        include
)

target_link_libraries(
    LowLevelKeyMapper
    PRIVATE
        nlohmann_json::nlohmann_json
        user32
        shell32
        advapi32
)

target_compile_definitions(
    LowLevelKeyMapper
    PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

if(MSVC)
    target_compile_options(
        LowLevelKeyMapper
        PRIVATE
            /W4
            /permissive-
            /EHsc
    )
else()
    target_compile_options(
        LowLevelKeyMapper
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# Keep the default configuration beside the executable.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    "${CMAKE_CURRENT_BINARY_DIR}/config.json"
    COPYONLY
)

install(
    TARGETS LowLevelKeyMapper
    RUNTIME DESTINATION .
)

install(
    FILES config.json
    DESTINATION .
)
