#pragma once
#include <string>

struct EmbeddedFile {
    const char* name;
    const unsigned char* data;
    size_t size;
};

// returns nullptr if the path wasn't embedded
const EmbeddedFile* findEmbedded(const std::string& path);