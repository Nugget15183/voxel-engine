#include <iostream>
#include "engine.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main() {
    engine e;
    if (e.init() != 0) { glfwTerminate(); return -1; }

    vector<float> verts = {
        // positions          // colors           // texture coords
        0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
       -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
       -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left
   };

    object tri(
        e,
        false,
        verts, //ALSO CONTAINS VERTEX ATTRIB DATA
        {0,1,3, 1,2,3}
    );
    e.submitRenderable(tri);

    while (!glfwWindowShouldClose(e.window)) {
        e.update();
        e.render();
    }
    glfwTerminate();
    return 0;
}