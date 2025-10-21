#pragma region BASIC_INITIALIZATION

#include "main.hpp"
#include <unordered_map>
#include <functional>

using glm::vec3;
using std::vector;
using namespace Game;

int width = 1920, height = 1080;

float lastFrame = 0.0f;
float deltaTime = 0.0f;

vec3 cameraPos = vec3(0.0f, 0.0f, 3.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);

glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

vector<Game::Geometry*> geometryObjects;


#pragma endregion


#pragma region SHADERS
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out float depthVal;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);

    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    vec4 viewSpacePos = view * worldPos;
    depthVal = -viewSpacePos.z / 10.0;
    depthVal = clamp(depthVal, 0.0, 1.0);

    gl_Position = projection * viewSpacePos;
}

)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in float depthVal;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 objectColor;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
};
#define MAX_DIR_LIGHTS 4
uniform int numDirLights;
uniform DirectionalLight dirLights[MAX_DIR_LIGHTS];

uniform sampler2D texture1;
uniform float ambientStrength = 0.1;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 texColor = texture(texture1, TexCoords).rgb;

    vec3 resultColor = vec3(0.0);
    for (int i = 0; i < numDirLights; ++i) {
        vec3 lightDir = normalize(-dirLights[i].direction);
        float diff = max(dot(norm, lightDir), 0.0);

        vec3 halfway = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfway), 0.0), 32.0);

        vec3 diffuse = diff * dirLights[i].color;
        vec3 specular = spec * dirLights[i].color;

        resultColor += (diffuse + specular) * texColor;
    }

    resultColor += ambientStrength * texColor;

    vec3 depthGray = vec3(1.0 - depthVal);
    resultColor = mix(depthGray, resultColor, 0.8);

    FragColor = vec4(resultColor, 1.0);
}

)";

#pragma endregion


#pragma region GAME_OBJECTS
// redirected to hpp
#pragma endregion

#pragma region WINDOW_SIZE_MANAGEMENT

void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
    projection = glm::perspective(glm::radians(45.0f), (float)w / (float)h, 0.1f, 100.0f);
}

#pragma endregion

#pragma region MOUSE_CONTROLS

float yaw = -90.0f; // yaw is initialized to -90.0 degrees so that the camera points initially along -Z.
float pitch = 0.0f;
float lastX = width / 2.0;
float lastY = height / 2.0;
bool firstMouse = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static float sensitivity = 0.1f;
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

#pragma endregion

#pragma region KEYBOARD_CONTROLS 

void processInput(GLFWwindow* window) {
    float cameraSpeed = 2.5f * deltaTime;

    static const std::unordered_map<int, std::function<void()>> actions = {
        {GLFW_KEY_W, [&]() { cameraPos += cameraSpeed * cameraFront; }},
        {GLFW_KEY_S, [&]() { cameraPos -= cameraSpeed * cameraFront; }},
        {GLFW_KEY_A, [&]() { cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; }},
        {GLFW_KEY_D, [&]() { cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; }},
        {GLFW_KEY_Q, [&]() { cameraPos -= cameraUp * cameraSpeed; }},
        {GLFW_KEY_E, [&]() { cameraPos += cameraUp * cameraSpeed; }},
        {GLFW_KEY_ESCAPE, [&]() { glfwSetWindowShouldClose(window, true); }},
    };

    for (const auto& [key, action] : actions)
        if (glfwGetKey(window, key) == GLFW_PRESS)
            action();
}


#define DEBUG
#ifdef DEBUG
void processModelInput(GLFWwindow* window,Game::Geometry& model) {
    float moveSpeed = 2.5f * deltaTime;

    static const std::unordered_map<int, std::function<void()>> actions = {
        {GLFW_KEY_I, [&]() { model.transform.position += glm::vec3(0.0f, 0.0f, -moveSpeed); }},
        {GLFW_KEY_K, [&]() { model.transform.position += glm::vec3(0.0f, 0.0f,  moveSpeed); }},
        {GLFW_KEY_J, [&]() { model.transform.position += glm::vec3(-moveSpeed, 0.0f, 0.0f); }},
        {GLFW_KEY_L, [&]() { model.transform.position += glm::vec3(moveSpeed, 0.0f, 0.0f); }},
        {GLFW_KEY_U, [&]() { model.transform.position += glm::vec3(0.0f, -moveSpeed, 0.0f); }},
        {GLFW_KEY_O, [&]() { model.transform.position += glm::vec3(0.0f,  moveSpeed, 0.0f); }},
    };

    for (const auto& [key, action] : actions)
        if (glfwGetKey(window, key) == GLFW_PRESS)
            action();
}
#endif

#pragma endregion


#pragma region SHADER_UTILS
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // --- check compile status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Shader compile error (" << (type == GL_VERTEX_SHADER ? "VERT" : "FRAG") << "):\n" << infoLog << std::endl;
    }
    return shader;
}

unsigned int createShaderProgram() {
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    // --- check link status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "Program link error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

#pragma endregion


int main() {
#pragma region GLFW INIT
    if (!glfwInit()) {
        std::cerr << "GLFW failed to init.\n";
        return -1;
    }


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Basic Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
	glfwFocusWindow(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);
#pragma endregion
#pragma region GLAD INIT
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    // After window setup, set up rendering params and reqs
    glEnable(GL_DEPTH_TEST);
#pragma endregion

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe

    LightManager lightManager;

    // Add a directional “sun”
    DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    sun.color = glm::vec3(1.0f, 0.95f, 0.9f);
    lightManager.dirLights.push_back(sun);

    // Add a fill light
    DirectionalLight fill;
    fill.direction = glm::normalize(glm::vec3(1.0f, -0.5f, 0.0f));
    fill.color = glm::vec3(0.3f, 0.4f, 0.5f);
    lightManager.dirLights.push_back(fill);


    Game::Geometry* monkey = new Game::Geometry("C:\\Users\\tis\\Documents\\monkey.fbx",
        Game::Transform(
            vec3(0.0f),

            glm::quat(glm::radians(
                vec3(
                    -90.0f,
                    0.0f,
                    0.0f))),
            
            vec3(1.0f))
        );
	geometryObjects.push_back(monkey);

    Shader shader(vertexShaderSource, fragmentShaderSource);
    shader.use();
    shader.setVec3("viewPos", cameraPos);



    while (!glfwWindowShouldClose(window)) {
#pragma region GLSetup
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        processModelInput(window, *geometryObjects[0]);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Upload camera uniforms
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setVec3("viewPos", cameraPos);

        // Upload lights
        lightManager.uploadToShader(shader);
#pragma endregion

        for (Game::Geometry* geometry : geometryObjects) {
			geometry->draw(shader.ID);
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (Game::Geometry* geometry : geometryObjects) {
        delete geometry;
	}
    glfwTerminate();
    return 0;
}
