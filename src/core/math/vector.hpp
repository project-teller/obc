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

    Vector3<T> operator+(const Vector3<T>& other) const
    {
        return Vector3<T>(x + other.x, y + other.y, z + other.z);
    }

    Vector3<T> operator-(const Vector3<T>& other) const
    {
        return Vector3<T>(x - other.x, y - other.y, z - other.z);
    }

    Vector3<T> operator-() const
    {
        return (*this) * -1;
    }

    Vector3<T> operator*(T factor) const
    {
        return Vector3<T>(x * factor, y * factor, z * factor);
    }

    Vector3<T> operator/(T factor) const
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
        default:
            return z;
        }
    }

    const T& operator[](std::size_t idx) const
    {
        switch (idx) {
        case 0:
            return x;
        case 1:
            return y;
        default:
            return z;
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

    Vector3<T> abs() const
    {
        return Vector3<T>(std::abs(x), std::abs(y), std::abs(z));
    }

    Vector3<T> elementwiseDiv(const Vector3<T>& other) const
    {
        return Vector3<T>(x / other.x, y / other.y, z / other.z);
    }

    int argmax() const
    {
        if (y > x) {
            return (z > y) ? 2 : 1;
        } else {
            return (z > x) ? 2 : 0;
        }
    }
};

using Vector3f = Vector3<float>;

}
