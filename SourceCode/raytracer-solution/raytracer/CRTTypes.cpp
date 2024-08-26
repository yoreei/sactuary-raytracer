#include "json.hpp"

#include "include/CRTTypes.h"
#include "include/Globals.h"

Ray::Ray(const Vec3& origin, const Vec3& direction) : origin(origin)
{
    setDirection(direction);
}

void Ray::setDirection(const Vec3& newDirection)
{
    direction = newDirection;
    invdir = { 1 / direction.x, 1 / direction.y, 1 / direction.z };
    sign[0] = (invdir.x < 0);
    sign[1] = (invdir.y < 0);
    sign[2] = (invdir.z < 0);
}

void Ray::reflect(const Vec3& point, const Vec3& N) {
    float cosi = dot(direction, N);
    origin = point;
    setDirection(direction = direction - 2 * cosi * N);
    direction.normalize();
}

bool Ray::refractSP(const Vec3 point, const Vec3& N, float etai, float etat)
{
    float cosi = dot(direction, N);
    assert(cosi < 1e-6);
    cosi = std::clamp(cosi, -1.f, 0.f);

    float eta = etai / etat;

    float cosTSq = 1.f - eta * eta * (1.f - cosi * cosi);
    if (cosTSq < 0.f) return false; // Total Internal Reflection
    float cosT = sqrtf(cosTSq);
    cosT = std::clamp(cosT, -1.f, 1.f);
    Vec3 newDirection = eta * direction + (eta * cosi - cosT) * N;
    newDirection.normalize();
    // explanation:
    // T = X + Y
    // X = -N * cosT
    // Y = eta * I + eta * cosi * N 
    // = eta * (I + cosI * N)
    // = M * sinT 
    // M = (I + cosI * N) * 1 / sinI, where M is unit surface tangent in plane of incidence
    // sinT = eta * sinI
    setDirection(newDirection);
    origin = point;

#ifndef NDEBUG
    float cosTTest = fabs(dot(direction, N));
    cosTTest = std::clamp(cosTTest, -1.f, 1.f);
    assertFEqual(cosT, cosTTest, 0.1f); // very low precision here.
#endif
    return true;
}

bool Ray::refractVladi(const Vec3& point, Vec3 normal, float iorI, float iorR) {
#ifndef NDEBUG
    auto dbgOldDirection = direction;
    assertFEqual(direction.lengthSquared(), 1.f);
    assert(dot(direction, normal) < -epsilon); 
#endif

    float cosI = -dot(direction, normal);
    float sinR = sqrt(1 - cosI * cosI) * iorI / iorR;
    if (sinR - 1.f >= -1e-6f) {
        return false; // total internal reflection
    }
    float cosR = sqrt(1 - sinR * sinR);
    // bump direction to avoid 0 vector check
    Vec3 newDirection = direction * (1.f + 1e-6f);
    Vec3 C = (newDirection + cosI * normal).getUnit();
    Vec3 B = C * sinR;
    Vec3 A = -normal * cosR;
    newDirection = A + B;
    newDirection.normalize();

    setDirection(newDirection);
    origin = point;

    assertFEqual(dot(direction, normal), -cosR, 1e-2f);
    assert(dot(direction, dbgOldDirection) > 0); // R must never be "reflected"
    return true;
}

nlohmann::ordered_json Vec3::toJson() const {
    nlohmann::ordered_json j{};
    j["x"] = x;
    j["y"] = y;
    j["z"] = z;
    return j;
}

void Vec3::componentMax(const Vec3& vec0, const Vec3& vec1, Vec3& vec2Output)
{
    vec2Output.x = std::max({ vec0.x, vec1.x, vec2Output.x });
    vec2Output.y = std::max({ vec0.y, vec1.y, vec2Output.y });
    vec2Output.z = std::max({ vec0.z, vec1.z, vec2Output.z });
}

void Vec3::componentMin(const Vec3& vec0, const Vec3& vec1, Vec3& vec2Output)
{
    vec2Output.x = std::min({ vec0.x, vec1.x, vec2Output.x });
    vec2Output.y = std::min({ vec0.y, vec1.y, vec2Output.y });
    vec2Output.z = std::min({ vec0.z, vec1.z, vec2Output.z });
}

nlohmann::ordered_json Matrix3x3::toJson() const {
    nlohmann::ordered_json j;
    j["row0"] = row(0).toJson();
    j["row1"] = row(1).toJson();
    j["row2"] = row(2).toJson();
    return j;
}

bool Matrix3x3::isOrthonormal() const {
    Vec3 col0 = getCol(0);
    Vec3 col1 = getCol(1);
    Vec3 col2 = getCol(2);

    bool unitVectors = col0.isUnit() &&
        col1.isUnit() &&
        col2.isUnit();

    bool perpendicular = fEqual(col0.dot(col1), 0.0f) &&
        fEqual(col0.dot(col2), 0.0f) &&
        fEqual(col1.dot(col2), 0.0f);

    return unitVectors && perpendicular;
}


/* For more, read https://leimao.github.io/blog/CPP-Float-Point-Number-Comparison/ */
void assertFEqual(float a, float b, float atol) {
    a;
    b;
    atol;
#ifndef NDEBUG
    float absA = std::fabs(a);
    float absB = std::fabs(b);
    float diff = std::fabs(a - b);

    // Use relative tolerance if numbers are large, absolute tolerance otherwise
    float rtol = epsilon * std::max(absA, absB);
    float tol = std::max(atol, rtol);
    assert(diff <= tol);
#endif
}

void to_json(nlohmann::json& j, const Vec3& v)
{
    j = nlohmann::json{ { "x", v.x },{ "y", v.y },{ "z", v.z } };
}

void from_json(const nlohmann::json& j, Vec3& v)
{
    j["x"].get_to(v.x);
    j["y"].get_to(v.y);
    j["z"].get_to(v.z);
}

Vec3 slerp(const Vec3& vec0Ref, const Vec3& vec1, float t) {
    assert(vec0Ref.isUnit());
    assert(vec1.isUnit());

    Vec3 vec0 = vec0Ref;
    // if opposing, nudge vec0 in arbitrary direction
    if ((vec0 + vec1).equal({ 0.f, 0.f, 0.f })) {
        Vec3 orthogonal = std::fabs(vec0.x) < 1.f ? Vec3(1, 0, 0) : Vec3(0, 1, 0);
        orthogonal = orthogonal - vec0 * vec0.dot(orthogonal);
        orthogonal.normalize();
        vec0 = vec0 + orthogonal * 0.1f;
        vec0.normalize();
    }
    float dot = vec0.dot(vec1);
    dot = std::clamp(dot, -1.f, 1.f);
    float theta = std::acos(dot);

    // Special case which preserves numerical stability
    if (std::fabs(theta) < epsilon) {
        return lerp(vec0, vec1, t);
    }

    float sinTheta = std::sin(theta);

    float factor0 = std::sin((1 - t) * theta) / sinTheta;
    float factor1 = std::sin(t * theta) / sinTheta;

    Vec3 result = vec0 * factor0 + vec1 * factor1;
    assert(result.isUnit());
    return result;
}

void to_json(nlohmann::json& j, const Matrix3x3& mat)
{
    j = nlohmann::json(mat.data);
}

void from_json(const nlohmann::json& j, Matrix3x3& mat)
{
    j.get_to(mat.data);
}

void to_json(nlohmann::json& j, const Color& color)
{
    j = nlohmann::json{ { "r", color.r },{ "g", color.g },{ "b", color.b } };
}

void from_json(const nlohmann::json& j, Color& color)
{
    j["r"].get_to(color.r);
    j["g"].get_to(color.g);
    j["b"].get_to(color.b);
}

Color Color::fromUnit(float fr, float fg, float fb) {
    auto r = static_cast<uint8_t>(std::round(fr * maxColorComponent));
    auto g = static_cast<uint8_t>(std::round(fg * maxColorComponent));
    auto b = static_cast<uint8_t>(std::round(fb * maxColorComponent));
    CHECK(r <= maxColorComponent && g <= maxColorComponent && b <= maxColorComponent);
    return Color{ r, g, b };
}
