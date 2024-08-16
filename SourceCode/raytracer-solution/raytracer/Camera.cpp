#include "include/Camera.h"

#include "json.hpp"

#include "include/Globals.h"

void Camera::setDir(const Vec3& dir)
{
    assert(dir.isUnit());

    mat.setCol(2, dir);

    Vec3 right;
    Vec3{ 0.f, 1.f, 0.f }.cross(dir, right);
    if (fEqual(right.lengthSquared(), 0))
    {
        Vec3{ 1.f, 0.f, 0.f }.cross(dir, right);
    }
    right.normalize();

    Vec3 up;
    dir.cross(right, up);
    up.normalize();

    mat.setCol(0, right);
    mat.setCol(1, up);

    assert(mat.isOrthonormal());
}

void Camera::setFov(float fovDeg) {
    float fovRad = (fovDeg * PI) / 180.0f;
    tanHalfFov = std::tan(fovRad / 2.0f);
}

void Camera::lookAt(const Vec3& dest)
{
    if (dest.equal(pos)) {
        throw std::runtime_error("dest == pos");
    }
    Vec3 dir = dest - pos;
    dir.normalize();
    setDir(dir);
}

nlohmann::ordered_json Camera::toJson() const
{
    nlohmann::ordered_json j = SceneObject::toJson();
    j["tanHalfFov"] = tanHalfFov;
    return j;
}

void Camera::emplaceTask(size_t maxX, size_t maxY, size_t x, size_t y, std::queue<TraceTask>& queue) const
{
    Ray ray = rayFromPixel(maxX, maxY, x, y);
    queue.emplace(ray, x, y);
}

Ray Camera::rayFromPixel(size_t maxX, size_t maxY, size_t x, size_t y) const {
    Vec3 coords{ static_cast<float>(x), static_cast<float>(y), 0 };
    ndcFromRaster(maxX, maxY, coords);
    imageFromNdc(maxX, maxY, coords);

    coords.x *= tanHalfFov;
    coords.y *= tanHalfFov;

    Vec3 raydir = getDir() + getRight() * coords.x + getUp() * coords.y;
    raydir.normalize();
    return { this->pos, raydir };
}

void Camera::ndcFromRaster(size_t maxX, size_t maxY, Vec3& coordinates) const
{
    float& x = coordinates.x;
    float& y = coordinates.y;

    // move point to center of Pixel box
    x += 0.5f;
    y += 0.5f;

    x /= float(maxX);
    y /= float(maxY);
}

void Camera::imageFromNdc(size_t maxX, size_t maxY, Vec3& coordinates) const
{
    float& x = coordinates.x;
    float& y = coordinates.y;

    // x[0, 1] -> x[0, 2]
    // y[0, 1] -> y[-2, 0]
    x *= 2.0f;
    y *= -2.0f;

    // x[0, 2] y[0, -2] -> xy[-1, 1]
    x -= 1;
    y += 1;

    float aspectRatio = (static_cast<float>(maxX) / static_cast<float>(maxY)); // TODO is calculated for every ray
    x *= aspectRatio;
}

