#pragma once
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

using namespace glm;

// Object material properties
struct Material {
    vec3 ka;    // Ambient coefficient
    vec3 kd;    // Diffuse coefficient
    vec3 ks;    // Specular coefficient
    float n;    // Specular power (shininess)
};

// Basic Ray class: P(t) = origin + t * direction
class Ray {
public:
    vec3 origin;
    vec3 dir;
    Ray(vec3 o, vec3 d) : origin(o), dir(normalize(d)) {}

    vec3 at(float t) const { return origin + t * dir; }
};

// Base class for all renderable surfaces
class Surface {
public:
    Material mat;
    virtual bool intersect(const Ray& ray, float tMin, float tMax, float& tHit, vec3& normal) = 0;
    virtual ~Surface() {}
};

// Sphere object implementation
class Sphere : public Surface {
public:
    vec3 center;
    float radius;

    Sphere(vec3 c, float r, Material m) : center(c), radius(r) { mat = m; }

    virtual bool intersect(const Ray& ray, float tMin, float tMax, float& tHit, vec3& normal) override {
        vec3 oc = ray.origin - center;
        float a = dot(ray.dir, ray.dir);
        float b = 2.0f * dot(oc, ray.dir);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant > 0) {
            float t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
            if (t > tMin && t < tMax) {
                tHit = t;
                normal = normalize(ray.at(t) - center);
                return true;
            }
        }
        return false;
    }
};

// Infinite plane object implementation
class Plane : public Surface {
public:
    float y_val;
    Plane(float y, Material m) : y_val(y) { mat = m; }

    virtual bool intersect(const Ray& ray, float tMin, float tMax, float& tHit, vec3& normal) override {
        if (glm::abs(ray.dir.y) < 1e-6) return false; // Parallel check
        float t = (y_val - ray.origin.y) / ray.dir.y;
        if (t > tMin && t < tMax) {
            tHit = t;
            normal = vec3(0, 1, 0); // Constant normal for y-plane
            return true;
        }
        return false;
    }
};

// Perspective camera implementation
class Camera {
public:
    vec3 eye = vec3(0, 0, 0);
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, d = 0.1f;
    int nx = 1024, ny = 1024;

    Ray getRay(int ix, int iy) {
        float u = l + (r - l) * (ix + 0.5f) / (float)nx;
        float v = b + (t - b) * (iy + 0.5f) / (float)ny;
        return Ray(eye, vec3(u, v, -d));
    }
};