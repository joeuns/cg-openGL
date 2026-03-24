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

using namespace glm;

// --- 유틸리티 함수: BMP 로더 ---
GLuint loadBMP_custom(const char* imagepath) {
    unsigned char header[54];
    unsigned int dataPos, imageSize, width, height;
    unsigned char* data;

    FILE* file;
    fopen_s(&file, imagepath, "rb");
    if (!file) { printf("이미지를 열 수 없습니다\n"); return 0; }
    if (fread(header, 1, 54, file) != 54) { return 0; }
    if (header[0] != 'B' || header[1] != 'M') { return 0; }

    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    width = *(int*)&(header[0x12]);
    height = *(int*)&(header[0x16]);

    if (imageSize == 0) imageSize = width * height * 3;
    if (dataPos == 0) dataPos = 54;

    data = new unsigned char[imageSize];
    fread(data, 1, imageSize, file);
    fclose(file);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // 이미지를 OpenGL에 전달 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    delete[] data;
    return textureID;
}

// --- 유틸리티 함수: 쉐이더 로더 ---
GLuint LoadShaders(const char* v_path, const char* f_path) {
    std::string v_code;
    std::ifstream v_stream(v_path, std::ios::in);
    if (v_stream.is_open()) {
        std::stringstream sstr; sstr << v_stream.rdbuf();
        v_code = sstr.str(); v_stream.close();
    }
    std::string f_code;
    std::ifstream f_stream(f_path, std::ios::in);
    if (f_stream.is_open()) {
        std::stringstream sstr; sstr << f_stream.rdbuf();
        f_code = sstr.str(); f_stream.close();
    }

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

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "HW2: Textured Cube", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // [중요] VAO 생성 (조님이 이전에 놓쳤던 부분!)
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLuint programID = LoadShaders("Transform.vertexshader", "Texture.fragmentshader");
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint Texture = loadBMP_custom("uvtemplate.bmp");
    GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

    // 정점 데이터 (36개)
    static const GLfloat g_vertex_buffer_data[] = {
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

    // UV 좌표 데이터 (이미지의 어느 부분을 가져올지 결정) 
    static const GLfloat g_uv_buffer_data[] = {
        0.000059f, 1.0f - 0.000004f, 0.000103f, 1.0f - 0.336048f, 0.335973f, 1.0f - 0.335903f,
        1.0f - 0.000013f, 1.0f - 0.671840f, 0.000059f, 1.0f - 0.000004f, 0.335973f, 1.0f - 0.000059f,
        0.667979f, 1.0f - 0.335851f, 0.335973f, 1.0f - 0.335903f, 0.667969f, 1.0f - 0.335892f,
        1.0f - 0.000013f, 1.0f - 0.671840f, 0.667969f, 1.0f - 0.671889f, 0.335973f, 1.0f - 0.335903f,
        0.000059f, 1.0f - 0.000004f, 0.335973f, 1.0f - 0.335903f, 0.335973f, 1.0f - 0.000059f,
        0.335973f, 1.0f - 0.335903f, 0.000103f, 1.0f - 0.336048f, 0.000059f, 1.0f - 0.000004f,
        0.667979f, 1.0f - 0.335851f, 0.335973f, 1.0f - 0.335903f, 0.335973f, 1.0f - 0.671644f,
        1.0f - 0.335851f, 1.0f - 0.000013f, 0.668104f, 1.0f - 0.000013f, 0.667979f, 1.0f - 0.335851f,
        0.668104f, 1.0f - 0.000013f, 1.0f - 0.335851f, 1.0f - 0.335851f, 1.0f - 0.000013f, 1.0f - 0.335851f,
        1.0f - 0.335851f, 1.0f - 0.000013f, 0.667979f, 1.0f - 0.335851f, 0.335973f, 1.0f - 0.000059f,
        1.0f - 0.335851f, 1.0f - 0.000013f, 0.335973f, 1.0f - 0.000059f, 0.668104f, 1.0f - 0.000013f,
        1.0f - 0.335851f, 1.0f - 0.000013f, 0.668104f, 1.0f - 0.000013f, 0.667979f, 1.0f - 0.335851f
    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(programID);

        // MVP 행렬 계산 (튜토리얼 3, 4 동일) [cite: 30, 64]
        mat4 Projection = perspective(radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
        mat4 View = lookAt(vec3(4, 3, 3), vec3(0, 0, 0), vec3(0, 1, 0));
        mat4 Model = mat4(1.0f);
        mat4 MVP = Projection * View * Model;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // 텍스처 바인딩
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glUniform1i(TextureID, 0);

        // 정점 (Location 0)
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // UV (Location 1) 
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteProgram(programID);
    glDeleteTextures(1, &Texture);
    glDeleteVertexArrays(1, &VertexArrayID);
    glfwTerminate();
    return 0;
}