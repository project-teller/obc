#pragma once

#include <cstddef>
#include <stdexcept>

namespace teller::math {

/**
 * @brief Simple 3D vector class template.
 */
template <typename T>
class Vector3 {

public:
    typedef T value_type;

    T x;
    T y;
    T z;

public:
    Vector3(T x = 0, T y = 0, T z = 0)
    {
        set(x, y, z);
    }

    Vector3<T>& set(T x, T y, T z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        return *this;
    }

    Vector3<T> operator+(const Vector3<T>& other)
    {
        return Vector3<T>(x + other.x, y + other.y, z + other.z);
    }

    Vector3<T> operator-(const Vector3<T>& other)
    {
        return Vector3<T>(x - other.x, y - other.y, z - other.z);
    }

    Vector3<T> operator-()
    {
        return (*this) * -1;
    }

    Vector3<T> operator*(T factor)
    {
        return Vector3<T>(x * factor, y * factor, z * factor);
    }

    Vector3<T> operator/(T factor)
    {
        return Vector3<T>(x / factor, y / factor, z / factor);
    }

    Vector3<T>& operator+=(const Vector3<T>& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3<T>& operator-=(const Vector3<T>& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3<T>& operator*=(T factor)
    {
        x *= factor;
        y *= factor;
        z *= factor;
        return *this;
    }

    Vector3<T>& operator/=(T factor)
    {
        x /= factor;
        y /= factor;
        z /= factor;
        return *this;
    }

    T& operator[](std::size_t idx)
    {
        switch (idx) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            throw std::runtime_error("index out of range");
        }
    }

    const T& operator[](std::size_t idx) const
    {
        switch (idx) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            throw std::runtime_error("index out of range");
        }
    }

    bool operator==(const Vector3<T>& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Vector3<T>& other) const
    {
        return !(*this == other);
    }
};

using Vector3f = Vector3<float>;

}
