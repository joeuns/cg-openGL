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

// JPG/PNG 로드를 위한 라이브러리
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace glm;


// Callback function to adjust the viewport when the window is resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Process keyboard input: Close window if ESC is pressed
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// --- 전역 변수: 카메라 제어용 [cite: 9, 83, 94] ---
vec3 position = vec3(0, 0, 7);
float horizontalAngle = 3.14f;
float verticalAngle = 0.0f;
float speed = 3.0f;
float mouseSpeed = 0.002f;

mat4 ViewMatrix;
mat4 ProjectionMatrix;

// --- 유틸리티: 텍스처 로더 ---
GLuint loadTexture(const char* path) {
    int width, height, nrChannels;
    // 이미지 로드 전 뒤집기 설정
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (!data) {
        // 이 메시지가 뜨면 100% 파일 경로 문제입니다.
        std::cout << "!! 텍스처 로드 실패: " << path << " 파일을 찾을 수 없습니다 !!" << std::endl;
        std::cout << "파일이 .cpp 소스 파일과 같은 폴더에 있는지 확인하세요." << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 채널 수에 따른 포맷 자동 설정
    GLenum format = GL_RGB;
    if (nrChannels == 4) format = GL_RGBA;
    else if (nrChannels == 1) format = GL_RED;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // 메모리 해제 전 안전 장치
    stbi_image_free(data);

    std::cout << "성공: " << path << " 로드 완료 (" << width << "x" << height << ")" << std::endl;
    return textureID;
}

// --- 유틸리티: 쉐이더 로더 [cite: 18] ---
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

// --- 입력 처리: 키보드 & 마우스 [cite: 83, 94] ---
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

    // VAO 생성 (필수!)
    GLuint VAO; glGenVertexArrays(1, &VAO); glBindVertexArray(VAO);

    GLuint programID = LoadShaders("Transform.vertexshader", "Texture.fragmentshader");
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint tex1 = loadTexture("texture1.jpg");
    GLuint tex2 = loadTexture("texture2.jpg");
    GLuint texFur = loadTexture("fur.jpg");   // 몸통(옆, 뒤, 위, 아래)
    GLuint texFace = loadTexture("face.jpg");  // 앞면(눈코입)
    GLuint texEar = loadTexture("ear.jpg");   // 귀 삼각형

    if (programID == 0) printf("쉐이더 로드 실패!\n");
    if (tex1 == 0) printf("cube1.jpg 로드 실패!\n");

    // 1. 큐브 데이터 
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

    // 고양이 데이터: 총 42개 정점 (몸통 30 + 앞면 6 + 귀 6)
    GLfloat cat_verts[] = {
        // --- [0-5] 왼쪽면 (-X) ---
        -1.0f,-1.0f,-1.0f, -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f,
        // --- [6-11] 뒷면 (-Z) ---
         1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
        // --- [12-17] 아랫면 (-Y) ---
         1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,
         1.0f,-1.0f, 1.0f, -1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,
         // --- [18-23] 오른쪽면 (+X) [수정됨] ---
          1.0f,-1.0f, 1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f, // 삼각형 1
          1.0f,-1.0f, 1.0f,  1.0f, 1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  // 삼각형 2
          // --- [24-29] 윗면 (+Y) ---
           1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f,
           1.0f, 1.0f, 1.0f,  1.0f, 1.0f,-1.0f, -1.0f, 1.0f,-1.0f,

           // --- [30-35] 앞면 (Z=1.0f, 얼굴) ---
           -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
           -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,

           // --- [36-41] 귀 (삼각형 2개) ---
           -1.0f,  1.0f, 1.0f,  -0.3f, 1.0f, 1.0f,  -0.8f, 1.8f, 1.0f,
            1.0f,  1.0f, 1.0f,   0.3f, 1.0f, 1.0f,   0.8f, 1.8f, 1.0f
    };

    GLfloat cat_uvs[] = {
        // [0-29] 털 UV (5개 면 반복)
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f, // 1
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f, // 2
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f, // 3
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 0.0f,0.0f, 1.0f,1.0f, 0.1f,1.0f, // 4 (오른쪽 수정 대응)
        0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f, 0.0f,1.0f, 0.0f,0.0f, // 5

        // [30-35] 얼굴 UV
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,

        // [36-41] 귀 UV
        0.0f, 0.0f,  1.0f, 0.0f,  0.5f, 1.0f,
        1.0f, 0.0f,  0.0f, 0.0f,  0.5f, 1.0f
    };


    GLuint vbo, uvbo; glGenBuffers(1, &vbo); glGenBuffers(1, &uvbo);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        processInput(window);
        computeInputs(window);
        glUseProgram(programID);

        // --- 큐브 1 (큰 사이즈, 왼쪽)  ---
        mat4 M1 = translate(mat4(1.0f), vec3(-3, 0, 0));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &(ProjectionMatrix * ViewMatrix * M1)[0][0]);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo); glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0); glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- 큐브 2 (작은 사이즈, 오른쪽)  ---
        mat4 M2 = translate(mat4(1.0f), vec3(3, 0, 0)) * scale(mat4(1.0f), vec3(0.5f));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &(ProjectionMatrix * ViewMatrix * M2)[0][0]);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- 고양이 모델 행렬 설정 ---
        // --- 고양이 그리기 시작 ---
        mat4 MCat = translate(mat4(1.0f), vec3(0.0f, 0.5f, 0.0f));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &(ProjectionMatrix * ViewMatrix * MCat)[0][0]);

        // VAO 속성 포인터 활성화 (필요한 경우)
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // 1. 털(몸통 5개 면) 그리기
        glBindTexture(GL_TEXTURE_2D, texFur);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 30 * 3 * sizeof(GLfloat), &cat_verts[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo);
        glBufferData(GL_ARRAY_BUFFER, 30 * 2 * sizeof(GLfloat), &cat_uvs[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_TRIANGLES, 0, 30);

        // 2. 얼굴(정면) 그리기
        glBindTexture(GL_TEXTURE_2D, texFace);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), &cat_verts[30 * 3], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(GLfloat), &cat_uvs[30 * 2], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 3. 귀 그리기
        glBindTexture(GL_TEXTURE_2D, texEar);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), &cat_verts[36 * 3], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(GLfloat), &cat_uvs[36 * 2], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window); glfwPollEvents();
    }
    glfwTerminate(); return 0;
}