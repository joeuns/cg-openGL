#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Library for loading JPG/PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace glm;

// --- Global Variables: Camera Control ---
vec3 position = vec3(0, 1, 12);
float horizontalAngle = 3.14f; 
float verticalAngle = -0.05f; 
float speed = 3.0f;
float mouseSpeed = 0.002f;

mat4 ViewMatrix;
mat4 ProjectionMatrix;

// Callback function to adjust the viewport when the window is resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Process keyboard input
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// --- Utility: Texture Loader ---
GLuint loadTexture(const char* path) {
    int width, height, nrChannels;
    // Flip image vertically before loading
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (!data) {
        std::cout << "!! Texture Load Failed: " << path << " - File not found !!" << std::endl;
        std::cout << "Check if the file is in the same folder as the .cpp source." << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Auto-set format based on channels
    GLenum format = GL_RGB;
    if (nrChannels == 4) format = GL_RGBA;
    else if (nrChannels == 1) format = GL_RED;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    std::cout << "Success: " << path << " loaded (" << width << "x" << height << ")" << std::endl;
    return textureID;
}

// --- Utility: Shader Loader ---
GLuint LoadShaders(const char* v_path, const char* f_path) {
    std::string v_code;
    std::ifstream v_stream(v_path, std::ios::in);
    if (v_stream.is_open()) { std::stringstream sstr; sstr << v_stream.rdbuf(); v_code = sstr.str(); }

    std::string f_code;
    std::ifstream f_stream(f_path, std::ios::in);
    if (f_stream.is_open()) { std::stringstream sstr; sstr << f_stream.rdbuf(); f_code = sstr.str(); }

    GLuint v_id = glCreateShader(GL_VERTEX_SHADER);
    const char* v_src = v_code.c_str();
    glShaderSource(v_id, 1, &v_src, NULL);
    glCompileShader(v_id);

    GLuint f_id = glCreateShader(GL_FRAGMENT_SHADER);
    const char* f_src = f_code.c_str();
    glShaderSource(f_id, 1, &f_src, NULL);
    glCompileShader(f_id);

    GLuint p_id = glCreateProgram();
    glAttachShader(p_id, v_id);
    glAttachShader(p_id, f_id);
    glLinkProgram(p_id);

    glDeleteShader(v_id); glDeleteShader(f_id);
    return p_id;
}

// --- Input Processing: Keyboard & Mouse ---
void computeInputs(GLFWwindow* window) {
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwSetCursorPos(window, 800 / 2, 600 / 2);

    horizontalAngle += mouseSpeed * float(800 / 2 - xpos);
    verticalAngle += mouseSpeed * float(600 / 2 - ypos);

    vec3 dir(cos(verticalAngle) * sin(horizontalAngle), sin(verticalAngle), cos(verticalAngle) * cos(horizontalAngle));
    vec3 right = vec3(sin(horizontalAngle - 3.14f / 2.0f), 0, cos(horizontalAngle - 3.14f / 2.0f));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += dir * deltaTime * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= dir * deltaTime * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * deltaTime * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * deltaTime * speed;

    ProjectionMatrix = perspective(radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    ViewMatrix = lookAt(position, position + dir, vec3(0, 1, 0));

    lastTime = currentTime;
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "HW2: Cat & Cubes", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST);

    // Create VAO (Required)
    GLuint VAO; glGenVertexArrays(1, &VAO); glBindVertexArray(VAO);

    GLuint programID = LoadShaders("Transform.vertexshader", "Texture.fragmentshader");
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    // Load Textures
    GLuint tex1 = loadTexture("texture1.jpg");
    GLuint tex2 = loadTexture("texture2.jpg");
    GLuint texFur = loadTexture("fur.jpg");     // Body (Side, Back, Top, Bottom)
    GLuint texFace = loadTexture("face.jpg");   // Front face (Face features)
    GLuint texEar = loadTexture("ear.jpg");     // Ear triangles

    if (programID == 0) printf("Shader Load Failed!\n");
    if (tex1 == 0) printf("texture1.jpg Load Failed!\n");

    // 1. Cube Data
    GLfloat cube_verts[] = {
        -1.0f,-1.0f,-1.0f, -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,
         1.0f, 1.0f,-1.0f,  1.0f,-1.0f,-1.0f, -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f, -1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f, -1.0f,-1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,  1.0f, 1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
         1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f
    };

    GLfloat cube_uvs[] = {
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 0.0f,1.0f, 1.0f,0.0f, 1.0f,1.0f,
        0.0f,1.0f, 1.0f,0.0f, 0.0f,0.0f, 0.0f,1.0f, 1.0f,1.0f, 0.0f,0.0f,
        0.0f,0.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,1.0f, 1.0f,1.0f, 1.0f,0.0f,
        1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f, 1.0f,1.0f, 0.0f,1.0f, 1.0f,0.0f,
        1.0f,0.0f, 0.0f,1.0f, 0.0f,0.0f, 1.0f,0.0f, 0.0f,1.0f, 0.0f,0.0f,
        1.0f,0.0f, 0.0f,1.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f,
        1.0f,1.0f, 0.0f,0.0f, 1.0f,0.0f
    };

    // 2. Cat Data: Body (0-29), Front Face (30-35), Ears (36-41)
    GLfloat cat_verts[] = {
        // [0-5] Left (-X)
        -1.0f,-1.0f,-1.0f, -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f,
        // [6-11] Back (-Z)
         1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
        // [12-17] Bottom (-Y)
        1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,
        1.0f,-1.0f, 1.0f, -1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,
         // [18-23] Right (+X)
         1.0f,-1.0f, 1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f,  1.0f, 1.0f,-1.0f,  1.0f, 1.0f, 1.0f,
         // [24-29] Top (+Y)
         1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,  1.0f, 1.0f,-1.0f, -1.0f, 1.0f,-1.0f,

         // [30-35] Front Face (Z=1.0f)
         -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,

         // [36-41] Ears (2 Triangles)
         -1.0f,  1.0f, 1.0f,  -0.3f, 1.0f, 1.0f,  -0.8f, 1.8f, 1.0f,
         1.0f,  1.0f, 1.0f,   0.3f, 1.0f, 1.0f,   0.8f, 1.8f, 1.0f
    };

    GLfloat cat_uvs[] = {
        // [0-29] Fur UV
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f,
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f,
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f,
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 0.0f,0.0f, 1.0f,1.0f, 0.1f,1.0f,
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f,
        // [30-35] Face UV
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
        // [36-41] Ear UV
        0.0f, 0.0f,  1.0f, 0.0f,  0.5f, 1.0f,
        1.0f, 0.0f,  0.0f, 0.0f,  0.5f, 1.0f
    };

    GLuint vbo, uvbo; glGenBuffers(1, &vbo); glGenBuffers(1, &uvbo);

    // --- Main Rendering Loop ---
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        processInput(window);
        computeInputs(window);
        glUseProgram(programID);

        // --- Render Cube 1 (Large, Left) ---
        mat4 M1 = translate(mat4(1.0f), vec3(-3, 0, 0));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &(ProjectionMatrix * ViewMatrix * M1)[0][0]);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo); glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0); glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- Render Cube 2 (Small, Right) ---
        mat4 M2 = translate(mat4(1.0f), vec3(3, 0, 0)) * scale(mat4(1.0f), vec3(0.5f));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &(ProjectionMatrix * ViewMatrix * M2)[0][0]);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- Render Cat (Multi-pass Texture) ---
        mat4 MCat = translate(mat4(1.0f), vec3(0.0f, 0.5f, 0.0f));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &(ProjectionMatrix * ViewMatrix * MCat)[0][0]);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // Step 1: Body Fur (Vertices 0-29)
        glBindTexture(GL_TEXTURE_2D, texFur);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 30 * 3 * sizeof(GLfloat), &cat_verts[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo);
        glBufferData(GL_ARRAY_BUFFER, 30 * 2 * sizeof(GLfloat), &cat_uvs[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_TRIANGLES, 0, 30);

        // Step 2: Front Face (Vertices 30-35)
        glBindTexture(GL_TEXTURE_2D, texFace);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), &cat_verts[30 * 3], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(GLfloat), &cat_uvs[30 * 2], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Step 3: Ear Triangles (Vertices 36-41)
        glBindTexture(GL_TEXTURE_2D, texEar);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), &cat_verts[36 * 3], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(GLfloat), &cat_uvs[36 * 2], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &uvbo);
    glDeleteProgram(programID);
    glfwTerminate();
    return 0;
}