cmake_minimum_required(VERSION 3.10)
project(MXWrite LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec 
    libavformat 
    libavutil 
    libswscale 
)
add_library(mxwrite STATIC mxwrite.cpp)
target_include_directories(mxwrite PUBLIC ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(mxwrite PUBLIC PkgConfig::FFMPEG)
target_compile_options(mxwrite PRIVATE -O2 -Wall  -pedantic)
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MXWriteConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/MXWriteConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MXWriteConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MXWrite
)

install(FILES mxwrite.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS mxwrite
    EXPORT MXWriteTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/MXWriteConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MXWriteConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MXWrite
)

install(EXPORT MXWriteTargets
    NAMESPACE MXWrite::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MXWrite
)

export(EXPORT MXWriteTargets
    NAMESPACE MXWrite::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/MXWriteTargets.cmake"
)