#pragma once
#include <array>
#include <string>
#include <vector>

namespace geo {
struct Vec3 {
    double x{}, y{}, z{};
    Vec3 operator+(const Vec3& b) const { return {x+b.x,y+b.y,z+b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x-b.x,y-b.y,z-b.z}; }
    Vec3 operator*(double k) const { return {x*k,y*k,z*k}; }
};
double dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
double length(Vec3 a);
Vec3 normalized(Vec3 a);
struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<std::array<int,2>> edges;
    std::vector<std::vector<int>> faces;
    std::vector<std::pair<std::string,double>> metrics;
    bool smoothShading=false;
};
struct Params { double a=2,b=2,c=2,h=2; int n=6; };
Mesh makeMesh(const std::string& kind, const Params& p);
std::vector<Vec3> intersectPlane(const Mesh& mesh, Vec3 normal, double offset);
std::string exactRadians(double degrees);
std::string format(double value, int precision=3);
}
