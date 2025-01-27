cmake_minimum_required(VERSION 3.10)
project(OpenCVExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLs ON)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

#find_package(OpenCV REQUIRED  HINTS "C:/opencv4")
find_package(OpenCV REQUIRED)
find_package(MXWrite REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec 
    libavformat 
    libavutil 
    libswscale 
)

add_executable(opencv_ex opencv_ex.cpp)
target_include_directories(opencv_ex PRIVATE ${OpenCV_INCLUDE_DIRS} ${MXWrite_INCLUDE_DIRS})
target_link_libraries(opencv_ex PRIVATE 
    ${OpenCV_LIBS} 
    MXWrite::mxwrite 
    PkgConfig::FFMPEG
)

