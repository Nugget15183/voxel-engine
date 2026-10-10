#ifndef VOXEL_ENGINE_LOGGER_H
#define VOXEL_ENGINE_LOGGER_H

#include <stdlib.h>
#include <string>

using namespace std;

class logger {
public:
    void info(string msg);
    void warning(string msg);
    void err(string msg);
};


#endif //VOXEL_ENGINE_LOGGER_H