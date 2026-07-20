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
int Width = 1024;  // Screen width (nx)
int Height = 1024; // Screen height (ny)
std::vector<float> OutputImage; // Color buffer (RGB)
std::vector<float> DepthBuffer;  // Z-buffer for visibility tests

int     gNumVertices = 0;
int     gNumTriangles = 0;
int* gIndexBuffer = nullptr;
vec3* gVertexBuffer = nullptr;

// -------------------------------------------------
// Geometry Generation (Unit Sphere Mesh)
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

            float x = std::sin(theta) * std::cos(phi);
            float y = std::cos(theta);
            float z = -std::sin(theta) * std::sin(phi);

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

// Cross product equivalent in 2D to calculate signed triangle area / orientation
float edgeFunction(const vec3& a, const vec3& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

// Fragment Shading Stage (Per-pixel evaluation)
vec3 computePhongPixelColor(const vec3& worldPos, const vec3& worldNormal, const vec3& eyePos, const vec3& lightPos) {
    vec3 ka(0.0f, 1.0f, 0.0f);
    vec3 kd(0.0f, 0.5f, 0.0f);
    vec3 ks(0.8f, 0.8f, 0.8f);
    float p = 64.0f;
    float Ia = 0.2f;
    float Il = 1.0f;

    // Renormalize attributes broken during interpolation (crucial for accurate specular calculation)
    vec3 normal = glm::normalize(worldNormal);
    vec3 viewDir = glm::normalize(eyePos - worldPos);
    vec3 lightDir = glm::normalize(lightPos - worldPos);
    vec3 halfVector = glm::normalize(lightDir + viewDir);

    // Calculate Blinn-Phong diffuse and specular dot products
    float n_dot_l = std::max(0.0f, glm::dot(normal, lightDir));
    float n_dot_h = std::max(0.0f, glm::dot(normal, halfVector));
    float specular = (n_dot_h > 0.0f) ? std::pow(n_dot_h, p) : 0.0f;

    // Apply Blinn-Phong specular-reflection intensity equation
    vec3 color = ka * Ia + kd * Il * n_dot_l + ks * Il * specular;

    // Execute Gamma Correction (Gamma = 2.2)
    color.r = std::pow(color.r, 1.0f / 2.2f);
    color.g = std::pow(color.g, 1.0f / 2.2f);
    color.b = std::pow(color.b, 1.0f / 2.2f);

    return glm::clamp(color, 0.0f, 1.0f);
}

// Draw a Phong-shaded triangle by interpolating geometric positions and surface normals 
void rasterizePhongTriangle(const vec3& s0, const vec3& s1, const vec3& s2,
    const vec3& w0_pos, const vec3& w1_pos, const vec3& w2_pos,
    const vec3& w0_nrm, const vec3& w1_nrm, const vec3& w2_nrm,
    const vec3& eyePos, const vec3& lightPos)
{
    // Determine bounding box restricted to screen dimensions
    int minX = std::max(0, (int)std::floor(std::min({ s0.x, s1.x, s2.x })));
    int maxX = std::min(Width - 1, (int)std::ceil(std::max({ s0.x, s1.x, s2.x })));
    int minY = std::max(0, (int)std::floor(std::min({ s0.y, s1.y, s2.y })));
    int maxY = std::min(Height - 1, (int)std::ceil(std::max({ s0.y, s1.y, s2.y })));

    float area = edgeFunction(s0, s1, s2.x, s2.y);
    if (std::abs(area) < 1e-5f) return; // Discard degenerate triangles

    // Main loops iterating over pixel blocks inside the bounding box
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            // Compute edge sub-weights for point containment test
            float w0 = edgeFunction(s1, s2, px, py);
            float w1 = edgeFunction(s2, s0, px, py);
            float w2 = edgeFunction(s0, s1, px, py);

            // Test if pixel center lies strictly inside or on the boundaries
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                w0 /= area;
                w1 /= area;
                w2 /= area;

                // Linearly interpolate the perspective depth value (z)
                float z = w0 * s0.z + w1 * s1.z + w2 * s2.z;
                int idx = y * Width + x;

                // Standard depth-test check for visibility occlusion handling
                if (z < DepthBuffer[idx]) {
                    DepthBuffer[idx] = z;

                    // Linearly interpolate world-space position coordinates
                    vec3 pixelWorldPos = w0 * w0_pos + w1 * w1_pos + w2 * w2_pos;

                    // Linearly interpolate world-space normal vectors across fragments
                    vec3 pixelWorldNormal = w0 * w0_nrm + w1 * w1_nrm + w2 * w2_nrm;

                    // Execute lighting computations per fragment using interpolated attributes
                    vec3 finalColor = computePhongPixelColor(pixelWorldPos, pixelWorldNormal, eyePos, lightPos);

                    // Update frame color channels with shaded output
                    OutputImage[idx * 3 + 0] = finalColor.r;
                    OutputImage[idx * 3 + 1] = finalColor.g;
                    OutputImage[idx * 3 + 2] = finalColor.b;
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
    // Clear back buffers (Background: Black, Initial depth: 1.0)
    std::fill(OutputImage.begin(), OutputImage.end(), 0.0f);
    std::fill(DepthBuffer.begin(), DepthBuffer.end(), 1.0f);

    create_scene();

    // 1. Modeling Transform matrix setup
    mat4 M_model = mat4(1.0f);
    M_model = glm::translate(M_model, vec3(0.0f, 0.0f, -7.0f));
    M_model = glm::scale(M_model, vec3(2.0f, 2.0f, 2.0f));

    // 2. View Transform matrix setup
    mat4 M_view = mat4(1.0f);
    vec3 eyePos(0.0f, 0.0f, 0.0f);
    vec3 lightPos(-4.0f, 4.0f, -3.0f);

    // 3. Perspective Projection matrix setup
    mat4 M_proj = glm::frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 1000.0f);
    mat4 VP = M_proj * M_view;

    // 4. Triangle Processing Loop
    for (int i = 0; i < gNumTriangles; ++i) {
        int idx[3] = { gIndexBuffer[3 * i + 0], gIndexBuffer[3 * i + 1], gIndexBuffer[3 * i + 2] };

        vec3 world_pos[3];
        vec3 world_nrm[3];
        vec3 screen_pos[3];

        for (int j = 0; j < 3; ++j) {
            vec3 localPos = gVertexBuffer[idx[j]];

            // Vertex position acts directly as local surface normal on a unit sphere
            vec3 localNormal = glm::normalize(localPos);

            // Pass geometric attributes down into World Space coordinates
            world_pos[j] = vec3(M_model * vec4(localPos, 1.0f));
            world_nrm[j] = glm::normalize(mat3(M_model) * localNormal);

            // --- A. Project and Transform Vertices to Screen Space ---
            vec4 clipPos = VP * vec4(world_pos[j], 1.0f);

            // Perform standard Homogeneous Perspective Division
            vec3 ndcPos = vec3(clipPos) / clipPos.w;

            // Map Normalized Device Coordinates (NDC) to 2D Screen pixel map positions
            screen_pos[j] = vec3(
                (ndcPos.x + 1.0f) * 0.5f * Width,
                (ndcPos.y + 1.0f) * 0.5f * Height,
                ndcPos.z
            );
        }

        // --- B. Back-Face Culling Geometry Test ---
        vec3 faceNormal = glm::normalize(glm::cross(world_pos[1] - world_pos[0], world_pos[2] - world_pos[0]));
        vec3 viewDir = glm::normalize(eyePos - world_pos[0]);
        if (glm::dot(faceNormal, viewDir) < 0.0f) continue;

        // Dispatch screen coordinates along with world parameters into the soft-phong shader
        rasterizePhongTriangle(screen_pos[0], screen_pos[1], screen_pos[2],
            world_pos[0], world_pos[1], world_pos[2],
            world_nrm[0], world_nrm[1], world_nrm[2],
            eyePos, lightPos);
    }
}

// Callback invoked automatically upon window viewport size adjustments
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

    GLFWwindow* window = glfwCreateWindow(Width, Height, "Phong Shading Viewer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glfwSetFramebufferSizeCallback(window, resize_callback);
    resize_callback(NULL, Width, Height);

    // Continuous execution loop updating window visualization frames
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // Push computed matrix buffer bits directly down onto the drawing viewport canvas
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