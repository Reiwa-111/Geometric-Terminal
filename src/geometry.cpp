#include "geometry.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace geo {
namespace { constexpr double PI=3.14159265358979323846;
std::vector<Vec3> ring(int n,double r,double z=0,double phase=PI/2){std::vector<Vec3> v;for(int i=0;i<n;++i){double t=2*PI*i/n+phase;v.push_back({r*std::cos(t),r*std::sin(t),z});}return v;}
void collectEdges(Mesh& m){std::set<std::pair<int,int>> e;for(const auto& f:m.faces)for(size_t i=0;i<f.size();++i){int a=f[i],b=f[(i+1)%f.size()];if(a>b)std::swap(a,b);e.emplace(a,b);}m.edges.clear();for(auto [a,b]:e)m.edges.push_back({a,b});}
void orientFacesOutward(Mesh& m){
    if(m.vertices.empty())return;
    Vec3 center{};for(auto p:m.vertices)center=center+p;center=center*(1.0/m.vertices.size());
    for(auto& f:m.faces){
        if(f.size()<3)continue;
        Vec3 normal{},faceCenter{};
        for(size_t i=0;i<f.size();++i){const auto a=m.vertices[f[i]],b=m.vertices[f[(i+1)%f.size()]];normal=normal+cross(a,b);faceCenter=faceCenter+a;}
        faceCenter=faceCenter*(1.0/f.size());
        if(dot(normal,faceCenter-center)<-1e-10)std::reverse(f.begin(),f.end());
    }
}
double areaRegular(int n,double side){return n*side*side/(4*std::tan(PI/n));}
double circumradiusRegular(int n,double side){return side/(2*std::sin(PI/n));}
}
double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3 a){return std::sqrt(dot(a,a));}
Vec3 normalized(Vec3 a){double l=length(a);if(l<1e-12)throw std::invalid_argument("Нулевой вектор");return a*(1/l);}
std::string format(double v,int precision){std::ostringstream s;s<<std::fixed<<std::setprecision(precision)<<v;return s.str();}

Mesh makeMesh(const std::string& kind,const Params& p){
    Mesh m;const double a=std::max(.01,p.a),b=std::max(.01,p.b),c=std::max(.01,p.c),h=std::max(.01,p.h);const int n=std::clamp(p.n,3,64);
    if(kind=="Triangle") {m.vertices={{-a/2,-h/3,0},{a/2,-h/3,0},{0,2*h/3,0}};m.faces={{0,1,2}};m.metrics={{"Area",a*h/2},{"Perimeter",a+2*std::hypot(a/2,h)}};}
    else if(kind=="Right triangle"){m.vertices={{0,0,0},{a,0,0},{0,b,0}};m.faces={{0,1,2}};m.metrics={{"Area",a*b/2},{"Hypotenuse",std::hypot(a,b)},{"Angle A",std::atan2(b,a)*180/PI},{"Angle B",std::atan2(a,b)*180/PI},{"Right angle",90}};}
    else if(kind=="Square"||kind=="Rectangle"){double x=a,y=kind=="Square"?a:b;m.vertices={{-x/2,-y/2,0},{x/2,-y/2,0},{x/2,y/2,0},{-x/2,y/2,0}};m.faces={{0,1,2,3}};m.metrics={{"Area",x*y},{"Perimeter",2*(x+y)},{"Diagonal",std::hypot(x,y)}};}
    else if(kind=="Circle"||kind=="Regular polygon"){int count=kind=="Circle"?96:n;m.vertices=ring(count,kind=="Circle"?a/2:circumradiusRegular(n,a));std::vector<int> face;for(int i=0;i<count;++i)face.push_back(i);m.faces={face};m.metrics={{kind=="Circle"?"Radius":"Side length",kind=="Circle"?a/2:a},{"Area",kind=="Circle"?PI*a*a/4:areaRegular(n,a)},{"Perimeter",kind=="Circle"?PI*a:n*a}};}
    else if(kind=="Cube"||kind=="Cuboid"){double x=a,y=kind=="Cube"?a:b,z=kind=="Cube"?a:c;for(int sz:{-1,1})for(int sy:{-1,1})for(int sx:{-1,1})m.vertices.push_back({sx*x/2,sy*y/2,sz*z/2});m.faces={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};m.metrics={{"Volume",x*y*z},{"Surface area",2*(x*y+y*z+x*z)},{"Space diagonal",std::sqrt(x*x+y*y+z*z)}};}
    else if(kind=="Tetrahedron"){m.vertices={{0,0,std::sqrt(2.0/3)*a},{-a/2,-a/(2*std::sqrt(3.0)),0},{a/2,-a/(2*std::sqrt(3.0)),0},{0,a/std::sqrt(3.0),0}};m.faces={{0,1,2},{0,2,3},{0,3,1},{1,3,2}};m.metrics={{"Volume",a*a*a/(6*std::sqrt(2.0))},{"Surface area",std::sqrt(3.0)*a*a}};}
    else if(kind=="Octahedron"){double r=a/std::sqrt(2.0);m.vertices={{r,0,0},{-r,0,0},{0,r,0},{0,-r,0},{0,0,r},{0,0,-r}};m.faces={{4,0,2},{4,2,1},{4,1,3},{4,3,0},{5,2,0},{5,1,2},{5,3,1},{5,0,3}};m.metrics={{"Volume",std::sqrt(2.0)*a*a*a/3},{"Surface area",2*std::sqrt(3.0)*a*a}};}
    else if(kind=="Cylinder"||kind=="Cone"||kind=="Prism"||kind=="Pyramid"){
        int count=(kind=="Prism"||kind=="Pyramid")?n:64;double baseRadius=(kind=="Prism"||kind=="Pyramid")?circumradiusRegular(count,a):a/2;auto lo=ring(count,baseRadius,-h/2);auto hi=ring(count,baseRadius,h/2);
        if(kind=="Cone"){m.vertices=lo;m.vertices.push_back({0,0,h/2});for(int i=0;i<count;++i)m.faces.push_back({i,(i+1)%count,count});m.faces.push_back([&]{std::vector<int> q;for(int i=count-1;i>=0;--i)q.push_back(i);return q;}());double r=a/2,l=std::hypot(r,h);m.metrics={{"Volume",PI*r*r*h/3},{"Lateral area",PI*r*l},{"Total area",PI*r*(l+r)}};}
        else if(kind=="Pyramid"){m.vertices=lo;m.vertices.push_back({0,0,h/2});m.faces.push_back([&]{std::vector<int> q;for(int i=count-1;i>=0;--i)q.push_back(i);return q;}());for(int i=0;i<count;++i)m.faces.push_back({i,(i+1)%count,count});double base=areaRegular(count,a),slant=std::hypot(h,a/(2*std::tan(PI/count)));m.metrics={{"Volume",base*h/3},{"Base area",base},{"Lateral area",count*a*slant/2}};}
        else {m.vertices=lo;m.vertices.insert(m.vertices.end(),hi.begin(),hi.end());std::vector<int> low,up;for(int i=0;i<count;++i){low.push_back(count-1-i);up.push_back(count+i);}m.faces={low,up};for(int i=0;i<count;++i)m.faces.push_back({i,(i+1)%count,(i+1)%count+count,i+count});double base=(kind=="Prism")?areaRegular(count,a):PI*a*a/4,per=(kind=="Prism")?count*a:PI*a;m.metrics={{"Volume",base*h},{"Base area",base},{"Surface area",2*base+per*h}};}
    }
    else if(kind=="Sphere"){int lat=16,lon=32;double r=a/2;m.smoothShading=true;m.vertices.push_back({0,0,r});for(int j=1;j<lat;++j){double ph=PI*j/lat;for(int i=0;i<lon;++i){double th=2*PI*i/lon;m.vertices.push_back({r*std::sin(ph)*std::cos(th),r*std::sin(ph)*std::sin(th),r*std::cos(ph)});}}int bottom=(int)m.vertices.size();m.vertices.push_back({0,0,-r});for(int i=0;i<lon;++i)m.faces.push_back({0,1+i,1+(i+1)%lon});for(int j=0;j<lat-2;++j)for(int i=0;i<lon;++i){int a0=1+j*lon+i,a1=1+j*lon+(i+1)%lon,b0=a0+lon,b1=a1+lon;m.faces.push_back({a0,b0,b1,a1});}int last=1+(lat-2)*lon;for(int i=0;i<lon;++i)m.faces.push_back({bottom,last+(i+1)%lon,last+i});m.metrics={{"Radius",r},{"Volume",4*PI*r*r*r/3},{"Surface area",4*PI*r*r}};}
    else throw std::invalid_argument("Unknown shape: "+kind);
    orientFacesOutward(m);collectEdges(m);return m;
}

std::vector<Vec3> intersectPlane(const Mesh& m,Vec3 normal,double offset){Vec3 n=normalized(normal);std::vector<Vec3> hits;auto addUnique=[&](Vec3 p){for(auto q:hits)if(length(p-q)<1e-7)return;hits.push_back(p);};for(auto e:m.edges){Vec3 a=m.vertices[e[0]],b=m.vertices[e[1]];double da=dot(n,a)-offset,db=dot(n,b)-offset;if(std::abs(da)<1e-8)addUnique(a);if(std::abs(db)<1e-8)addUnique(b);if(da*db< -1e-14)addUnique(a+(b-a)*(da/(da-db)));}if(hits.size()<3)return hits;Vec3 center{};for(auto p:hits)center=center+p;center=center*(1.0/hits.size());Vec3 ref=std::abs(n.x)<.8?Vec3{1,0,0}:Vec3{0,1,0};Vec3 u=normalized(cross(n,ref)),v=cross(n,u);std::sort(hits.begin(),hits.end(),[&](Vec3 a,Vec3 b){Vec3 x=a-center,y=b-center;return std::atan2(dot(x,v),dot(x,u))<std::atan2(dot(y,v),dot(y,u));});return hits;}

std::string exactRadians(double degrees){long long bestN=0,bestD=1;double best=1e9;for(int d=1;d<=360;++d){long long n=std::llround(degrees*d/180.0);double err=std::abs(degrees-180.0*n/d);if(err<best){best=err;bestN=n;bestD=d;}}if(best>1e-6)return format(degrees*3.14159265358979323846/180,5)+" rad";if(bestN==0)return "0";long long g=std::gcd(std::abs(bestN),bestD);bestN/=g;bestD/=g;std::string s=bestN<0?"-":"";long long n=std::abs(bestN);if(n!=1)s+=std::to_string(n);s+="pi";if(bestD!=1)s+="/"+std::to_string(bestD);return s;}
}
