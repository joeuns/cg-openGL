#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

using namespace glm;

// --- Ray Class ---
class Ray {
public:
    vec3 origin;
    vec3 dir;
    Ray(vec3 o, vec3 d) : origin(o), dir(normalize(d)) {}
};

// --- Base Surface Class ---
class Surface {
public:
    virtual bool intersect(const Ray& ray, float tMin, float tMax, float& tHit) = 0;
    virtual ~Surface() {}
};

// --- Sphere Class ---
class Sphere : public Surface {
public:
    vec3 center;
    float radius;

    Sphere(vec3 c, float r) : center(c), radius(r) {}

    virtual bool intersect(const Ray& ray, float tMin, float tMax, float& tHit) override {
        vec3 oc = ray.origin - center;
        float a = dot(ray.dir, ray.dir);
        float b = 2.0f * dot(oc, ray.dir);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant > 0) {
            // 1st Check: Try the closer intersection (Entry point)
            float t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
            if (t > tMin && t < tMax) {
                tHit = t;
                return true;
            }
            // 2nd Check: Try the farther intersection (Exit point)
            t = (-b + glm::sqrt(discriminant)) / (2.0f * a);
            if (t > tMin && t < tMax) {
                tHit = t;
                return true;
            }
        }
        return false;
    }
};

// --- Plane Class ---
class Plane : public Surface {
public:
    float y_val; // Plane equation: y = y_val
    Plane(float y) : y_val(y) {}

    virtual bool intersect(const Ray& ray, float tMin, float tMax, float& tHit) override {
        if (abs(ray.dir.y) < 1e-6) return false; // Parallel to plane
        float t = (y_val - ray.origin.y) / ray.dir.y;
        if (t > tMin && t < tMax) {
            tHit = t;
            return true;
        }
        return false;
    }
};

// --- Camera Class ---
class Camera {
public:
    vec3 eye = vec3(0, 0, 0);
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, d = 0.1f;
    int nx = 1024, ny = 1024;

    Ray getRay(int ix, int iy) {
        float u = l + (r - l) * (ix + 0.5f) / (float)nx;
        float v = b + (t - b) * (iy + 0.5f) / (float)ny;
        return Ray(eye, vec3(u, v, -d)); // Looking towards -w direction
    }
};