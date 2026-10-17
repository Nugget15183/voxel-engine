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

//comment

struct object;

struct camera {
    glm::vec3 pos={0,0,0};
    glm::vec2 rot={0,0.0f};//pitch, yaw

    float speed = 100.0f;

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

    std::unordered_map<int, object> opaqueRenderables;
    std::unordered_map<int, object> transparentRenderables;

    int nextRenderId = 0;

    int  submitOpaqueRenderable(object o);
    void removeOpaqueRenderable(int id);
    int  submitTRenderable(object o);
    void removeTRenderable(int id);

    //update ingame elements
    void update();

    //variables
    unsigned int shader_program=0;
    unsigned int textureAtlas=0;

    unsigned int viewLoc=0;
    unsigned int projLoc=0;
    unsigned int camPosLoc=0;

    GLFWwindow* window;
    util utils;
    logger log;
    camera *c;
    terrain* t;

    double dt=0.00;
    float fps=0.0f;
    bool firstMouse = true;
    bool mCaptured = true;
    float sensitivity = 0.1f;

    vector<int> winSize = {800, 600};
    vector<double> mousePos = {0,0};
private:
    float lastframe = 0.0f;
    float lastfpsUpdate = (float)glfwGetTime();
    int framecount=0;

    bool initalized = false;
    unsigned int vert_shader=0;
    unsigned int frag_shader=0;
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

    unsigned int VBO=0;
    unsigned int VAO=0;
    unsigned int EBO=0;
    unsigned int texture = 0;

    glm::vec3 sortPos = {0,0,0};

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
        GLsizei stride = 11 * sizeof(float);

        //non-int attribs
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(4);

        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)(10 * sizeof(float)));
        glEnableVertexAttribArray(5);


        texture = e->textureAtlas;
        glBindVertexArray(0);
    }

    void destroy() {
        if (EBO) { glDeleteBuffers(1, &EBO);      EBO = 0; }
        if (VBO) { glDeleteBuffers(1, &VBO);      VBO = 0; }
        if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    }

    void draw(glm::mat4 proj, glm::mat4 view) {
        glBindTexture(GL_TEXTURE_2D, texture);//bind texture
        glBindVertexArray(VAO);//bind vao
        glDrawElements(GL_TRIANGLES, indicies.size(), GL_UNSIGNED_INT, 0);//draw
    }
};

#endif //VOXEL_ENGINE_ENGINE_H