#include <GL/freeglut.h>
#include "RayTracer.h"
#include <cmath>
#include <random>

// Framebuffer and Scene constants
const int W = 1024, H = 1024;
unsigned char pixels[W * H * 3];

// Lighting and numerical stability parameters
const vec3 lightPos(-4.0f, 4.0f, -3.0f);
const float EPSILON = 0.02f; // Offset to prevent self-intersection (Shadow Acne)

// Q4: Implementation of Toon Shading
// This custom feature provides a stylized, non-photorealistic look by quantizing color intensity.
vec3 shadeToon(const Ray& ray, float t, vec3 normal, const Material& mat, const std::vector<Surface*>& scene) {
    vec3 hitPoint = ray.at(t);
    vec3 L = normalize(lightPos - hitPoint);

    // 1. Shadow Visibility Test using Shadow Rays
    Ray shadowRay(hitPoint + normal * EPSILON, L);
    float tShadow, distToLight = length(lightPos - hitPoint);
    vec3 nShadow;

    for (auto obj : scene) {
        if (obj->intersect(shadowRay, 0.001f, distToLight, tShadow, nShadow)) {
            // In shadow: Return a dimmed ambient component for a flat, toon-like shadow effect
            return mat.ka * 0.5f;
        }
    }

    // 2. Compute Diffuse Intensity based on Lambertian Cosine Law
    float diffuseIntensity = std::max(0.0f, dot(normal, L));

    // 3. [Q4 Core] Intensity Quantization (Posterization)
    // Discrete steps are created to simulate hand-drawn cel animation shading levels.
    float toonLevels = 3.0f;
    float level = std::floor(diffuseIntensity * toonLevels) / toonLevels;

    // Minimum Brightness Correction: Prevent surfaces from becoming pitch black in lit areas
    if (level < 0.2f) level = 0.2f;

    // 4. Stylized Specular Highlight
    vec3 V = normalize(ray.origin - hitPoint);
    vec3 H_vec = normalize(L + V);
    float specIntensity = std::pow(std::max(0.0f, dot(normal, H_vec)), mat.n);

    // Only apply specular if it exceeds a high threshold to maintain the toon aesthetic
    float toonSpecular = (mat.n > 0 && specIntensity > 0.5f) ? 1.0f : 0.0f;

    // Combine discrete diffuse levels and binary specular highlights with ambient light
    vec3 color = mat.kd * level + (mat.ks * toonSpecular);
    return color + mat.ka;
}

void render() {
    Camera cam;
    std::vector<Surface*> scene;

    // Define object materials (ka, kd, ks, n)
    Material mP = { vec3(0.2f), vec3(1.0f), vec3(0.0f), 0.0f };
    Material mS1 = { vec3(0.2f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f), 0.0f };
    Material mS2 = { vec3(0.0f, 0.2f, 0.0f), vec3(0.0f, 0.5f, 0.0f), vec3(0.5f), 32.0f };
    Material mS3 = { vec3(0.0f, 0.0f, 0.2f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f), 0.0f };

    // Initialize scene geometry
    scene.push_back(new Plane(-2.0f, mP));
    scene.push_back(new Sphere(vec3(-4.0f, 0.0f, -7.0f), 1.0f, mS1));
    scene.push_back(new Sphere(vec3(0.0f, 0.0f, -7.0f), 2.0f, mS2));
    scene.push_back(new Sphere(vec3(4.0f, 0.0f, -7.0f), 1.0f, mS3));


    // Q3: Antialiasing sample settings
    const int N = 64;
    const float gamma = 2.2f;
    const float invGamma = 1.0f / gamma;

    // Random number generator for jittered sampling
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // Main Ray Tracing Loop
    for (int j = 0; j < H; j++) {
        for (int i = 0; i < W; i++) {
            vec3 pixelColorSum(0.0f);

            // Q3: Stratified/Random sampling for Anti-aliasing (N samples per pixel)
            for (int s = 0; s < N; s++) {
                // Generate random jitter offsets within the pixel area 
                float offsetX = dist(gen);
                float offsetY = dist(gen);

                // Calculate sample position on the image plane
                float u = cam.l + (cam.r - cam.l) * (i + offsetX) / (float)cam.nx;
                float v = cam.b + (cam.t - cam.b) * (j + offsetY) / (float)cam.ny;

                // Construct a primary ray from the eye through the sample point
                Ray ray(cam.eye, vec3(u, v, -cam.d));

                float tHit, tClosest = 1e20f;
                vec3 hitNormal;
                Material hitMat;
                bool hit = false;

                // Find the closest intersection among all objects in the scene
                for (auto obj : scene) {
                    vec3 tempNormal;
                    if (obj->intersect(ray, 0.001f, tClosest, tHit, tempNormal)) {
                        hit = true;
                        tClosest = tHit;
                        hitNormal = tempNormal;
                        hitMat = obj->mat;
                    }
                }

                // Compute shading if an object is hit; otherwise, use background color
                if (hit) {
                    pixelColorSum += shadeToon(ray, tClosest, hitNormal, hitMat, scene);
                }
                else {
                    pixelColorSum += vec3(0.0f); // Accumulate black for missed rays
                }
            }

            // Q3: Apply Box Filter by averaging the accumulated samples 
            vec3 avgColor = pixelColorSum / (float)N;

            // Q2: Post-processing - Gamma Correction (gamma = 2.2) 
            avgColor.r = std::pow(std::max(0.0f, std::min(1.0f, avgColor.r)), invGamma);
            avgColor.g = std::pow(std::max(0.0f, std::min(1.0f, avgColor.g)), invGamma);
            avgColor.b = std::pow(std::max(0.0f, std::min(1.0f, avgColor.b)), invGamma);

            // Convert floating-point color [0, 1] to 8-bit unsigned char [0, 255]
            int idx = (j * W + i) * 3;
            pixels[idx] = (unsigned char)(avgColor.r * 255);
            pixels[idx + 1] = (unsigned char)(avgColor.g * 255);
            pixels[idx + 2] = (unsigned char)(avgColor.b * 255);
        }
    }

    for (auto obj : scene) delete obj;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(W, H, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glFlush();
}

int main(int argc, char** argv) {
    render();
    glutInit(&argc, argv);
    glutInitWindowSize(W, H);
    glutCreateWindow("Q4");
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}