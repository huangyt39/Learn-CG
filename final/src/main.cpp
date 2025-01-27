#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>

#include "camera/Camera.h"
#include "manager/manager.h"
#include "object/Object.h"
#include "object/constants/Positions.h"
#include "object/constants/Vertices.h"
#include "player/Player.h"
#include "shader/Shader.h"
#include "skybox/skybox.h"
#include "utils/utils.h"
#include "text/Text.h"
#include "particle/particle.h"
#include "explosion/Explosion.h"

#include FT_FREETYPE_H  

using namespace std;

// 设置窗口大小
const unsigned int SCR_WIDTH = 1080;
const unsigned int SCR_HEIGHT = 700;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 初始化摄像机
Camera camera(glm::vec3(0.0f, 7.0f, 7.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

const float movementSpeed = 3.0f;
const float mouseSensitivity = 0.2f;

float deltaTime = 0.0f;  // time between current frame and last frame
float lastFrame = 0.0f;

// restart
bool restart = false;

// 创建manager
Manager manager = Manager();

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.moveForward(deltaTime * movementSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.moveBack(deltaTime * movementSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.moveLeft(deltaTime * movementSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.moveRight(deltaTime * movementSpeed);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.moveUp(deltaTime * movementSpeed);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.moveDown(deltaTime * movementSpeed);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods) {
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) restart = true;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        manager.playerMove(Forward);
    else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        manager.playerMove(Left);
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        manager.playerMove(Back);
    else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        manager.playerMove(Right);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.rotate(xoffset * mouseSensitivity, yoffset * mouseSensitivity);
}

void renderTitle() {
    static Model font("assets/title.fbx");
    static Shader fontShader("glsl/font.vs.glsl", "glsl/font.fs.glsl");
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f,
        100.0f);

    fontShader.use();
    fontShader.setMat4("view", camera.GetViewMatrix());
    fontShader.setMat4("projection", projection);
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 5.0f, -5.0f));
    fontShader.setMat4("model", model);
    font.Draw(fontShader, 0, false);
}

int main() {
    // 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // 主版本号 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // 次版本号 3
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                   GLFW_OPENGL_CORE_PROFILE);  // 使用核心模式

    // 创建一个窗口对象
    GLFWwindow* window =
        glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sokoban", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // 将窗口上下文设置为主上下文
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);

    //// 告知 GLFW 捕捉鼠标动作
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 创建着色器
    Shader shader("glsl/shader.vs.glsl", "glsl/shader.fs.glsl");
    Shader depthShader("glsl/depth.vs.glsl", "glsl/depth.fs.glsl");
    Shader skyboxShader("glsl/skyboxShader.vs.glsl",
                        "glsl/skyboxShader.fs.glsl");
    Shader playerShader("glsl/player.vs.glsl", "glsl/player.fs.glsl");
	Shader textShader("glsl/text.vs.glsl", "glsl/text.fs.glsl");
	Shader particleShader("glsl/particle.vs.glsl", "glsl/particle.fs.glsl");

    // 加载纹理
    unsigned int groundTexture = loadTexture("assets/grass.png");
    unsigned int wallTexture = loadTexture("assets/wall.png");
    unsigned int boxTexture = loadTexture("assets/box.jpg");
    unsigned int dirtTexture = loadTexture("assets/dirt.png");
    unsigned int endTexture = loadTexture("assets/end.png");

    // Configure depth map FBO
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                 SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 创建地图
    createMap();

    // 创建对象
    vector<Object> ground = createObjects(
        planeVertices, vector<unsigned int>{groundTexture, depthMap},
        groundPositions);
    vector<Object> wall =
        createObjects(cubeVertices, vector<unsigned int>{wallTexture, depthMap},
                      wallPositions);
    vector<Object> box = createObjects(
        cubeVertices, vector<unsigned int>{boxTexture, depthMap}, boxPositions);
    vector<Object> dirt =
        createObjects(cubeVertices, vector<unsigned int>{dirtTexture, depthMap},
                      dirtPositions);
    vector<Object> end =
        createObjects(planeVertices, vector<unsigned int>{endTexture, depthMap},
                      endPositions);

    // player
    Player* player = Player::getInstance("assets/sheep.obj",
                                         SCR_WIDTH, SCR_HEIGHT, depthMap);
    manager.init(&wall, &box, player);

	// text
	Text text = Text();
	int frameCount = 20;
    string fps("FPS: ");

    //创建天空盒
    Skybox skybox(&skyboxShader);

	// 创建粒子系统
	ParticleGenerator Particles(300);

    Explosion explosions(particleShader);

    // 配置着色器
    shader.use();
    shader.setInt("diffuseTexture", 0);
    shader.setInt("shadowMap", 1);

    glm::vec3 lightPos(-2.0f, 7.0f, 2.0f);

    // 渲染
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
		processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (restart) {
            manager.resetObjsPos();
            explosions.reset();
            restart = false;
        }
        // 1. render depth of scene to texture (from light's perspective)
        // --------------------------------------------------------------
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 15.0f);
        lightView =
            glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        // render scene from light's point of view
        depthShader.use();
        depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        renderObjects(ground, &depthShader, false);
        renderObjects(dirt, &depthShader, false);
        renderObjects(wall, &depthShader, false);
        renderObjects(box, &depthShader, false);
        renderObjects(end, &depthShader, false);

        player->render(&depthShader, lightPos, false);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // reset viewport
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. render scene as normal using the generated depth/shadow map
        // --------------------------------------------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f,
            100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        // set light uniforms
        shader.setVec3("viewPos", camera.Position);
        shader.setVec3("lightPos", lightPos);
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        renderObjects(ground, &shader);
        renderObjects(dirt, &shader);
        renderObjects(wall, &shader);
        renderObjects(box, &shader);
        renderObjects(end, &shader);

		if (manager.isGameOver()) {
			 //渲染粒子
			particleShader.use();
			particleShader.setMat4("projection", projection);
			particleShader.setMat4("view", view);
			Particles.Update(0.05f, 500);
			Particles.Draw(deltaTime, particleShader, glm::vec3(-2.5f, 0.0f, -0.5f));
			Particles.Draw(deltaTime, particleShader, glm::vec3(-2.5f, 0.0f, -1.5f));
			Particles.Draw(deltaTime, particleShader, glm::vec3(-2.5f, 0.0f, -2.5f));
            Particles.Draw(deltaTime, particleShader, player->position);

            //explosion
            explosions.Update(deltaTime, 1);
            explosions.Draw(glm::vec3(-2.5f, 0.0f, -0.5f));
            explosions.Draw(glm::vec3(-2.5f, 0.0f, -1.5f));
            explosions.Draw(glm::vec3(-2.5f, 0.0f, -2.5f));
		}

        playerShader.use();
        player->setView(camera.GetViewMatrix());
        player->render(&playerShader, lightPos);

        // 渲染天空盒
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skybox.render(view, projection);

        renderTitle();

		// 渲染文本
		textShader.use();
		projection = glm::ortho(0.0f, static_cast<GLfloat>(SCR_WIDTH), 0.0f, static_cast<GLfloat>(SCR_HEIGHT));
		textShader.setMat4("projection", projection);

		if (frameCount == 20) {
			fps = string("FPS: ") + std::to_string((int)(1 / deltaTime));
			frameCount = 0;
		} else { 
			++frameCount; 
		}
		text.RenderText(textShader, fps, 25.0f, 25.0f, 0.7f, glm::vec3(0.5, 0.8f, 0.2f));


        // 检查并调用事件，交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}