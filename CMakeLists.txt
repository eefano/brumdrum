cmake_minimum_required(VERSION 3.3)
project(brumdrum)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -O2")

set(SOURCE_FILES main.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Threads)
find_package(PkgConfig REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)
pkg_search_module(jack REQUIRED jack)

include_directories(${GLFW_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${jack_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES}
        ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${jack_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

