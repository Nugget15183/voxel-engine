#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <utility>
#include <vector>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "logger.h"
#include "util.h"

#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "terrain.h"

struct object;

struct camera {
    glm::vec3 pos={0,0,0};
    glm::vec2 rot={0,0.0f};//pitch, yaw

    float speed = 3.0f;

    glm::vec3 front() {
        glm::vec3 ret = glm::vec3(
            cos(glm::radians(rot.y)) * cos(glm::radians(rot.x)),
            sin(glm::radians(rot.x)),
            sin(glm::radians(rot.y)) * cos(glm::radians(rot.x))
        );

        return normalize(ret);
    }

    glm::mat4 view() {
        return glm::lookAt(pos, pos + front(), {0,1,0});
    }
};

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
    unsigned int shader_program;
    unsigned int textureAtlas;

    GLFWwindow* window;
    util utils;
    logger log;
    camera *c;

    double dt=0.00;
    float fps=0.0f;
    bool firstMouse = true;
    float sensitivity = 0.1f;

    vector<int> winSize = {800, 600};
    vector<double> mousePos = {0,0};
private:
    float lastframe;
    float lastfpsUpdate = (float)glfwGetTime();
    int framecount;

    bool initalized = false;

    vector<object> render_queue;
    unsigned int vert_shader;
    unsigned int frag_shader;
};

struct object {
    object(engine* e, bool is2d, vector<float> verts, vector<unsigned int> indi) {
        this->is_2d=is2d;
        this->verts=move(verts);
        this->e=move(e);
        this->indicies=move(indi);
    }

    engine* e;

    logger l = logger();

    glm::vec3 pos = {0,0,0};
    glm::vec3 rot = {0,0,0};

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

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        texture = e->textureAtlas;
        glBindVertexArray(0);
    }

    void draw() {
        glUseProgram(e->shader_program);

        //perspective projection
        glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)e->winSize[0]/e->winSize[1], 0.1f, 10000.0f);

        //view matrix
        glm::mat4 view = e->c->view();

        //model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pos.x, pos.y, pos.z));

        unsigned int modelLoc = glGetUniformLocation(e->shader_program, "model");
        unsigned int viewLoc = glGetUniformLocation(e->shader_program, "view");
        unsigned int projLoc = glGetUniformLocation(e->shader_program, "proj");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

        glBindTexture(GL_TEXTURE_2D, texture);//bind texture
        glBindVertexArray(VAO);//bind vao
        glDrawElements(GL_TRIANGLES, indicies.size(), GL_UNSIGNED_INT, 0);//draw
    }
};



#endif //VOXEL_ENGINE_ENGINE_H