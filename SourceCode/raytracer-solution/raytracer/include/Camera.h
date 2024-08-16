#pragma once
#include <tuple>
#include <queue>

#include "json_fwd.h"

#include "include/CRTTypes.h"
#include "include/Image.h"
#include "include/SceneObject.h"

class Camera : public SceneObject
{
public:
    Camera() = default;
    Camera(float fovDeg, Vec3 _pos, Matrix3x3 _mat)
        : SceneObject(_pos, _mat) { setFov(fovDeg); }
    auto getPos() const { return pos; }
    auto getDir() const { return mat.getCol(2); }
    auto getUp() const { return mat.getCol(1); }
    auto getRight() const { return mat.getCol(0); }

    /* warning: does not preserve roll. Implementing quaternions will fix this */
    void setDir(const Vec3& dir);
    void setFov(float fovDeg);

    void lookAt(const Vec3& dest);
    nlohmann::ordered_json toJson() const;
    /* Adds ray to `queue` with direction (`x`, `y`) on the image plane */
    void emplaceTask(size_t maxX, size_t maxY, size_t x, size_t y, std::queue<TraceTask>& queue) const;

private:
    /* creates ray with direction (`x`, `y`) on the image plane */
    Ray rayFromPixel(size_t maxX, size_t maxY, size_t x, size_t y) const;

    /* Normalized Device Coordinates From Raster Coordinates
    *  x, y [0, width] [0, height] -> x, y [0, 1.0] [0, 1.0]
    *  example: top left raster pixel (0,0) -> (0, 0)
    *  example: bottom right raster pixel (width, height) -> (1, 1) */
    void ndcFromRaster(size_t maxX, size_t maxY, Vec3& coordinates) const;

    /* Image Plane Coordinates From Normalized Device Coordinates
    *  returns image plane coordinates. Positive y is up, positive x is right
    *  example: top left NDC (0,0) -> (-1, 1)
    *  example: bottom right NDC (1,1) -> (1, -1) */
    void imageFromNdc(size_t maxX, size_t maxY, Vec3& coordinates) const;

    float tanHalfFov = 1.f; // Field of view in degrees
};
