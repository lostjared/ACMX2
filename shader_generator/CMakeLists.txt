cmake_minimum_required(VERSION 3.16)
project(shader_generator)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(PROJECT_NAME shader_generator)
set(SOURCES gencode.cpp mx2-ollama.cpp)
set(HEADERS mx2-ollama.hpp)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})
target_compile_options(${PROJECT_NAME} PRIVATE ${CURL_CFLAGS_OTHER})
target_include_directories(${PROJECT_NAME} PRIVATE .)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
