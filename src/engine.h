#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <utility>
#include <vector>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "logger.h"
#include "util.h"

struct object;

class engine {
public:
    //initialize the engine
    int init();

    //draw objects
    void render();

    //submit objects for rendering
    void submitRenderable(object);

    //update ingame elements
    void update();

    //variables
    GLFWwindow* window;
    unsigned int shader_program;

    util utils;
private:
    bool initalized = false;
    logger log;

    vector<object> render_queue;
    unsigned int vert_shader;
    unsigned int frag_shader;
};

struct object {
    object(engine e, bool is2d, vector<float> verts, vector<unsigned int> indi) {
        this->is_2d=is2d;
        this->verts=move(verts);
        this->e=move(e);
        this->indicies=move(indi);
    }

    engine e;

    vector<int> pos = {0,0,0};
    vector<int> rot = {0,0,0};

    vector<float> verts = {};
    vector<unsigned int> indicies = {};

    bool is_2d = false;

    unsigned int VBO;
    unsigned int VAO;
    unsigned int EBO;
    unsigned int texture = 0;

    //call once before rendering
    void create() {
        //gen buffers
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenVertexArrays(1, &VAO);

        //bind the generated buffers
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(unsigned int), indicies.data(), GL_STATIC_DRAW);

        //set vertex attributes
        GLsizei stride = 8 * sizeof(float);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        texture = e.utils.genTexture("resources/textures/noTexture.png");//temp no texture
        glBindVertexArray(0);
    }

    void draw() {
        glUseProgram(e.shader_program);

        glBindTexture(GL_TEXTURE_2D, texture);//bind texture
        glBindVertexArray(VAO);//bind vao
        glDrawElements(GL_TRIANGLES, indicies.size(), GL_UNSIGNED_INT, 0);//draw
    }
};

#endif //VOXEL_ENGINE_ENGINE_H