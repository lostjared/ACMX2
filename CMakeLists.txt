cmake_minimum_required(VERSION 3.10)
project(ACMX2)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(libmx2 REQUIRED)
find_package(OpenCV REQUIRED)
add_executable(acmx2 acmx.cpp)
target_include_directories(acmx2 PRIVATE 
    ${CMAKE_PREFIX_PATH}/include/mx2
    /usr/include/mx2
    /usr/local/include/mx2
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(acmx2 PRIVATE libmx2::mx ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${OpenCV_LIBRARIES})
install(TARGETS acmx2 DESTINATION bin)