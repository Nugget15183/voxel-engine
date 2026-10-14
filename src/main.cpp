#include <iostream>
#include "engine.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

int main() {
    //skybox
    //ui elements such as text
    //proper lighting
    //polishing
    //biomes + more block variety

    engine e;

    camera c = camera();
    c.pos={0.0f,130.0f,-3.0f};
    e.c=&c;

    if (e.init() != 0) {
        glfwTerminate();
        return -1;
    }

    while (!glfwWindowShouldClose(e.window)) {
        e.update();
        e.render();
    }
    glfwTerminate();
    return 0;
}
