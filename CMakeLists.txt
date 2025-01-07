cmake_minimum_required(VERSION 3.10)
project(ACMX2 LANGUAGES C CXX)
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
option(AUDIO "Audio support" OFF)
if(AUDIO)
find_library(RTAUDIO_LIBRARY NAMES rtaudio REQUIRED)
find_path(RTAUDIO_INCLUDE_DIR NAMES rtaudio/RtAudio.h REQUIRED)
set(AUDIO_INCLUDE ${RTAUDIO_INCLUDE_DIR})
set(AUDIO_LIBS ${RTAUDIO_LIBRARY})
set(AUDIO_SRC "audio.cpp")
message(STATUS "AUDIO LIBRARY: ${RTAUDIO_LIBRARY}")
message(STATUS "AUDIO INCLUDE: ${RTAUDIO_INCLUDE_DIR}")
else()
set(AUDIO_INCLUDE "")
set(AUDIO_LIBS "")
set(AUDIO_SRC "")
message(STATUS "AUDIO DISABLED")
endif()
message(STATUS "AVFORMAT_LIB: ${AVFORMAT_LIB}")
message(STATUS "AVCODEC_LIB: ${AVCODEC_LIB}")
message(STATUS "AVUTIL_LIB: ${AVUTIL_LIB}")
message(STATUS "SWSCALE_LIB: ${SWSCALE_LIB}")
set(FF_LIBS ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
add_library(ffwrite STATIC ffwrite.cpp)
target_include_directories(ffwrite PRIVATE /usr/include /usr/local/include ${CMAKE_PREFIX_PATH}/include ${CMAKE_INSTALL_PREFIX}/include /opt/homebrew/include)
target_link_libraries(ffwrite PRIVATE ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
add_executable(acmx2 acmx.cpp ${AUDIO_SRC})
if(AUDIO)
target_compile_definitions(acmx2 PRIVATE AUDIO_ENABLED)
endif()
target_compile_options(ffwrite PRIVATE -O2 -Wall -pedantic)
target_compile_options(acmx2 PRIVATE -O2 -Wall -pedantic)
target_include_directories(acmx2 PRIVATE 
    ${libmx2_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${AUDIO_INCLUDE}/rtaudio
    ${CMAKE_INSTALL_PREFIX}/include
)
target_link_libraries(acmx2 PRIVATE 
    libmx2::mx 
    ${SDL2_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES} 
    ${OpenCV_LIBRARIES} 
    ${AUDIO_LIBS}
    ffwrite
)
install(TARGETS acmx2 DESTINATION bin)
