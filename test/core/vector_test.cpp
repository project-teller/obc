#include <gtest/gtest.h>

#include "core/math/vector.hpp"

using namespace teller::math;

TEST(VectorTest, assignment)
{
    Vector3f vec;

    EXPECT_EQ(vec.x, 0);
    EXPECT_EQ(vec.y, 0);
    EXPECT_EQ(vec.z, 0);

    vec.set(2, 3, 5);

    EXPECT_EQ(vec.x, 2);
    EXPECT_EQ(vec.y, 3);
    EXPECT_EQ(vec.z, 5);
}

TEST(VectorTest, addition)
{
    Vector3f a(2, 3, 5), b(3, 5, 8), c;

    c = a + b;
    EXPECT_EQ(c.x, 5);
    EXPECT_EQ(c.y, 8);
    EXPECT_EQ(c.z, 13);

    a += b;
    EXPECT_EQ(a, c);
    EXPECT_FALSE(a != c);
}

TEST(VectorTest, subtraction)
{
    Vector3f a(5, 8, 13), b(3, 5, 8), c;

    c = a - b;
    EXPECT_EQ(c.x, 2);
    EXPECT_EQ(c.y, 3);
    EXPECT_EQ(c.z, 5);

    a -= b;
    EXPECT_EQ(a, c);
    EXPECT_FALSE(a != c);
}

TEST(VectorTest, multiplication)
{
    Vector3f a(5, 8, 13), b;

    b = a * 2;
    EXPECT_EQ(b.x, 10);
    EXPECT_EQ(b.y, 16);
    EXPECT_EQ(b.z, 26);

    a *= -2;
    EXPECT_EQ(a, -b);
}

TEST(VectorTest, division)
{
    Vector3f a(10, 16, 26), b;

    b = a / 2;
    EXPECT_EQ(b.x, 5);
    EXPECT_EQ(b.y, 8);
    EXPECT_EQ(b.z, 13);

    a /= -2;
    EXPECT_EQ(a, -b);
}

TEST(VectorTest, indexing)
{
    const Vector3f a(10, 16, 26);
    Vector3f b(10, 16, 26);

    EXPECT_EQ(a[0], a.x);
    EXPECT_EQ(a[1], a.y);
    EXPECT_EQ(a[2], a.z);
    try {
        a[3];
        FAIL() << "Expected std::runtime_error";
    } catch (std::runtime_error const& err) {
        /* pass */
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }

    EXPECT_EQ(b[0], b.x);
    EXPECT_EQ(b[1], b.y);
    EXPECT_EQ(b[2], b.z);
    try {
        b[3];
        FAIL() << "Expected std::runtime_error";
    } catch (std::runtime_error const& err) {
        /* pass */
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }

    b[0] = 42;
    EXPECT_EQ(b.x, 42);
    try {
        b[3] = 42;
        FAIL() << "Expected std::runtime_error";
    } catch (std::runtime_error const& err) {
        /* pass */
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}
