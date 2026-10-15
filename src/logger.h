#ifndef VOXEL_ENGINE_LOGGER_H
#define VOXEL_ENGINE_LOGGER_H

#include <stdlib.h>
#include <string>

using namespace std;

class logger {
public:
    void info(const string& msg);
    void warning(const string& msg);
    void err(const string& msg);
};


#endif //VOXEL_ENGINE_LOGGER_H