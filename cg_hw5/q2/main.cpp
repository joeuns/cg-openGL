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
std::vector<float> DepthBuffer;  // Custom Z-buffer for occlusion handling

int     gNumVertices = 0;
int     gNumTriangles = 0;
int* gIndexBuffer = nullptr;
vec3* gVertexBuffer = nullptr;

// Generate a unit sphere mesh using the provided data topology
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
            theta = (float)j / (height - 1) * M_PI;
            phi = (float)i / (width - 1) * M_PI * 2;

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);

            gVertexBuffer[t] = vec3(x, y, z);
            t++;
        }
    }

    gVertexBuffer[t] = vec3(0.0f, 1.0f, 0.0f);
    t++;

    gVertexBuffer[t] = vec3(0.0f, -1.0f, 0.0f);
    t++;

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
// Software Rasterizer Pipeline Operations
// -------------------------------------------------

// Perform homogeneous coordinate transformation and perspective divide
vec3 transformVertex(const mat4& M, const vec3& v) {
    vec4 v4 = M * vec4(v, 1.0f);
    if (v4.w != 0.0f) {
        return vec3(v4.x / v4.w, v4.y / v4.w, v4.z / v4.w);
    }
    return vec3(v4);
}

// Compute 2D edge function for barycentric coordinates
float edgeFunction(const vec3& a, const vec3& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

// Rasterize a single triangle with bounding box clipping, custom color per object, and Z-buffer test
void rasterizeTriangle(const vec3& v0, const vec3& v1, const vec3& v2, const vec3& color) {
    // Define a bounding box clipped to the viewport boundaries
    int minX = std::max(0, (int)std::floor(std::min({ v0.x, v1.x, v2.x })));
    int maxX = std::min(Width - 1, (int)std::ceil(std::max({ v0.x, v1.x, v2.x })));
    int minY = std::max(0, (int)std::floor(std::min({ v0.y, v1.y, v2.y })));
    int maxY = std::min(Height - 1, (int)std::ceil(std::max({ v0.y, v1.y, v2.y })));

    float area = edgeFunction(v0, v1, v2.x, v2.y);
    if (std::abs(area) < 1e-5) return;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float w0 = edgeFunction(v1, v2, px, py);
            float w1 = edgeFunction(v2, v0, px, py);
            float w2 = edgeFunction(v0, v1, px, py);

            // Inside-outside test using edge functions
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                w0 /= area;
                w1 /= area;
                w2 /= area;

                // Interpolate depth value (z) across the triangle face
                float z = w0 * v2.z + w1 * v0.z + w2 * v1.z;
                int idx = y * Width + x;

                // Occlusion handling via depth test (Inverted operator due to perspective matrix mapping characteristics)
                if (z > DepthBuffer[idx]) {
                    DepthBuffer[idx] = z;

                    // Fill frame buffers with dynamically passed custom colors
                    OutputImage[idx * 3 + 0] = color.r;
                    OutputImage[idx * 3 + 1] = color.g;
                    OutputImage[idx * 3 + 2] = color.b;
                }
            }
        }
    }
}

// Main rendering pipeline setup and loop execution
void render()
{
    // Clear frame and depth buffers (Background: Black, Initial depth coordinate cleared to -1.0)
    std::fill(OutputImage.begin(), OutputImage.end(), 0.0f);
    std::fill(DepthBuffer.begin(), DepthBuffer.end(), -1.0f);

    create_scene();

    // Retrieve elapsed application runtime using GLFW clock to feed time-driven animations
    float time = (float)glfwGetTime();

    // -------------------------------------------------------------
    // Shared Transformation Matrices (View, Projection, Viewport)
    // -------------------------------------------------------------
    // View Transform: Camera at origin looking down -w axis
    mat4 M_view = mat4(1.0f);

    // Perspective Projection Transform setup using given boundaries
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, n = -0.1f, f = -1000.0f;
    mat4 M_proj = mat4(0.0f);
    M_proj[0][0] = (2.0f * n) / (r - l); M_proj[2][0] = (r + l) / (r - l);
    M_proj[1][1] = (2.0f * n) / (t - b); M_proj[2][1] = (t + b) / (t - b);
    M_proj[2][2] = (f + n) / (n - f);     M_proj[3][2] = (2.0f * f * n) / (f - n);
    M_proj[2][3] = 1.0f;

    // Viewport Transform mapping normalized coordinates to screen pixel spaces
    mat4 M_viewport = mat4(0.0f);
    M_viewport[0][0] = Width / 2.0f;  M_viewport[3][0] = (Width - 1) / 2.0f;
    M_viewport[1][1] = Height / 2.0f; M_viewport[3][1] = (Height - 1) / 2.0f;
    M_viewport[2][2] = 1.0f;           M_viewport[3][3] = 1.0f;

    // -------------------------------------------------------------
    // [OBJECT 1] Earth: Central large sphere with procedural shading
    // -------------------------------------------------------------
    mat4 M_model1 = mat4(1.0f);
    M_model1 = glm::translate(M_model1, vec3(0.0f, 0.0f, -7.0f)); // Position at (0, 0, -7)
    M_model1 = glm::rotate(M_model1, time * 0.4f, vec3(0.0f, 1.0f, 0.0f)); // Apply continuous Y-axis rotation (Self-rotation)
    M_model1 = glm::scale(M_model1, vec3(2.0f, 2.0f, 2.0f));       // Scale factor 2.0

    for (int i = 0; i < gNumTriangles; ++i) {
        int k0 = gIndexBuffer[3 * i + 0];
        int k1 = gIndexBuffer[3 * i + 1];
        int k2 = gIndexBuffer[3 * i + 2];

        vec3 localV0 = gVertexBuffer[k0];
        vec3 localV1 = gVertexBuffer[k1];
        vec3 localV2 = gVertexBuffer[k2];

        // Compute barycenter of the triangle in 3D local space for procedural pattern generation
        vec3 center = (localV0 + localV1 + localV2) / 3.0f;

        vec3 earthColor;

        // Lower values of frequency create larger continental chunks, higher values make it finer
        float frequency = 4.0f;

        // Procedural spatial-partition logic utilizing spatial frequencies to generate large camouflage/checker continent patterns
        float pattern = sinf(center.x * frequency) * sinf(center.y * frequency) * sinf(center.z * frequency);

        if (pattern > 0.0f) {
            // Low-saturation Land mass tone: rgb(32, 77, 36) mapped to [0,1] floating scale
            earthColor = vec3(0.125f, 0.302f, 0.141f);
        }
        else {
            // Low-saturation Deep Ocean tone: rgb(30, 45, 89) mapped to [0,1] floating scale
            earthColor = vec3(0.118f, 0.176f, 0.349f);
        }

        // Apply geometric pipeline sequence to vertices
        vec3 v0 = transformVertex(M_viewport, transformVertex(M_proj, transformVertex(M_view, transformVertex(M_model1, localV0))));
        vec3 v1 = transformVertex(M_viewport, transformVertex(M_proj, transformVertex(M_view, transformVertex(M_model1, localV1))));
        vec3 v2 = transformVertex(M_viewport, transformVertex(M_proj, transformVertex(M_view, transformVertex(M_model1, localV2))));

        rasterizeTriangle(v0, v1, v2, earthColor);
    }

    // -------------------------------------------------------------
    // [OBJECT 2] Moon: Small orbiting sphere demonstrating custom animation
    // -------------------------------------------------------------
    // Calculate circular orbit kinematics around Earth on the X-Z projection plane
    float orbitRadius = 3.8f;
    float orbitX = sinf(time * 1.2f) * orbitRadius;
    float orbitZ = -7.0f + cosf(time * 1.2f) * orbitRadius; // Centered at Earth's depth position (-7.0)

    mat4 M_model2 = mat4(1.0f);
    M_model2 = glm::translate(M_model2, vec3(orbitX, 0.0f, orbitZ)); // Transform to dynamic orbit positions
    M_model2 = glm::scale(M_model2, vec3(0.5f, 0.5f, 0.5f));         // Scale down moon radius to 0.5

    vec3 moonColor = vec3(0.85f, 0.85f, 0.7f); // Assign flat pastel warm grey tone to Moon

    for (int i = 0; i < gNumTriangles; ++i) {
        int k0 = gIndexBuffer[3 * i + 0];
        int k1 = gIndexBuffer[3 * i + 1];
        int k2 = gIndexBuffer[3 * i + 2];

        // Apply geometric pipeline sequence to Moon vertices (Reusing the base shared sphere buffer data data structures)
        vec3 v0 = transformVertex(M_viewport, transformVertex(M_proj, transformVertex(M_view, transformVertex(M_model2, gVertexBuffer[k0]))));
        vec3 v1 = transformVertex(M_viewport, transformVertex(M_proj, transformVertex(M_view, transformVertex(M_model2, gVertexBuffer[k1]))));
        vec3 v2 = transformVertex(M_viewport, transformVertex(M_proj, transformVertex(M_view, transformVertex(M_model2, gVertexBuffer[k2]))));

        rasterizeTriangle(v0, v1, v2, moonColor);
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
    // -------------------------------------------------
    // Initialize Window
    // -------------------------------------------------
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glfwSetFramebufferSizeCallback(window, resize_callback);
    resize_callback(NULL, Width, Height);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        // Re-execute rasterization loop sequentially inside runtime cycles to capture animation ticks
        render();

        glClear(GL_COLOR_BUFFER_BIT);

        // -------------------------------------------------------------
        // Blit host memory pixel stream to context viewport
        glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
        // -------------------------------------------------------------

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }

    // Deallocate heap arrays cleanly upon exit routine
    if (gVertexBuffer != nullptr) delete[] gVertexBuffer;
    if (gIndexBuffer != nullptr) delete[] gIndexBuffer;

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}