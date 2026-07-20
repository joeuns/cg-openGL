#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <algorithm>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------------------------------------------------
// Global Variables & Framebuffers
// -------------------------------------------------
int Width = 1024;  // Assignment requirement: nx = 1024
int Height = 1024; // Assignment requirement: ny = 1024
std::vector<float> OutputImage;
std::vector<float> DepthBuffer;

int     gNumVertices = 0;
int     gNumTriangles = 0;
int* gIndexBuffer = nullptr;
vec3* gVertexBuffer = nullptr;

// -------------------------------------------------
// Geometry Generation (Unit Sphere)
// -------------------------------------------------
void create_scene()
{
    if (gVertexBuffer != nullptr) delete[] gVertexBuffer;
    if (gIndexBuffer != nullptr) delete[] gIndexBuffer;

    int width = 32;
    int height = 16;

    float theta, phi;
    int t;

    gNumVertices = (height - 2) * width + 2;
    gNumTriangles = (height - 2) * (width - 1) * 2;

    gVertexBuffer = new vec3[gNumVertices];
    gIndexBuffer = new int[3 * gNumTriangles];

    t = 0;
    for (int j = 1; j < height - 1; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            theta = (float)j / (height - 1) * (float)M_PI;
            phi = (float)i / (width - 1) * (float)M_PI * 2.0f;

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);

            gVertexBuffer[t++] = vec3(x, y, z);
        }
    }

    gVertexBuffer[t++] = vec3(0.0f, 1.0f, 0.0f);
    gVertexBuffer[t++] = vec3(0.0f, -1.0f, 0.0f);

    t = 0;
    for (int j = 0; j < height - 3; ++j)
    {
        for (int i = 0; i < width - 1; ++i)
        {
            gIndexBuffer[t++] = j * width + i;
            gIndexBuffer[t++] = (j + 1) * width + (i + 1);
            gIndexBuffer[t++] = j * width + (i + 1);
            gIndexBuffer[t++] = j * width + i;
            gIndexBuffer[t++] = (j + 1) * width + i;
            gIndexBuffer[t++] = (j + 1) * width + (i + 1);
        }
    }
    for (int i = 0; i < width - 1; ++i)
    {
        gIndexBuffer[t++] = (height - 2) * width;
        gIndexBuffer[t++] = i;
        gIndexBuffer[t++] = i + 1;
        gIndexBuffer[t++] = (height - 2) * width + 1;
        gIndexBuffer[t++] = (height - 3) * width + (i + 1);
        gIndexBuffer[t++] = (height - 3) * width + i;
    }
}

// -------------------------------------------------
// Software Rasterizer Utilities
// -------------------------------------------------

// Compute 2D edge function for barycentric coordinates
float edgeFunction(const vec3& a, const vec3& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

// Rasterize a single triangle with the flat shaded color
void rasterizeTriangle(const vec3& v0, const vec3& v1, const vec3& v2, const vec3& color) {
    int minX = std::max(0, (int)std::floor(std::min({ v0.x, v1.x, v2.x })));
    int maxX = std::min(Width - 1, (int)std::ceil(std::max({ v0.x, v1.x, v2.x })));
    int minY = std::max(0, (int)std::floor(std::min({ v0.y, v1.y, v2.y })));
    int maxY = std::min(Height - 1, (int)std::ceil(std::max({ v0.y, v1.y, v2.y })));

    float area = edgeFunction(v0, v1, v2.x, v2.y);
    if (std::abs(area) < 1e-5f) return; // 넓이가 0인 찌그러진 삼각형 무시

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float w0 = edgeFunction(v1, v2, px, py);
            float w1 = edgeFunction(v2, v0, px, py);
            float w2 = edgeFunction(v0, v1, px, py);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                w0 /= area;
                w1 /= area;
                w2 /= area;

                // Z-버퍼 보간 (이전 코드의 정점 미스매치 픽스)
                float z = w0 * v0.z + w1 * v1.z + w2 * v2.z;
                int idx = y * Width + x;

                if (z < DepthBuffer[idx]) {
                    DepthBuffer[idx] = z;

                    // 프레임버퍼에 셰이딩된 컬러 갱신
                    OutputImage[idx * 3 + 0] = color.r;
                    OutputImage[idx * 3 + 1] = color.g;
                    OutputImage[idx * 3 + 2] = color.b;
                }
            }
        }
    }
}

// -------------------------------------------------
// Main Rendering Pipeline
// -------------------------------------------------
void render()
{
    std::fill(OutputImage.begin(), OutputImage.end(), 0.0f);
    std::fill(DepthBuffer.begin(), DepthBuffer.end(), 1.0f);

    create_scene();

    // 1. Modeling Transform
    mat4 M_model = mat4(1.0f);
    M_model = glm::translate(M_model, vec3(0.0f, 0.0f, -7.0f));
    M_model = glm::scale(M_model, vec3(2.0f, 2.0f, 2.0f));

    // 2. View Transform (e = 0, 0, 0 / Identity)
    mat4 M_view = mat4(1.0f);
    vec3 eyePos(0.0f, 0.0f, 0.0f);

    // 3. Perspective Projection
    // 명세서 상 n = 0.1, f = 1000.0 (near, far 거리값은 양수가 안전함)
    mat4 M_proj = glm::frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 1000.0f);

    // 4. Material & Light Properties
    vec3 ka(0.0f, 1.0f, 0.0f);
    vec3 kd(0.0f, 0.5f, 0.0f);
    vec3 ks(0.8f, 0.8f, 0.8f);
    float p = 64.0f;
    float Ia = 0.2f;
    float Il = 1.0f;
    vec3 lightPos(-4.0f, 4.0f, -3.0f);

    // 5. 삼각형 순회
    for (int i = 0; i < gNumTriangles; ++i) {
        int k0 = gIndexBuffer[3 * i + 0];
        int k1 = gIndexBuffer[3 * i + 1];
        int k2 = gIndexBuffer[3 * i + 2];

        // 로컬 좌표
        vec4 obj_v0 = vec4(gVertexBuffer[k0], 1.0f);
        vec4 obj_v1 = vec4(gVertexBuffer[k1], 1.0f);
        vec4 obj_v2 = vec4(gVertexBuffer[k2], 1.0f);

        // 월드 좌표로 변환
        vec3 world_v0 = vec3(M_model * obj_v0);
        vec3 world_v1 = vec3(M_model * obj_v1);
        vec3 world_v2 = vec3(M_model * obj_v2);

        // --- A. Flat Shading 연산 ---
        vec3 centroid = (world_v0 + world_v1 + world_v2) / 3.0f;
        vec3 normal = glm::normalize(glm::cross(world_v1 - world_v0, world_v2 - world_v0));
        vec3 viewDir = glm::normalize(eyePos - centroid);

        // Back-Face Culling
        if (glm::dot(normal, viewDir) < 0.0f) continue;

        vec3 lightDir = glm::normalize(lightPos - centroid);
        vec3 halfVector = glm::normalize(lightDir + viewDir);

        float n_dot_l = std::max(0.0f, glm::dot(normal, lightDir));
        float n_dot_h = std::max(0.0f, glm::dot(normal, halfVector));
        float specular = (n_dot_h > 0.0f) ? std::pow(n_dot_h, p) : 0.0f;

        // Blinn-Phong 모델
        vec3 color = ka * Ia + kd * Il * n_dot_l + ks * Il * specular;

        // 감마 보정 (Gamma = 2.2)
        color.r = std::pow(color.r, 1.0f / 2.2f);
        color.g = std::pow(color.g, 1.0f / 2.2f);
        color.b = std::pow(color.b, 1.0f / 2.2f);
        color = glm::clamp(color, 0.0f, 1.0f); // 부동소수점 오버플로우 방지 

        // --- B. Screen 공간으로 투영 변환 ---
        mat4 VP = M_proj * M_view;
        vec4 clip_v0 = VP * vec4(world_v0, 1.0f);
        vec4 clip_v1 = VP * vec4(world_v1, 1.0f);
        vec4 clip_v2 = VP * vec4(world_v2, 1.0f);

        // 원근 나눗셈 (Perspective Divide)
        vec3 ndc_v0 = vec3(clip_v0) / clip_v0.w;
        vec3 ndc_v1 = vec3(clip_v1) / clip_v1.w;
        vec3 ndc_v2 = vec3(clip_v2) / clip_v2.w;

        // 뷰포트 매핑 (Viewport Transform)
        auto toScreen = [](const vec3& ndc) {
            return vec3((ndc.x + 1.0f) * 0.5f * Width,
                (ndc.y + 1.0f) * 0.5f * Height,
                ndc.z);
            };

        vec3 screen_v0 = toScreen(ndc_v0);
        vec3 screen_v1 = toScreen(ndc_v1);
        vec3 screen_v2 = toScreen(ndc_v2);

        // 래스터라이즈 호출
        rasterizeTriangle(screen_v0, screen_v1, screen_v2, color);
    }
}

void resize_callback(GLFWwindow*, int nw, int nh)
{
    Width = nw;
    Height = nh;
    glViewport(0, 0, nw, nh);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(Width), 0.0, static_cast<double>(Height), 1.0, -1.0);

    OutputImage.resize(Width * Height * 3, 0.0f);
    DepthBuffer.resize(Width * Height, 1.0f);

    render();
}

int main(int argc, char* argv[])
{
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(Width, Height, "Flat Shading Viewer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glfwSetFramebufferSizeCallback(window, resize_callback);
    resize_callback(NULL, Width, Height);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // 소프트웨어 래스터라이즈 결과를 화면 텍스처로 전송
        glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }

    if (gVertexBuffer != nullptr) delete[] gVertexBuffer;
    if (gIndexBuffer != nullptr) delete[] gIndexBuffer;

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}