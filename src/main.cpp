#include <iostream>
#include "engine.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main() {
    engine e;
    if (e.init() != 0) {
        glfwTerminate();
        return -1;
    }

    camera c = camera();
    c.pos={0.0f,0.0f,-3.0f};
    e.c=&c;

    while (!glfwWindowShouldClose(e.window)) {
        e.update();
        e.render();
    }
    glfwTerminate();
    return 0;
}
