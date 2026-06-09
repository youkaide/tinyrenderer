#pragma once
#include <cmath>
#include <cassert>
#include <iostream>

template<int n> struct vec
{
    double data[n] = { 0 };
    double& operator[](const int i) { assert(i >= 0 && i < n); return data[i]; }
    double  operator[](const int i) const { assert(i >= 0 && i < n); return data[i]; }
};

template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v)
{
    for (int i = 0; i < n; i++) out << v[i] << " ";
    return out;
}

template<> struct vec<2>
{
    double x = 0, y = 0;
    double& operator[](const int i) { assert(i >= 0 && i < 2); return 1 == i ? y : x; }
    double  operator[](const int i) const { assert(i >= 0 && i < 2); return 1 == i ? y : x; }

    vec<2> operator+(const vec<2>& v) const { return{ x + v.x,y + v.y }; }
    vec<2> operator-(const vec<2>& v) const { return{ x - v.x,y - v.y }; }
    vec<2> operator*(double f) const { return{ x * f,y * f }; }
    vec<2> operator/(double f) const
    {
        assert(std::abs(f) > 1e-12);
        return{ x / f,y / f };
    }

    double dot(const vec<2>& v) const
    {
        return x * v.x + y * v.y;
    }

    double cross(const vec<2>& v) const
    {
        return x * v.y - y * v.x;
    }

    double norm() const
    {
        return std::sqrt(x * x + y * y);
    }

    vec<2> normalize() const
    {
        double dis = norm();
        if (dis == 0) return{ 0,0 };
        return { x / dis,y / dis };
    }

    double distance(const vec<2>& v) const
    {
        return std::sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y));
    }

    double squared_distance(const vec<2>& v) const
    {
        return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y);
    }
};

typedef vec<2> vec2;

template<> struct vec<3>
{
    double x = 0, y = 0, z = 0;
    double& operator[](const int i) { assert(i >= 0 && i < 3); return i ? (1 == i ? y : z) : x; }
    double  operator[](const int i) const { assert(i >= 0 && i < 3); return i ? (1 == i ? y : z) : x; }

    vec<3> operator+(const vec<3>& v) const { return { x + v.x,y + v.y,z + v.z }; }
    vec<3> operator-(const vec<3>& v) const { return{ x - v.x,y - v.y,z - v.z }; }
    vec<3> operator*(double f) const { return { x * f,y * f,z * f }; }
    vec<3> operator/(double f) const
    {
        assert(std::abs(f) > 1e-12);
        return { x / f,y / f,z / f };
    }

    double dot(const vec<3>& v) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    vec<3> cross(const vec<3>& v) const
    {
        return { y * v.z - v.y * z,z * v.x - x * v.z,x * v.y - y * v.x };
    }

    double norm() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    vec<3> normalize() const
    {
        double dis = norm();
        if (dis == 0) return{ 0,0,0 };
        return { x / dis,y / dis,z / dis };
    }

    double distance(const vec<3>& v) const
    {
        return std::sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z));
    }

    double squared_distance(const vec<3>& v) const
    {
        return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z);
    }
};

typedef vec<3> vec3;

template<> struct vec<4>
{
    double x = 0, y = 0, z = 0, w = 0;
    double& operator[](const int i) { assert(i >= 0 && i < 4); return i ? (1 == i ? y : (2 == i ? z : w)) : x; }
    double  operator[](const int i) const { assert(i >= 0 && i < 4); return i ? (1 == i ? y : (2 == i ? z : w)) : x; }

    vec<4> operator+(const vec<4>& v) const { return { x + v.x,y + v.y,z + v.z,w + v.w }; }
    vec<4> operator-(const vec<4>& v) const { return{ x - v.x,y - v.y,z - v.z,w - v.w }; }
    vec<4> operator*(double f) const { return { x * f,y * f,z * f,w * f }; }
    vec<4> operator/(double f) const
    {
        assert(std::abs(f) > 1e-12);
        return { x / f,y / f,z / f,w / f };
    }

    double dot(const vec<4>& v) const
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    double norm() const
    {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    vec<4> normalize() const
    {
        double dis = norm();
        if (dis == 0) return{ 0,0,0,0 };
        return { x / dis,y / dis,z / dis,w / dis };
    }

    double distance(const vec<4>& v) const
    {
        return std::sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z) + (w - v.w) * (w - v.w));
    }

    double squared_distance(const vec<4>& v) const
    {
        return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z) + (w - v.w) * (w - v.w);
    }
};

typedef vec<4> vec4;

inline vec<4> embed(const vec<3>& v)
{
    return { v.x,v.y,v.z,1.0 };
}

inline vec<3> proj(const vec<4>& v)
{
    assert(std::abs(v.w) > 1e-12);
    return { v.x / v.w,v.y / v.w,v.z / v.w };
}

template<int n> struct mat
{
    vec<n> rows[n];
    vec<n>& operator[](const int i) { assert(i >= 0 && i < n); return rows[i]; }
    vec<n>  operator[](const int i) const { assert(i >= 0 && i < n); return rows[i]; }

    mat()
    {
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                rows[i][j] = 0.0;
            }
        }
    }

    static mat<n> identity()
    {
        mat<n> m;

        for (int i = 0; i < n; i++)
            m[i][i] = 1.0;

        return m;
    }

    vec<n> col(int idx) const
    {
        assert(idx >= 0 && idx < n);
        vec<n> result;
        for (int i = 0; i < n; ++i)
        {
            result[i] = rows[i][idx];
        }
        return result;
    }

    mat<n> transpose()
    {
        mat<n> res;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                res[i][j] = rows[j][i];
            }
        }
        return res;
    }
};

template<int n> std::ostream& operator<<(std::ostream& out, const mat<n>& m)
{
    for (int i = 0; i < n; i++)
    {
        out << m[i] << "\n";
    }
    return out;
}

template<int n>
mat<n> operator*(const mat<n>& A, const mat<n>& B)
{
    mat<n> res;
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            res[i][j] = A.rows[i].dot(B.col(j));
        }
    }
    return res;
}

template<int n>
vec<n> operator*(const mat<n>& M, const vec<n>& V)
{
    vec<n> res;
    for (int i = 0; i < n; ++i)
    {
        res[i] = M[i].dot(V);
    }
    return res;
}

typedef mat<3> mat3;
typedef mat<4> mat4;