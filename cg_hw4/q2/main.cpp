#include <GL/freeglut.h>
#include "RayTracer.h"

// Framebuffer and Scene constants
const int W = 1024, H = 1024;
unsigned char pixels[W * H * 3];

// Lighting and numerical stability parameters
const vec3 lightPos(-4.0f, 4.0f, -3.0f);
const float EPSILON = 0.02f; // Offset to prevent self-intersection (Shadow Acne)

// Compute final color using Blinn-Phong model
vec3 shade(const Ray& ray, float t, vec3 normal, const Material& mat, const std::vector<Surface*>& scene) {
    vec3 hitPoint = ray.at(t);
    vec3 L = normalize(lightPos - hitPoint);
    vec3 V = normalize(ray.origin - hitPoint);
    vec3 H_vec = normalize(L + V); // Half-vector for specular highlights

    // Shadow test: Ray from surface towards the light
    Ray shadowRay(hitPoint + normal * EPSILON, L);
    float tShadow, distToLight = length(lightPos - hitPoint);
    vec3 nShadow;

    for (auto obj : scene) {
        if (obj->intersect(shadowRay, 0.001f, distToLight, tShadow, nShadow)) {
            return mat.ka; // In shadow: return ambient component only
        }
    }

    // Phong Shading: Ambient + Diffuse + Specular
    vec3 ambient = mat.ka;
    vec3 diffuse = mat.kd * std::max(0.0f, dot(normal, L));
    vec3 specular = (mat.n > 0) ? mat.ks * std::pow(std::max(0.0f, dot(normal, H_vec)), mat.n) : vec3(0.0f);

    return ambient + diffuse + specular;
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


    // Q2: Gamma Correction Setting
    const float gamma = 2.2f;
    const float invGamma = 1.0f / gamma;

    // Main Ray Tracing Loop
    for (int j = 0; j < H; j++) {
        for (int i = 0; i < W; i++) {
            Ray ray = cam.getRay(i, j);
            float tHit, tClosest = 1e20f;
            vec3 hitNormal;
            Material hitMat;
            bool hit = false;

            // Find the closest intersection point
            for (auto obj : scene) {
                vec3 tempNormal;
                if (obj->intersect(ray, 0.001f, tClosest, tHit, tempNormal)) {
                    hit = true;
                    tClosest = tHit;
                    hitNormal = tempNormal;
                    hitMat = obj->mat;
                }
            }

            int idx = (j * W + i) * 3;
            if (hit) {
                // 1. Shading calculation in linear space (Linear Color)
                vec3 res = shade(ray, tClosest, hitNormal, hitMat, scene);

                // 2. gamma collection (C_corrected = C_linear ^ (1/gamma))
                res.r = std::pow(std::max(0.0f, std::min(1.0f, res.r)), invGamma);
                res.g = std::pow(std::max(0.0f, std::min(1.0f, res.g)), invGamma);
                res.b = std::pow(std::max(0.0f, std::min(1.0f, res.b)), invGamma);

                // 3.Convert to 8-bit integer (0-255) and save
                pixels[idx] = (unsigned char)(res.r * 255);
                pixels[idx + 1] = (unsigned char)(res.g * 255);
                pixels[idx + 2] = (unsigned char)(res.b * 255);
            }
            else {
                pixels[idx] = pixels[idx + 1] = pixels[idx + 2] = 0;
            }
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
    glutCreateWindow("Q2 - Gamma Correction");
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}