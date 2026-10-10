#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <vector>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "logger.h"

struct object {
    object(bool is2d) {
        this->is_2d=is2d;
    }

    vector<int> pos = {0,0,0};
    vector<int> rot = {0,0,0};

    bool is_2d = false;

    vector<float> verts = {};
    vector<float> indicies = {};
    vector<float> UVs = {};
};

class engine {
public:
    //initialize the engine
    int init();

    //draw objects
    void render();

    //submit 3d objects for rendering
    void submit3d();
    //submit ui elements for rendering
    void submit2d();

    //update ingame elements
    void update();

    //variables
    GLFWwindow* window;
private:
    bool initalized = false;
    logger log;
    vector<object> render_queue;
};


#endif //VOXEL_ENGINE_ENGINE_H