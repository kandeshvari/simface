cmake_minimum_required(VERSION 3.7)
project(simface)

include_directories(
        /home/dk/.pebble-sdk/SDKs/4.3/sdk-core/pebble/diorite/include
)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES
        src/c/main.c
        )
add_executable(simface ${SOURCE_FILES})
