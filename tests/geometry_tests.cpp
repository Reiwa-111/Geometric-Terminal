#include "geometry.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void closedEuler(const geo::Mesh& m,const char* name){long euler=(long)m.vertices.size()-(long)m.edges.size()+(long)m.faces.size();check(euler==2,name);for(const auto& f:m.faces){check(f.size()>=3,name);for(int i:f)check(i>=0&&i<(int)m.vertices.size(),name);}for(auto e:m.edges){check(e[0]>=0&&e[1]>=0&&e[0]<(int)m.vertices.size()&&e[1]<(int)m.vertices.size(),name);}}
void outwardWinding(const geo::Mesh& m,const char* name){geo::Vec3 center{};for(auto p:m.vertices)center=center+p;center=center*(1.0/m.vertices.size());for(const auto& f:m.faces){geo::Vec3 normal{},faceCenter{};for(size_t i=0;i<f.size();++i){auto a=m.vertices[f[i]],b=m.vertices[f[(i+1)%f.size()]];normal=normal+geo::cross(a,b);faceCenter=faceCenter+a;}faceCenter=faceCenter*(1.0/f.size());check(geo::dot(normal,faceCenter-center)>1e-10,name);}}
double polygonAreaXY(const std::vector<geo::Vec3>& p){double a=0;for(size_t i=0;i<p.size();++i){auto x=p[i],y=p[(i+1)%p.size()];a+=x.x*y.y-y.x*x.y;}return std::abs(a)/2;}
double polygonArea3D(const std::vector<geo::Vec3>& p,geo::Vec3 normal){geo::Vec3 sum{};for(size_t i=0;i<p.size();++i)sum=sum+geo::cross(p[i],p[(i+1)%p.size()]);return std::abs(geo::dot(sum,geo::normalized(normal)))/2;}
}
int main(){
    try{
        geo::Params p{2,3,4,3,6};
        for(const char* kind:{"Triangle","Right triangle","Square","Rectangle","Circle","Regular polygon","Cube","Cuboid","Tetrahedron","Octahedron","Cylinder","Cone","Prism","Pyramid","Sphere"}){
            auto m=geo::makeMesh(kind,p);check(!m.vertices.empty(),kind);check(!m.edges.empty(),kind);check(!m.faces.empty(),kind);check(!m.metrics.empty(),kind);
        }
        auto cube=geo::makeMesh("Cube",{2,2,2,2,6});check(cube.vertices.size()==8,"cube vertices");check(cube.edges.size()==12,"cube edges");check(cube.faces.size()==6,"cube faces");closedEuler(cube,"cube Euler/topology");
        closedEuler(geo::makeMesh("Tetrahedron",p),"tetrahedron Euler");closedEuler(geo::makeMesh("Octahedron",p),"octahedron Euler");closedEuler(geo::makeMesh("Cylinder",p),"cylinder Euler");closedEuler(geo::makeMesh("Cone",p),"cone Euler");closedEuler(geo::makeMesh("Prism",p),"prism Euler");closedEuler(geo::makeMesh("Pyramid",p),"pyramid Euler");closedEuler(geo::makeMesh("Sphere",p),"sphere Euler");
        for(const char* kind:{"Cube","Cuboid","Tetrahedron","Octahedron","Cylinder","Cone","Prism","Pyramid","Sphere"})outwardWinding(geo::makeMesh(kind,p),kind);
        auto hex=geo::makeMesh("Regular polygon",{2,2,2,2,6});for(auto e:hex.edges)check(std::abs(geo::length(hex.vertices[e[0]]-hex.vertices[e[1]])-2)<1e-8,"regular polygon side length matches parameter");
        auto prism=geo::makeMesh("Prism",{2,2,2,3,6});check(std::abs(prism.metrics[1].second-6*4/(4*std::tan(3.14159265358979323846/6)))<1e-8,"prism base area matches side length");
        auto octa=geo::makeMesh("Octahedron",{2,2,2,2,6});check(std::abs(octa.metrics[1].second-2*std::sqrt(3.0)*4)<1e-8,"octahedron surface area");
        auto cut=geo::intersectPlane(cube,{0,0,1},0);check(cut.size()==4,"cube midplane section vertex count");check(std::abs(polygonAreaXY(cut)-4)<1e-8,"cube midplane section area");
        auto side=geo::intersectPlane(cube,{1,0,0},0);check(side.size()==4,"cube vertical section");auto miss=geo::intersectPlane(cube,{0,0,1},2);check(miss.empty(),"plane outside cube");
        auto diagonal=geo::intersectPlane(cube,{1,1,1},0);check(diagonal.size()==6,"arbitrary diagonal plane section");check(std::abs(polygonArea3D(diagonal,{1,1,1})-3*std::sqrt(3.0))<1e-7,"diagonal section area");
        auto pyramid=geo::makeMesh("Pyramid",{4,4,4,3,6});auto pyramidCut=geo::intersectPlane(pyramid,{0,0,1},0);
        check(pyramidCut.size()==6,"hexagonal pyramid mid-section vertex count");
        for(auto q:pyramidCut)check(std::abs(q.z)<1e-9,"pyramid section lies on cutting plane");
        check(std::abs(polygonArea3D(pyramidCut,{0,0,1})-10.392304845413264)<1e-8,"hexagonal pyramid mid-section area");
        double pyramidPerimeter=0;for(size_t i=0;i<pyramidCut.size();++i)pyramidPerimeter+=geo::length(pyramidCut[(i+1)%pyramidCut.size()]-pyramidCut[i]);
        check(std::abs(pyramidPerimeter-12)<1e-8,"hexagonal pyramid mid-section perimeter");
        geo::Vec3 p0{0,0,1},p1{1,0,1},p2{0,1,1};auto planeNormal=geo::normalized(geo::cross(p1-p0,p2-p0));double planeOffset=geo::dot(planeNormal,p0);auto throughThree=geo::intersectPlane(cube,planeNormal,planeOffset);check(throughThree.size()==4,"plane through three points");check(std::abs(polygonArea3D(throughThree,planeNormal)-4)<1e-8,"three-point plane section area");
        auto tri=geo::makeMesh("Right triangle",{3,4,1,2,6});check(std::abs(tri.metrics[0].second-6)<1e-9,"right triangle area");check(std::abs(tri.metrics[1].second-5)<1e-9,"right triangle hypotenuse");
        check(geo::exactRadians(30)=="pi/6","30 degrees exact radians");check(geo::exactRadians(45)=="pi/4","45 degrees exact radians");check(geo::exactRadians(90)=="pi/2","90 degrees exact radians");check(geo::exactRadians(180)=="pi","180 degrees exact radians");check(geo::exactRadians(270)=="3pi/2","270 degrees exact radians");check(geo::exactRadians(360)=="2pi","360 degrees exact radians");check(geo::exactRadians(-90)=="-pi/2","negative exact radians");check(geo::exactRadians(1080)=="6pi","multiple full turns exact radians");
        check(std::abs(geo::length({3,4,0})-5)<1e-10,"vector length");bool zeroRejected=false;try{geo::normalized({0,0,0});}catch(const std::invalid_argument&){zeroRejected=true;}check(zeroRejected,"zero normal rejected");
        std::cout<<"GeoTerminal geometry tests: 15 models, 7 Euler checks, 9 outward-winding checks, 5 plane sections, exact radians and vector math PASS\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
