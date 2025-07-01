cmake_minimum_required(VERSION 3.13)

project(codecrafters-http-server LANGUAGES C)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(http-server ${SOURCE_FILES})

# Link Windows socket libraries if on Windows
if(WIN32)
    target_link_libraries(http-server ws2_32 wsock32)
endif()
