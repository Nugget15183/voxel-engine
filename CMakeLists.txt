cmake_minimum_required(VERSION 4.0)
project(voxel_engine)

set(CMAKE_CXX_STANDARD 20)

# dev mode
option(EMBED_RESOURCES "Embed resources and link statically (release/standalone build)" OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if (EMBED_RESOURCES AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# dependencies
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(packages/glfw-3.5.1)
add_subdirectory(packages/glm)

add_library(glad STATIC packages/glad/src/glad.c)
target_include_directories(glad PUBLIC packages/glad/include)

# exec
add_executable(${PROJECT_NAME}
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
        src/cavecarver.h
)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm)

# resources
set(EMBED_FILES
        resources/vert.glsl
        resources/frag.glsl
        resources/textures/terrain.png
)

if (EMBED_RESOURCES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE EMBED_RESOURCES)

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

    target_sources(${PROJECT_NAME} PRIVATE ${EMBED_OUT})

    if (MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
else()
    # dev mode
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            RESOURCE_DIR="${CMAKE_SOURCE_DIR}/resources/")
endif()