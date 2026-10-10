#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include "glad/glad.h"
#include "GLFW/glfw3.h"

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
private:
    bool initalized = false;
    GLFWwindow* window;
};


#endif //VOXEL_ENGINE_ENGINE_H