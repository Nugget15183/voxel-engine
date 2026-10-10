#include <iostream>
#include "engine.h"

int main() {
    engine e = engine();

    if (e.init()==0) {
        while (!glfwWindowShouldClose(e.window)) {
            e.update();
            e.render();
        }
    }

    glfwTerminate();
    return 0;
}