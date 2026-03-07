cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(acmx2_interface LANGUAGES CXX C)
set(LAUNCHER_MAJOR_VERSION 2)
set(LAUNCHER_MINOR_VERSION 3)
set(LAUNCHER_PATCH_VERSION 0)
set(LAUNCHER_VERSION ${LAUNCHER_MAJOR_VERSION}.${LAUNCHER_MINOR_VERSION}.${LAUNCHER_PATCH_VERSION})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "============================================")
message(STATUS "Building ${PROJECT_NAME} v${LAUNCHER_VERSION}")
message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")
message(STATUS "============================================")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND COMPILER_FLAGS "-O3")
else()
    message(STATUS "Debug Mode enabled")
    list(APPEND COMPILER_FLAGS "-g")
endif()

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "/usr/local/lib;/usr/lib")

message(STATUS "")
message(STATUS "--- Checking Dependencies ---")

find_package(Qt6 COMPONENTS Core Gui Widgets Concurrent Network QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Qt5 Found")
    message(STATUS "  Qt5Core: ${Qt5Core_LIBRARIES}")
    message(STATUS "  Qt5Gui: ${Qt5Gui_LIBRARIES}")
    message(STATUS "  Qt5Widgets: ${Qt5Widgets_LIBRARIES}")
    message(STATUS "  Qt5Concurrent: ${Qt5Concurrent_LIBRARIES}")
    message(STATUS "  Qt5Network: ${Qt5Network_LIBRARIES}")
else()
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Qt6 Found")
    message(STATUS "  Qt6Core: ${Qt6Core_LIBRARIES}")
    message(STATUS "  Qt6Gui: ${Qt6Gui_LIBRARIES}")
    message(STATUS "  Qt6Widgets: ${Qt6Widgets_LIBRARIES}")
    message(STATUS "  Qt6Concurrent: ${Qt6Concurrent_LIBRARIES}")
    message(STATUS "  Qt6Network: ${Qt6Network_LIBRARIES}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(Threads_FOUND)
    message(STATUS "Threads library found")
else()
    message(FATAL_ERROR "Threads library not found!")
endif()

message(STATUS "")
message(STATUS "--- Dependency Check Complete ---")
message(STATUS "")

set(LAUNCHER_GUI_HEADERS
    audio-window.hpp
    editor.hpp
    main_window.hpp
    prop.hpp
    settings.hpp
    shader.hpp
    shaderlibrary.hpp
    shaderpass.hpp
    syntax.hpp
    version_info.hpp
)

set(LAUNCHER_GUI_SOURCES
    audio-window.cpp
    editor.cpp
    main.cpp
    main_window.cpp
    prop.cpp
    settings.cpp
    shader.cpp
    shaderlibrary.cpp
    shaderpass.cpp
    syntax.cpp
)

set(LAUNCHER_GUI_RESOURCES
    qresource.qrc
)

add_executable(acmx2_interface
    ${LAUNCHER_GUI_HEADERS}
    ${LAUNCHER_GUI_SOURCES}
    ${LAUNCHER_GUI_RESOURCES}
)

target_include_directories(acmx2_interface
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(acmx2_interface
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Concurrent
            Qt6::Network
            Threads::Threads
    )
else()
    target_link_libraries(acmx2_interface
        PRIVATE
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Concurrent
            Qt5::Network
            Threads::Threads
    )
endif()

target_compile_options(acmx2_interface PRIVATE ${COMPILER_FLAGS})

add_custom_command(TARGET acmx2_interface POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_CURRENT_BINARY_DIR}/data
)

install(TARGETS acmx2_interface DESTINATION bin RENAME launcher)

message(STATUS "")
message(STATUS "============================================")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} v${LAUNCHER_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Target: acmx2_interface")
message(STATUS "============================================")
