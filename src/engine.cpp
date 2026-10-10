#include "engine.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "logger.h"

void setWindowCallbacks(GLFWwindow* win) {
    glfwSetFramebufferSizeCallback(win, [](GLFWwindow* window, int width, int height) -> void {
        glViewport(0, 0, width, height);
    });
}

int engine::init() {
    log=logger();

    log.info("Initalizing...");

    if (!glfwInit()!=0) {log.err("GLFW Failed to initalize."); return -1;}

    //window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //window creation
    window = glfwCreateWindow(800, 600, "Voxel engine", NULL, NULL);
    if (window == nullptr) {log.err("GLFW Failed to create a window."); glfwTerminate(); return -1;}
    glfwMakeContextCurrent(window);

    //load glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {log.err("GLAD Failed to load."); return -1;}

    glViewport(0, 0, 800, 600);//size of the window
    setWindowCallbacks(window);

    log.info("Initalized successfully");
    return 0;
}

void engine::render() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void engine::submit3d() {

}

void engine::submit2d() {

}

void engine::update() {

}

