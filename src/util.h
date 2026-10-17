#pragma once

#ifndef VOXEL_ENGINE_UTIL_H
#define VOXEL_ENGINE_UTIL_H

#include <string>
#include <fstream>

#include "glad/glad.h"
#include "logger.h"
#include "stb_image.h"
#include "embedded.h"

using namespace std;

//change resource lookup mode based on dev build vs release
#ifdef EMBED_RESOURCES
const inline EmbeddedFile* lookupEmbedded(string& path) {return findEmbedded(path);}
#else
inline EmbeddedFile* lookupEmbedded(string&) {return nullptr;}
#endif

inline string diskpath(string& path) {
#ifdef RESOURCE_DIR
    string prefix = "resources/";
    if (path.rfind(prefix, 0) == 0) return string(RESOURCE_DIR) + path.substr(prefix.size());
#endif
    return path;
}

class util {
public:
    logger log = logger();

    string readfile(string path) {
        //embedded copy
        if (const EmbeddedFile* f = lookupEmbedded(path))
            return string((const char*)f->data, f->size);

        //disk
        std::ifstream file(diskpath(path));
        if (!file) log.err("Failed to open file: " + path);
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        return content;
    }

    unsigned int genTexture(string path) {
        int width, height, nrChannels;
        unsigned char* data = nullptr;

        if (const EmbeddedFile* f = lookupEmbedded(path))
            data = stbi_load_from_memory(f->data, (int)f->size, &width, &height, &nrChannels, 0);
        else
            data = stbi_load(diskpath(path).c_str(), &width, &height, &nrChannels, 0);

        if (!data) {
            log.err("Failed to load texture: " + path);
            return 0;
        }

        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);
        return texture;
    }
};

#endif //VOXEL_ENGINE_UTIL_H