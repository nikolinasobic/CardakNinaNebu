#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

//***
void renderQuad();

unsigned int loadTexture(char const * path, bool gammaCorrection);



//***

// settings

//1920 1100
//800 600
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1100;
//***
bool bloom = true;
bool hdr = true;
bool hKEY_Pressed = false;
float exposure = 1.2f;
//***

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;




struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};




struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    DirLight dirLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

//***
glm::vec3 amb;
glm::vec3 diff;
glm::vec3 spec;
//***

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CardakNinaNebuNinaZemlji", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    //Face culling (ne renderuju se stranice koje se ne vide)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    //blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // kompajliranje sejdera
    //***
    Shader shader("resources/shaders/lighting.vs", "resources/shaders/lighting.fs");
    //***
    Shader ourShader("resources/shaders/model_shader.vs", "resources/shaders/model_shader.fs");
    Shader skyboxShader("resources/shaders/skyboxShader.vs", "resources/shaders/skyboxShader.fs");
    Shader hdrShader("resources/shaders/hdrShader.vs", "resources/shaders/hdrShader.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");



    //ucitavanje modela

    //dvorac
    stbi_set_flip_vertically_on_load(false);
    Model dvorac("resources/objects/castle/source/Castle/Castle.obj");
    dvorac.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    //ostrvo
    //stbi_set_flip_vertically_on_load(false);
    Model ostrvo("resources/objects/island/source/floating_island_exp3/floating_island_exp2/4.obj");
    ostrvo.SetShaderTextureNamePrefix("material.");
    //stbi_set_flip_vertically_on_load(true);

    //oblak
    stbi_set_flip_vertically_on_load(false);
    Model oblak("resources/objects/oblak4/scene.gltf");
    oblak.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    //drvo
    stbi_set_flip_vertically_on_load(false);
    Model drvo("resources/objects/drvo2/scene.gltf");
    drvo.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    //lampa
    stbi_set_flip_vertically_on_load(false);
    Model lampa("resources/objects/lampa2/scene.gltf");
    lampa.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

   //kula
    stbi_set_flip_vertically_on_load(false);
    Model kula("resources/objects/kula3/scene.gltf");
    kula.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    //kapija
    stbi_set_flip_vertically_on_load(false);
    Model kapija("resources/objects/kapija/scene.gltf");
    kapija.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    //lav
    stbi_set_flip_vertically_on_load(false);
    Model lav("resources/objects/lav/scene.gltf");
    lav.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    //oblaci
    Model oblaci("resources/objects/oblaci/scene.gltf");
    oblaci.SetShaderTextureNamePrefix("material.");



    // skybox - parametri
    float skyboxVertices[] = {
            // positions
            -100.0f,  100.0f, -100.0f,
            -100.0f, -100.0f, -100.0f,
            100.0f, -100.0f, -100.0f,
            100.0f, -100.0f, -100.0f,
            100.0f,  100.0f, -100.0f,
            -100.0f,  100.0f, -100.0f,

            -100.0f, -100.0f,  100.0f,
            -100.0f, -100.0f, -100.0f,
            -100.0f,  100.0f, -100.0f,
            -100.0f,  100.0f, -100.0f,
            -100.0f,  100.0f,  100.0f,
            -100.0f, -100.0f,  100.0f,

            100.0f, -100.0f, -100.0f,
            100.0f, -100.0f,  100.0f,
            100.0f,  100.0f,  100.0f,
            100.0f,  100.0f,  100.0f,
            100.0f,  100.0f, -100.0f,
            100.0f, -100.0f, -100.0f,

            -100.0f, -100.0f,  100.0f,
            -100.0f,  100.0f,  100.0f,
            100.0f,  100.0f,  100.0f,
            100.0f,  100.0f,  100.0f,
            100.0f, -100.0f,  100.0f,
            -100.0f, -100.0f,  100.0f,

            -100.0f,  100.0f, -100.0f,
            100.0f,  100.0f, -100.0f,
            100.0f,  100.0f,  100.0f,
            100.0f,  100.0f,  100.0f,
            -100.0f,  100.0f,  100.0f,
            -100.0f,  100.0f, -100.0f,

            -100.0f, -100.0f, -100.0f,
            -100.0f, -100.0f,  100.0f,
            100.0f, -100.0f, -100.0f,
            100.0f, -100.0f, -100.0f,
            -100.0f, -100.0f,  100.0f,
            100.0f, -100.0f,  100.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    vector<std::string> faces_sky {
            FileSystem::getPath("resources/textures/skybox/px.png"),
            FileSystem::getPath("resources/textures/skybox/nx.png"),
            FileSystem::getPath("resources/textures/skybox/py.png"),
            FileSystem::getPath("resources/textures/skybox/ny.png"),
            FileSystem::getPath("resources/textures/skybox/pz.png"),
            FileSystem::getPath("resources/textures/skybox/nz.png")
    };

    stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTextureSky = loadCubemap(faces_sky);
    stbi_set_flip_vertically_on_load(true);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);



    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-4.0f, -25.0f, -19.0f);
    pointLight.ambient = glm::vec3(1.1f, 1.2f, 1.3f);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    amb = glm::vec3(pointLight.ambient);
    diff = glm::vec3(pointLight.diffuse);
    spec = glm::vec3(pointLight.specular);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    DirLight& dirLight = programState->dirLight;

    dirLight.direction = glm::vec3(-2.0f, -1.0f, -0.3f);
    dirLight.ambient = glm::vec3(1.0f, 1.0f, 1.0f);
    dirLight.diffuse = glm::vec3(1, 0.7, 0.1);
    dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);

//***
    unsigned int rockTexture = loadTexture(FileSystem::getPath("resources/objects/island/source/floating_island_exp3/floating_island_exp2/mountainRock.jpeg").c_str(), true); // note that we're loading the texture as an SRGB texture

//***

    //***
// configure floating point framebuffer
    // ------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
        );
    }
//    glBindTexture(GL_TEXTURE_2D, colorBuffer);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    // create depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    // attach buffers
    //glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    //if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    //    std::cout << "Framebuffer not complete!" << std::endl;
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

//    shader.use();
//    shader.setInt("diffuseTexture", 0);
//    hdrShader.use();
//    hdrShader.setInt("hdrBuffer", 0);
    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);
    hdrShader.setInt("bloomBlur", 1);
    blurShader.use();
    blurShader.setInt("image", 0);
    //***



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------

        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //***
        // 1. render scene into floating point framebuffer
        // -----------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rockTexture);
        // set lighting uniforms

        //
        std::vector<glm::vec3> lightPositions;
        lightPositions.push_back(glm::vec3( pointLight.position));
        // colors
        std::vector<glm::vec3> lightColors;
        lightColors.push_back(glm::vec3(pointLight.ambient));
        //

        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            shader.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            shader.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
        }
        shader.setVec3("viewPos", programState->camera.Position);

        //***

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);


        //renderovanje modela

        //dvorac
        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::translate(model, glm::vec3(-6.5f, -20.0f, 18.0f));
        //model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0f, 0, 0));
        model = glm::translate(model, glm::vec3(0.0f,-21.0f, 0.0f));
        model = glm::translate(model, glm::vec3(-5.0f,0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f,0.0f, -23.0f));
        model = glm::scale(model, glm::vec3(0.93f));
        //model = glm::scale(model, glm::vec3(0.7f));
        ourShader.setMat4("model", model);
        dvorac.Draw(ourShader);

        //ostrvo
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f,50.0f, 0.0f));
        model = glm::translate(model, glm::vec3(-20.0f,0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f,0.0f, -10.0f));

        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1.0f, 0));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);
        ostrvo.Draw(ourShader);

        //drvo
        glDisable(GL_CULL_FACE);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-12.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, -30.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -18.0f));
        model = glm::scale(model, glm::vec3(0.04f));
        ourShader.setMat4("model", model);
        drvo.Draw(ourShader);
        glEnable(GL_CULL_FACE);

//        //lampa
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, -29.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -18.0f));
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 1.0f, 0));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0));
        model = glm::rotate(model, glm::radians(-180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(2.0f));
        ourShader.setMat4("model", model);
        lampa.Draw(ourShader);
//
//
//        //klupa
//        model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(-13.0f, 0.0f, 0.0f));
//        model = glm::translate(model, glm::vec3(0.0f, -37.0f, 0.0f));
//        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -30.0f));
//        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0));
//        model = glm::scale(model, glm::vec3(1.6f));
//        ourShader.setMat4("model", model);
//        klupa.Draw(ourShader);


        //kula
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-14.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, -37.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -29.0f));
        model = glm::scale(model, glm::vec3(0.8f));
        ourShader.setMat4("model", model);
        kula.Draw(ourShader);


        //kapija
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-7.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, -27.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -19.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0));
        model = glm::scale(model, glm::vec3(2.0f));
        ourShader.setMat4("model", model);
        kapija.Draw(ourShader);


        //lav
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-7.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, -29.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -17.0f));
        model = glm::scale(model, glm::vec3(0.02f));
        ourShader.setMat4("model", model);
        lav.Draw(ourShader);

        //oblaci
//        model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(-7.0f, 0.0f, 0.0f));
//        model = glm::translate(model, glm::vec3(0.0f, -29.0f, 0.0f));
//        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -17.0f));
//        model = glm::scale(model, glm::vec3(0.02f));
//        ourShader.setMat4("model", model);
//        oblaci.Draw(ourShader);



        //skybox se renderuje poslednji

        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureSky);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        glBindFramebuffer(GL_FRAMEBUFFER, 0);




        //***
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
//            glBindVertexArray(quadVAO);
//            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//            glBindVertexArray(0);
                renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
//        hdrShader.setInt("hdr", hdr);
//        hdrShader.setFloat("exposure", exposure);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setInt("bloom", bloom);
        hdrShader.setInt("hdr", hdr);
        hdrShader.setFloat("exposure", exposure);
//        glBindVertexArray(quadVAO);
//        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//        glBindVertexArray(0);
        renderQuad();

        //std::cout << "hdr: " << (hdr ? "on" : "off") << "| exposure: " << exposure << std::endl;
        //***

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}
















// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

//glm::vec3 amb = glm::vec3(programState->pointLight.ambient);
//glm::vec3 diff = glm::vec3(programState->pointLight.diffuse);
//glm::vec3 spec = glm::vec3(programState->pointLight.specular);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }


    }

    //podesavanje lampe da radi na klik
    
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        if (programState->pointLight.ambient != glm::vec3(0.0f)
            && programState->pointLight.diffuse != glm::vec3(0.0f)
            && programState->pointLight.specular != glm::vec3(0.0f) ) {

            programState->pointLight.ambient = glm::vec3(0.0f);
            programState->pointLight.diffuse = glm::vec3(0.0f);
            programState->pointLight.specular = glm::vec3(0.0f);

        } else {
            programState->pointLight.ambient = glm::vec3(amb);
            programState->pointLight.diffuse = glm::vec3(diff);
            programState->pointLight.specular = glm::vec3(spec);
        }
    }
}


unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


//***
// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}



//***