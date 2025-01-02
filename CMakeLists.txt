cmake_minimum_required(VERSION 3.10)
project(ACMX2)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(libmx2 REQUIRED)
find_package(OpenCV REQUIRED)
find_library(AVFORMAT_LIB NAMES avformat REQUIRED)
find_library(AVCODEC_LIB NAMES avcodec REQUIRED)
find_library(AVUTIL_LIB NAMES avutil REQUIRED)
find_library(SWSCALE_LIB NAMES swscale REQUIRED)
message(STATUS "AVFORMAT_LIB: ${AVFORMAT_LIB}")
message(STATUS "AVCODEC_LIB: ${AVCODEC_LIB}")
message(STATUS "AVUTIL_LIB: ${AVUTIL_LIB}")
message(STATUS "SWSCALE_LIB: ${SWSCALE_LIB}")
set(FF_LIBS ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
add_library(ffwrite STATIC ffwrite.cpp)
target_include_directories(ffwrite PRIVATE)
target_link_libraries(ffwrite PRIVATE ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
add_executable(acmx2 acmx.cpp)
target_include_directories(acmx2 PRIVATE 
    ${CMAKE_PREFIX_PATH}/include/mx2
    /usr/include/mx2
    /usr/local/include/mx2
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(acmx2 PRIVATE 
    libmx2::mx 
    ${SDL2_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES} 
    ${OpenCV_LIBRARIES} 
    ffwrite
)
install(TARGETS acmx2 DESTINATION bin)
