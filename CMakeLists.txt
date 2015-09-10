cmake_minimum_required(VERSION 3.3)
project(untitled)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES main.c)

add_executable(untitled ${SOURCE_FILES})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES} GL)

