#include <GL/freeglut.h>
#include <omp.h>
#include "RayTracer.h"

// --- Framebuffer and Scene Resolution ---
// Default resolution is 1024 x 1024 as per assignment requirements.
// NOTE: If real-time performance (Mouse Tracking/Animation) is slow on your machine, 
// feel free to lower the resolution to 512 or 256 for a smoother experience.
const int W = 512;
const int H = 512;
//---------------------------------------//

unsigned char pixels[W * H * 3];

// Lighting and numerical stability parameters
vec3 lightPos = vec3(-4.0f, 4.0f, -3.0f);
float timer = 0.0f; // Track elapsed time
const float EPSILON = 0.02f; // Offset to prevent self-intersection (Shadow Acne)

// Flag to track mouse presence
bool isMouseInWindow = false;


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
    cam.nx = W;
    cam.ny = H;

    std::vector<Surface*> scene;

    // Define object materials (ka, kd, ks, n)
    Material mP = { vec3(0.2), vec3(1.0), vec3(0.0), 0.0f };
    Material mS1 = { vec3(0.2, 0, 0), vec3(1, 0, 0), vec3(0), 0.0f };
    Material mS2 = { vec3(0, 0.2, 0), vec3(0, 0.5, 0), vec3(0.5), 32.0f };
    Material mS3 = { vec3(0, 0, 0.2), vec3(0, 0, 1), vec3(0), 0.0f };

    // Initialize scene geometry
    scene.push_back(new Plane(-2.0f, mP));
    scene.push_back(new Sphere(vec3(-4, 0, -7), 1.0f, mS1));
    scene.push_back(new Sphere(vec3(0, 0, -7), 2.0f, mS2));
    scene.push_back(new Sphere(vec3(4, 0, -7), 1.0f, mS3));

    // Main Ray Tracing Loop
    #pragma omp parallel for schedule(dynamic) // * Parallel processing using all CPU cores * //
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
                vec3 res = shade(ray, tClosest, hitNormal, hitMat, scene);
                pixels[idx] = (unsigned char)(std::min(1.0f, res.r) * 255);
                pixels[idx + 1] = (unsigned char)(std::min(1.0f, res.g) * 255);
                pixels[idx + 2] = (unsigned char)(std::min(1.0f, res.b) * 255);
            }
            else {
                pixels[idx] = pixels[idx + 1] = pixels[idx + 2] = 0; // Black background
            }
        }
    }

    for (auto obj : scene) delete obj;
}

// Detect when mouse enters or leaves the window
void onEntry(int state) {
    if (state == GLUT_ENTERED) {
        isMouseInWindow = true;
    }
    else {
        isMouseInWindow = false;
    }
}

// Callback function for mouse motion
void onMouseMotion(int x, int y) {
    // Map mouse screen coordinates [0, 1024] to world coordinates [-10, 10]
    lightPos.x = ((float)x / W) * 20.0f - 10.0f;
    lightPos.y = 10.0f - ((float)y / H) * 20.0f; // Invert Y-axis
    lightPos.z = -3.0f; // Keep a fixed depth for the light source

    // Re-render and refresh the display
    render();
    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(W, H, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glFlush();
}

void update() {
    // Only move automatically if mouse is NOT in the window
    if (!isMouseInWindow) {
        timer += 0.05f;
        lightPos.x = 6.0f * sin(timer);

        render(); // Re-render for animation
        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    render();
    glutInit(&argc, argv);
    glutInitWindowSize(W, H);
    glutCreateWindow("Ray Tracer - Phong Shading");

    glutDisplayFunc(display);
    glutIdleFunc(update); // Register the update function for animation

    glutPassiveMotionFunc(onMouseMotion);
    glutEntryFunc(onEntry); // Register mouse enter/leave detection

    glutMainLoop();

    return 0;
}