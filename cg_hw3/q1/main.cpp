#include <GL/freeglut.h>
#include "RayTracer.h"

const int W = 1024;
const int H = 1024;
unsigned char pixels[W * H * 3]; // RGB buffer

void render() {
    Camera cam;
    std::vector<Surface*> scene;

    // Initialize Scene Objects
    scene.push_back(new Plane(-2.0f));
    scene.push_back(new Sphere(vec3(-4, 0, -7), 1.0f));
    scene.push_back(new Sphere(vec3(0, 0, -7), 2.0f));
    scene.push_back(new Sphere(vec3(4, 0, -7), 1.0f));

    for (int j = 0; j < H; j++) {
        for (int i = 0; i < W; i++) {
            Ray ray = cam.getRay(i, j);
            bool hit = false;
            float tHit, tClosest = 1e20;

            for (auto obj : scene) {
                if (obj->intersect(ray, 0.001f, tClosest, tHit)) {
                    hit = true;
                    tClosest = tHit; // Update closest intersection
                }
            }

            int idx = (j * W + i) * 3;
            if (hit) {
                pixels[idx] = pixels[idx + 1] = pixels[idx + 2] = 255; // White
            }
            else {
                pixels[idx] = pixels[idx + 1] = pixels[idx + 2] = 0;   // Black
            }
        }
    }

    // Clean up
    for (auto obj : scene) delete obj;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw the pixel buffer to screen
    glDrawPixels(W, H, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glFlush();
}

int main(int argc, char** argv) {
    render(); // Run Ray Tracing once

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(W, H);
    glutCreateWindow("Ray Intersection - Q1");

    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}