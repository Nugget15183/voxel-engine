cmake_minimum_required(VERSION 4.0)
project(voxel_engine)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(packages/glfw-3.5.1)
add_subdirectory(packages/glm)

add_library(glad STATIC packages/glad/src/glad.c)
target_include_directories(glad PUBLIC packages/glad/include)

set(EMBED_FILES
        resources/vert.glsl
        resources/frag.glsl
        resources/textures/terrain.png
)
set(EMBED_OUT ${CMAKE_BINARY_DIR}/generated/embedded.cpp)

set(EMBED_BODY "")
set(EMBED_TABLE "")
set(i 0)
foreach(f ${EMBED_FILES})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${f})

    file(READ ${CMAKE_SOURCE_DIR}/${f} hex HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")

    string(APPEND EMBED_BODY "static const unsigned char file${i}[] = {${bytes}0};\n")
    string(APPEND EMBED_TABLE "    {\"${f}\", file${i}, sizeof(file${i}) - 1},\n")
    math(EXPR i "${i}+1")
endforeach()

set(EMBED_CONTENT "#include \"embedded.h\"\n\n${EMBED_BODY}\nstatic const EmbeddedFile table[] = {\n${EMBED_TABLE}};\n\nconst EmbeddedFile* findEmbedded(const std::string& path) {\n    for (const auto& f : table) if (path == f.name) return &f;\n    return nullptr;\n}\n")
file(CONFIGURE OUTPUT ${EMBED_OUT} CONTENT "${EMBED_CONTENT}" @ONLY)

add_executable(voxel_engine
        src/main.cpp
        src/engine.cpp
        src/engine.h
        src/logger.cpp
        src/logger.h
        src/util.h
        src/stb_image.h
        src/terrain.cpp
        src/terrain.h
        src/stb_perlin.h
        src/worldgen.cpp
        src/worldgen.h
        src/embedded.h
        ${EMBED_OUT}
)
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm)

if (MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
endif()