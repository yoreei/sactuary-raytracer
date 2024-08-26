#pragma once

#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <vector>

#include "json_fwd.h"

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: (" #condition "), function " << __FUNCTION__ \
                      << ", file " << __FILE__ << ", line " << __LINE__ << "." << std::endl; \
            std::abort(); \
        } \
    } while (false)


inline constexpr float epsilon = 1e-5f; // high precision
// inline constexpr float epsilon = 1e-4f; // good precision

class Vec3;
namespace Utils {
    /*
    * @return the number of elements moved
    */

    template <typename T>
    inline size_t move_back(std::vector<T>& to, std::vector<T>& from) {
        size_t padding = from.size();
        to.reserve(to.size() + from.size());
        to.insert(to.end(),
            std::make_move_iterator(from.begin()),
            std::make_move_iterator(from.end()));
        return padding;
    }


    template <typename T>
    T jsonGetDefault(const nlohmann::json& j, const std::string& key, T defaultVal);
    template <>
    Vec3 jsonGetDefault<Vec3>(const nlohmann::json& j, const std::string& key, Vec3 defaultVal);

    template<class T>
    inline bool contains(const std::vector<T>& vec, const T& item) {
        return std::find(vec.begin(), vec.end(), item) != vec.end();
    }

    inline std::string stringFromBool(bool b) {
        return b ? "true" : "false";
    }
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        os << vec[i];
        if (i != vec.size() - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

