cmake_minimum_required(VERSION 4.0)
project(voxel_engine)

set(CMAKE_CXX_STANDARD 20)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(packages/glfw-3.5.1)
add_subdirectory(packages/glm)

add_library(glad STATIC packages/glad/src/glad.c)
target_include_directories(glad PUBLIC packages/glad/include)

add_executable(voxel_engine src/main.cpp
        src/engine.cpp
        src/engine.h
        src/logger.cpp
        src/logger.h
        src/util.h
        src/stb_image.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm)