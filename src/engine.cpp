#include "engine.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "logger.h"
#include "util.h"
#include "terrain.h"
#include <algorithm>

void loadTerrain(engine* e) {
    e->t = new terrain();
    e->t->renderDistance = 8;

    float start = (float)glfwGetTime();
    e->log.info("Generating terrain...");

    //wait till the starting chunk is loaded
    int ccx = (int)floor(e->c->pos.x / CX);
    int ccz = (int)floor(e->c->pos.z / CZ);

    while (!e->t->getChunk(ccx, ccz)) {
        e->t->updateAround(e, e->c->pos, 4);
        this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    e->log.info("First chunk ready after " + std::to_string(glfwGetTime() - start) + "s");
}

void setWindowCallbacks(GLFWwindow* win) {
    glfwSetFramebufferSizeCallback(win, [](GLFWwindow* window, int width, int height) -> void {
        glViewport(0, 0, width, height);

        auto* e = static_cast<engine*>(glfwGetWindowUserPointer(window));
        e->winSize={width, height};
    });

    glfwSetCursorPosCallback(win, [](GLFWwindow* window, double x, double y) -> void {
        auto* e = static_cast<engine*>(glfwGetWindowUserPointer(window));

        if (!e->mCaptured) return;

        if (e->firstMouse) {
            e->firstMouse = false;
            e->mousePos={x,y};
            return;
        }

        float dx = (float)(x - e->mousePos[0]);
        float dy = (float)(e->mousePos[1] - y);
        e->mousePos={x,y};

        e->c->rot.x = glm::clamp(e->c->rot.x + dy * e->sensitivity, -89.0f, 89.0f);
        e->c->rot.y += dx * e->sensitivity;
    });
}

void handleKeyInput(GLFWwindow* win) {
    auto* e = static_cast<engine*>(glfwGetWindowUserPointer(win));

    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        auto* e = static_cast<engine*>(glfwGetWindowUserPointer(win));

        e->mCaptured=!e->mCaptured;

        if (e->mCaptured) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    float speed = e->c->speed*(float)e->dt;
    glm::vec3 front = e->c->front();
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) e->c->pos += front * speed;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) e->c->pos -= front * speed;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) e->c->pos -= right * speed;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) e->c->pos += right * speed;
}

int engine::init() {
    log=logger();
    utils=util();

    log.info("Initalizing...");

    if (!glfwInit()) {log.err("GLFW Failed to initalize."); return -1;}

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

    //Shader initalization
    string rv = utils.readfile("resources/vert.glsl");
    string rf =  utils.readfile("resources/frag.glsl");
    const GLchar *vert_src = rv.c_str();
    const GLchar *frag_src =rf.c_str();

    vert_shader=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &vert_src, NULL);
    glCompileShader(vert_shader);

    frag_shader=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_src, NULL);
    glCompileShader(frag_shader);

    //confirm vertex shader compiled successfully
    int vsucc;
    char vinfo_log[512];
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &vsucc);
    if (!vsucc) {log.err("Failed to compile vertex shader: " + string(vinfo_log)); return -1;}

    //confirm frag shader compiled successfully
    int fsucc;
    char finfo_log[512];
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &fsucc);
    if (!fsucc) {log.err("Failed to compile fragment shader: " + string(finfo_log)); return -1;}

    //create shader program and link
    shader_program=glCreateProgram();
    glAttachShader(shader_program, vert_shader);
    glAttachShader(shader_program, frag_shader);
    glLinkProgram(shader_program);

    //link shaders and confirm success
    int lsucc;
    char linfo_log[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &lsucc);
    if (!lsucc) {log.err("Failed to link shaders: " + string(linfo_log)); return -1;}

    //opengl settings and misc stuff
    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, this);

    textureAtlas=utils.genTexture("resources/textures/terrain.png");

    loadTerrain(this);

    log.info("Initalized successfully");
    return 0;
}

void engine::render() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) log.err("GL error: " + std::to_string(err));

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // opaque pass
    for (auto& [id, o] : opaqueRenderables) o.draw();

    // transparent pass
    std::vector<object*> sorted;
    sorted.reserve(transparentRenderables.size());
    for (auto& [id, o] : transparentRenderables) sorted.push_back(&o);

    glm::vec3 camPos = c->pos;
    std::sort(sorted.begin(), sorted.end(), [&](object* a, object* b) {
        glm::vec3 da = a->sortPos - camPos, db = b->sortPos - camPos;
        return glm::dot(da, da) > glm::dot(db, db);   // farthest first
    });

    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    for (object* o : sorted) o->draw();
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

int engine::submitOpaqueRenderable(object o) {
    int id = nextRenderId++;
    auto& slot = opaqueRenderables.emplace(id, std::move(o)).first->second;
    slot.create();
    return id;
}

void engine::removeOpaqueRenderable(int id) {
    auto it = opaqueRenderables.find(id);
    if (it == opaqueRenderables.end()) return;
    it->second.destroy();
    opaqueRenderables.erase(it);
}

int engine::submitTRenderable(object o) {
    int id = nextRenderId++;
    auto& slot = transparentRenderables.emplace(id, std::move(o)).first->second;
    slot.create();
    return id;
}

void engine::removeTRenderable(int id) {
    auto it = transparentRenderables.find(id);
    if (it == transparentRenderables.end()) return;
    it->second.destroy();
    transparentRenderables.erase(it);
}

void engine::update() {
    handleKeyInput(window);

    float now = (float)glfwGetTime();
    dt=now-lastframe;

    this->t->updateAround(this, this->c->pos, 2);
    this->t->remeshDirty(this, 2);

    if (lastfpsUpdate+1<now) {
        fps=framecount;
        framecount=0;
        lastfpsUpdate=now;
        log.info("FPS: " + std::to_string(fps));
    }

    lastframe=now;
    framecount++;
}