#include "viewport.hpp"
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/log.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <GL/gl.h>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
constexpr double PI=3.14159265358979323846;
struct CameraBasis { geo::Vec3 right,up,eye; };
CameraBasis cameraBasis(double azimuth,double elevation){
    const double a=azimuth*PI/180.0,e=elevation*PI/180.0;
    geo::Vec3 eye{std::cos(e)*std::cos(a),std::cos(e)*std::sin(a),std::sin(e)};
    geo::Vec3 forward=eye*(-1),worldUp=std::abs(eye.z)>.995?geo::Vec3{0,1,0}:geo::Vec3{0,0,1};
    geo::Vec3 right=geo::normalized(geo::cross(forward,worldUp));
    geo::Vec3 up=geo::normalized(geo::cross(right,forward));
    return {right,up,eye};
}
}

GeometryViewport::GeometryViewport(wxWindow* parent)
    : wxGLCanvas(parent,wxID_ANY,[&]{static int attrs[]={WX_GL_RGBA,WX_GL_DOUBLEBUFFER,WX_GL_DEPTH_SIZE,24,0};return attrs;}(),wxDefaultPosition,wxDefaultSize,wxFULL_REPAINT_ON_RESIZE|wxBORDER_NONE),context_(new wxGLContext(this)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT,&GeometryViewport::onPaint,this);Bind(wxEVT_SIZE,&GeometryViewport::onSize,this);
    Bind(wxEVT_LEFT_DOWN,&GeometryViewport::onMouse,this);Bind(wxEVT_LEFT_UP,&GeometryViewport::onMouse,this);
    Bind(wxEVT_MIDDLE_DOWN,&GeometryViewport::onMouse,this);Bind(wxEVT_MIDDLE_UP,&GeometryViewport::onMouse,this);
    Bind(wxEVT_RIGHT_DOWN,&GeometryViewport::onMouse,this);Bind(wxEVT_RIGHT_UP,&GeometryViewport::onMouse,this);
    Bind(wxEVT_MOTION,&GeometryViewport::onMouse,this);Bind(wxEVT_MOUSEWHEEL,&GeometryViewport::onWheel,this);
}
GeometryViewport::~GeometryViewport(){if(context_){SetCurrent(*context_);if(fontBase_)glDeleteLists(fontBase_,96);delete context_;context_=nullptr;}}
void GeometryViewport::setMesh(const geo::Mesh& m){const bool first=mesh_.vertices.empty();mesh_=m;selectedVertex_=-1;selectedEdge_=-1;selectedFace_=-1;if(first){panX_=panY_=0;double radius=0;for(auto p:m.vertices)radius=std::max(radius,geo::length(p));if(radius>1e-6)zoom_=std::clamp(5.2/radius,.75,3.2);}Refresh(false);}
void GeometryViewport::setTool(Tool t){tool_=t;pendingLineStart_.reset();SetCursor(t==Tool::Select?wxCursor(wxCURSOR_ARROW):t==Tool::Orbit?wxCursor(wxCURSOR_SIZING):wxCursor(wxCURSOR_CROSS));Refresh(false);}
void GeometryViewport::setGrid(bool v){grid_=v;Refresh(false);}void GeometryViewport::setAxes(bool v){axes_=v;Refresh(false);}void GeometryViewport::setLabels(bool v){labels_=v;Refresh(false);}
void GeometryViewport::setCoordinateLabels(bool v){coordinateLabels_=v;Refresh(false);}
void GeometryViewport::setSnapToGrid(bool v){snapToGrid_=v;}
geo::Vec3 GeometryViewport::planeNormal()const{return planeNormal_;}
void GeometryViewport::setPlane(geo::Vec3 n,double o,bool v,bool s){try{planeNormal_=geo::normalized(n);planeOffset_=o;planeVisible_=v;sectionVisible_=s;}catch(const std::invalid_argument&){planeVisible_=false;}Refresh(false);}
void GeometryViewport::setVector(geo::Vec3 a,geo::Vec3 b,bool v){vectorA_=a;vectorB_=b;vectorVisible_=v;Refresh(false);}
void GeometryViewport::setView(const wxString& view){if(view=="TOP"){yaw_=-90;pitch_=89.9;}else if(view=="FRONT"){yaw_=-90;pitch_=0;}else if(view=="RIGHT"){yaw_=0;pitch_=0;}else{yaw_=-45;pitch_=32;}Refresh(false);}
void GeometryViewport::resetCamera(){yaw_=-45;pitch_=32;panX_=panY_=0;double radius=0;for(auto p:mesh_.vertices)radius=std::max(radius,geo::length(p));zoom_=radius>1e-6?std::clamp(7.0/radius,.75,3.2):1;Refresh(false);}
void GeometryViewport::deleteSelectedPoint(){if(selectedPoint_>=0&&selectedPoint_<(int)customPoints_.size()){customPoints_.erase(customPoints_.begin()+selectedPoint_);selectedPoint_=-1;if(onSelection_)onSelection_();Refresh(false);}}
void GeometryViewport::selectCustomPoint(int index){if(index>=0&&index<(int)customPoints_.size()){selectedPoint_=index;selectedVertex_=-1;selectedEdge_=-1;selectedFace_=-1;Refresh(false);}}
void GeometryViewport::addCustomPoint(geo::Vec3 p){customPoints_.push_back(p);selectedPoint_=(int)customPoints_.size()-1;selectedVertex_=-1;selectedEdge_=-1;selectedFace_=-1;if(onPointAdded_)onPointAdded_();if(onSelection_)onSelection_();Refresh(false);}
void GeometryViewport::clearConstructionLines(){constructionLines_.clear();pendingLineStart_.reset();Refresh(false);}
void GeometryViewport::clearCustomPoints(){customPoints_.clear();selectedPoint_=-1;selectedVertex_=-1;selectedEdge_=-1;selectedFace_=-1;pendingLineStart_.reset();if(onPointAdded_)onPointAdded_();if(onSelection_)onSelection_();Refresh(false);}
wxString GeometryViewport::selectionDescription()const{if(selectedPoint_>=0&&selectedPoint_<(int)customPoints_.size()){auto p=customPoints_[selectedPoint_];return wxString::Format("P%d  (%.3f, %.3f, %.3f)",selectedPoint_+1,p.x,p.y,p.z);}if(selectedVertex_>=0&&selectedVertex_<(int)mesh_.vertices.size()){auto p=mesh_.vertices[selectedVertex_];return wxString::Format("Vertex %d  (%.3f, %.3f, %.3f)",selectedVertex_+1,p.x,p.y,p.z);}if(selectedEdge_>=0&&selectedEdge_<(int)mesh_.edges.size()){auto e=mesh_.edges[selectedEdge_];return wxString::Format("Edge %d  |e| = %.3f",selectedEdge_+1,geo::length(mesh_.vertices[e[0]]-mesh_.vertices[e[1]]));}if(selectedFace_>=0)return wxString::Format("Face %d selected",selectedFace_+1);return "Nothing selected";}

void GeometryViewport::onPaint(wxPaintEvent&){wxPaintDC dc(this);SetCurrent(*context_);render();SwapBuffers();}
void GeometryViewport::onSize(wxSizeEvent& e){Refresh(false);e.Skip();}
wxPoint GeometryViewport::project(geo::Vec3 p)const{
    const auto basis=cameraBasis(yaw_,pitch_);
    const auto size=GetClientSize();const double x2=geo::dot(basis.right,p)*zoom_+panX_,y2=geo::dot(basis.up,p)*zoom_+panY_;
    double scale=std::max(1.0,size.y/20.0);
    if(perspective_){const double distance=std::max(1.0,30.0-zoom_*geo::dot(basis.eye,p));scale=size.y/(2*std::tan(22.5*PI/180.0)*distance);}
    return {int(size.x/2+x2*scale),int(size.y/2-y2*scale)};
}
std::optional<geo::Vec3> GeometryViewport::worldOnXY(wxPoint p)const{
    const auto size=GetClientSize();
    const double sx=(p.x-size.x*.5),sy=(size.y*.5-p.y);
    const auto basis=cameraBasis(yaw_,pitch_);
    if(perspective_){
        const double nx=2.0*p.x/std::max(1,size.x)-1.0,ny=1.0-2.0*p.y/std::max(1,size.y),t=std::tan(22.5*PI/180.0),aspect=double(size.x)/std::max(1,size.y);
        const double ax=zoom_*basis.right.x+nx*t*aspect*zoom_*basis.eye.x,bx=zoom_*basis.right.y+nx*t*aspect*zoom_*basis.eye.y;
        const double ay=zoom_*basis.up.x+ny*t*zoom_*basis.eye.x,by=zoom_*basis.up.y+ny*t*zoom_*basis.eye.y;
        const double rx=nx*t*aspect*30.0-panX_,ry=ny*t*30.0-panY_,det=ax*by-bx*ay;
        if(std::abs(det)<1e-9)return std::nullopt;
        return geo::Vec3{(rx*by-bx*ry)/det,(ax*ry-rx*ay)/det,0};
    }
    const double base=std::max(1.0,size.y/20.0);
    const double worldX=(sx/base-panX_)/zoom_,worldY=(sy/base-panY_)/zoom_;
    const double det=basis.right.x*basis.up.y-basis.right.y*basis.up.x;
    if(std::abs(det)<.08)return std::nullopt;
    return geo::Vec3{(worldX*basis.up.y-worldY*basis.right.y)/det,(worldY*basis.right.x-worldX*basis.up.x)/det,0};
}
void GeometryViewport::onMouse(wxMouseEvent& e){
    const wxPoint pos=e.GetPosition();
    if(e.LeftDown()){
        SetFocus();
        if(tool_==Tool::Point||tool_==Tool::Segment||tool_==Tool::Line||tool_==Tool::Ray){
            auto maybePoint=worldOnXY(pos);if(!maybePoint){wxBell();return;}auto p=*maybePoint;if(snapToGrid_){p.x=std::round(p.x*2)/2;p.y=std::round(p.y*2)/2;}
            if(tool_==Tool::Point){customPoints_.push_back(p);selectedPoint_=(int)customPoints_.size()-1;selectedVertex_=-1;selectedEdge_=-1;selectedFace_=-1;if(onPointAdded_)onPointAdded_();if(onSelection_)onSelection_();}
            else if(!pendingLineStart_)pendingLineStart_=p;else{constructionLines_.push_back({*pendingLineStart_,p,tool_});pendingLineStart_.reset();}
            Refresh(false);return;
        }
        if(tool_==Tool::Select){pick(pos);return;}
        dragging_=true;panDrag_=false;lastMouse_=pos;CaptureMouse();
    }else if(e.MiddleDown()){
        // Keep camera navigation independent of the active modeling tool.
        dragging_=true;panDrag_=e.ShiftDown();lastMouse_=pos;CaptureMouse();
    }else if(e.RightDown()){
        dragging_=true;panDrag_=true;lastMouse_=pos;CaptureMouse();
    }else if(e.Dragging()&&dragging_){
        wxPoint d=pos-lastMouse_;
        if(panDrag_){double s=std::max(1.0,GetClientSize().y/20.0);panX_+=d.x/s;panY_-=d.y/s;}
        else{yaw_+=d.x*orbitSensitivity_;pitch_=std::clamp(pitch_-d.y*orbitSensitivity_,-89.0,89.0);}
        lastMouse_=pos;Refresh(false);
    }else if(e.LeftUp()||e.MiddleUp()||e.RightUp()){
        dragging_=false;if(HasCapture())ReleaseMouse();
    }else if(e.Moving()&&(tool_==Tool::Point||tool_==Tool::Segment||tool_==Tool::Line||tool_==Tool::Ray))SetCursor(wxCursor(wxCURSOR_CROSS));
}
void GeometryViewport::onWheel(wxMouseEvent& e){zoom_=std::clamp(zoom_*(e.GetWheelRotation()>0?1.12:1/1.12),.15,8.0);Refresh(false);}
void GeometryViewport::pick(wxPoint p){
    const auto basis=cameraBasis(yaw_,pitch_);
    auto depth=[&](geo::Vec3 v){return geo::dot(basis.eye,v);};
    double best=14,bestDepth=-1e30;int found=-1,foundCustom=-1;
    for(int i=0;i<(int)mesh_.vertices.size();++i){wxPoint q=project(mesh_.vertices[i]);double d=std::hypot(q.x-p.x,q.y-p.y),z=depth(mesh_.vertices[i]);if(d<=14&&(d<best-1e-6||(std::abs(d-best)<.25&&z>bestDepth))){best=d;bestDepth=z;found=i;foundCustom=-1;}}
    for(int i=0;i<(int)customPoints_.size();++i){wxPoint q=project(customPoints_[i]);double d=std::hypot(q.x-p.x,q.y-p.y),z=depth(customPoints_[i]);if(d<=14&&(d<best-1e-6||(std::abs(d-best)<.25&&z>bestDepth))){best=d;bestDepth=z;found=-1;foundCustom=i;}}
    selectedPoint_=-1;selectedVertex_=-1;selectedEdge_=-1;selectedFace_=-1;
    if(found>=0||foundCustom>=0){selectedVertex_=found;selectedPoint_=foundCustom;}
    else {
        double edgeBest=8,edgeDepth=-1e30;int edge=-1;
        for(int i=0;i<(int)mesh_.edges.size();++i){auto e=mesh_.edges[i];wxPoint a=project(mesh_.vertices[e[0]]),b=project(mesh_.vertices[e[1]]);double dx=b.x-a.x,dy=b.y-a.y,den=dx*dx+dy*dy;double t=den>0?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/den,0.0,1.0):0;double d=std::hypot(p.x-(a.x+t*dx),p.y-(a.y+t*dy));double z=depth(mesh_.vertices[e[0]])*(1-t)+depth(mesh_.vertices[e[1]])*t;if(d<edgeBest-1e-6||(std::abs(d-edgeBest)<.25&&z>edgeDepth)){edgeBest=d;edgeDepth=z;edge=i;}}
        if(edge>=0)selectedEdge_=edge;
        else {double faceBest=1e9,faceDepth=-1e30;for(int i=0;i<(int)mesh_.faces.size();++i){const auto& f=mesh_.faces[i];bool inside=false;for(size_t j=0,k=f.size()-1;j<f.size();k=j++){wxPoint a=project(mesh_.vertices[f[j]]),b=project(mesh_.vertices[f[k]]);if(((a.y>p.y)!=(b.y>p.y))&&(p.x<(b.x-a.x)*(p.y-a.y)/double((b.y-a.y)==0?1:b.y-a.y)+a.x))inside=!inside;}if(inside){geo::Vec3 c{};for(int idx:f)c=c+mesh_.vertices[idx];c=c*(1.0/f.size());wxPoint q=project(c);double d=std::hypot(q.x-p.x,q.y-p.y),z=depth(c);if(d<faceBest-1e-6||(std::abs(d-faceBest)<.25&&z>faceDepth)){faceBest=d;faceDepth=z;selectedFace_=i;}}}}
    }
    if(onSelection_) onSelection_();
    Refresh(false);
}

void GeometryViewport::drawText3D(geo::Vec3 p,const std::string& s){if(!fontBase_)return;glRasterPos3d(p.x,p.y,p.z);glListBase(fontBase_-32);glCallLists((GLsizei)s.size(),GL_UNSIGNED_BYTE,s.c_str());}
void GeometryViewport::drawText2D(double x,double y,const std::string& s){if(!fontBase_)return;glRasterPos2d(x,y);glListBase(fontBase_-32);glCallLists((GLsizei)s.size(),GL_UNSIGNED_BYTE,s.c_str());}

void GeometryViewport::drawGrid(){if(!grid_)return;glDisable(GL_LIGHTING);glLineWidth(1);glColor4f(.075f,.11f,.15f,1);glBegin(GL_LINES);for(int i=-19;i<=19;++i){double q=i*.5;if(i%2==0)continue;glVertex3d(q,-10,0);glVertex3d(q,10,0);glVertex3d(-10,q,0);glVertex3d(10,q,0);}glEnd();glColor4f(.12f,.18f,.23f,1);glBegin(GL_LINES);for(int i=-10;i<=10;++i){glVertex3d(i,-10,0);glVertex3d(i,10,0);glVertex3d(-10,i,0);glVertex3d(10,i,0);}glEnd();glColor4f(.20f,.29f,.35f,1);glBegin(GL_LINES);for(int i=-10;i<=10;++i){glVertex3d(i,-.08,0);glVertex3d(i,.08,0);glVertex3d(-.08,i,0);glVertex3d(.08,i,0);}glEnd();if(labels_){for(int i=-10;i<=10;i+=2){if(i==0)continue;drawText3D({(double)i,-.28,0},std::to_string(i));drawText3D({.12,(double)i,0},std::to_string(i));}}}
void GeometryViewport::drawAxes(){if(!axes_)return;glLineWidth(2);glBegin(GL_LINES);glColor3f(.98f,.34f,.43f);glVertex3d(-11,0,0);glVertex3d(11,0,0);glColor3f(.35f,.86f,.58f);glVertex3d(0,-11,0);glVertex3d(0,11,0);glColor3f(.36f,.62f,1);glVertex3d(0,0,-11);glVertex3d(0,0,11);glEnd();glPointSize(6);glBegin(GL_POINTS);glColor3f(.98f,.34f,.43f);glVertex3d(11,0,0);glColor3f(.35f,.86f,.58f);glVertex3d(0,11,0);glColor3f(.36f,.62f,1);glVertex3d(0,0,11);glEnd();if(labels_){drawText3D({11.2,0,0},"X");drawText3D({0,11.2,0},"Y");drawText3D({0,0,11.2},"Z");drawText3D({.15,.15,.15},"O");}}
void GeometryViewport::drawPlaneAndSection(){
    if(!planeVisible_)return;
    geo::Vec3 n=planeNormal(),ref=std::abs(n.x)<.8?geo::Vec3{1,0,0}:geo::Vec3{0,1,0};
    geo::Vec3 u=geo::normalized(geo::cross(n,ref)),v=geo::cross(n,u),c=n*planeOffset_;
    // Fit the cutting plane to the current object instead of drawing a giant
    // viewport-sized sheet that obscures the construction.
    double extent=.5;
    for(auto p:mesh_.vertices){auto d=p-c;extent=std::max(extent,std::max(std::abs(geo::dot(d,u)),std::abs(geo::dot(d,v))));}
    const double s=std::max(1.0,extent*1.18);
    const geo::Vec3 corners[]={c-u*s-v*s,c+u*s-v*s,c+u*s+v*s,c-u*s+v*s};
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    glColor4f(.30f,.62f,.92f,.055f);glBegin(GL_QUADS);for(auto p:corners)glVertex3d(p.x,p.y,p.z);glEnd();
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);
    glEnable(GL_LINE_STIPPLE);glLineStipple(1,0x0F0F);glColor3f(.28f,.55f,.76f);glLineWidth(1.0f);glBegin(GL_LINE_LOOP);for(auto p:corners)glVertex3d(p.x,p.y,p.z);glEnd();glDisable(GL_LINE_STIPPLE);
    if(sectionVisible_&&!mesh_.vertices.empty()){
        auto points=geo::intersectPlane(mesh_,n,planeOffset_);
        if(points.size()>=3){
            // A section is a visible cut cap, not another surface competing
            // with the original depth buffer. Draw it as a foreground overlay
            // so the whole cross-section remains readable on the solid body.
            glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(1.0f,.48f,.08f,.62f);glBegin(GL_POLYGON);for(auto p:points)glVertex3d(p.x,p.y,p.z);glEnd();
            glDisable(GL_BLEND);glDepthMask(GL_TRUE);
            glColor3f(1.0f,.72f,.22f);glLineWidth(3.0f);glBegin(GL_LINE_LOOP);for(auto p:points)glVertex3d(p.x,p.y,p.z);glEnd();
            glPointSize(8);glBegin(GL_POINTS);for(auto p:points)glVertex3d(p.x,p.y,p.z);glEnd();
            glEnable(GL_DEPTH_TEST);
        }
    }
    glDepthFunc(GL_LESS);
}
void GeometryViewport::drawMesh(){
    const bool solid=drawMode_!=DrawMode::Wireframe;
    const bool wire=drawMode_!=DrawMode::Solid;
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glFrontFace(GL_CCW);
    // In wireframe mode, first write only the depth of front-facing surfaces.
    // This hides rear edges without painting any faces.
    if(!solid){glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);}
    if(solid||wire){
        glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);glEnable(GL_COLOR_MATERIAL);glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);glShadeModel(mesh_.smoothShading?GL_SMOOTH:GL_FLAT);
        const GLfloat ambient[]={.20f,.22f,.26f,1},diffuse[]={.96f,.97f,1,1},specular[]={.42f,.46f,.52f,1},position[]={-4.f,5.f,8.f,0.f},materialSpecular[]={.32f,.36f,.42f,1};
        glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);glLightfv(GL_LIGHT0,GL_SPECULAR,specular);glLightfv(GL_LIGHT0,GL_POSITION,position);glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,materialSpecular);glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,36.f);
        for(size_t i=0;i<mesh_.faces.size();++i){const auto& f=mesh_.faces[i];if(f.size()<3)continue;const auto a=mesh_.vertices[f[0]],b=mesh_.vertices[f[1]],c=mesh_.vertices[f[2]];auto normal=geo::cross(b-a,c-a);const double nl=geo::length(normal);if(nl<1e-12)continue;normal=normal*(1/nl);if(solid)glColor4f((int)i==selectedFace_?.98f:.30f,(int)i==selectedFace_?.68f:.55f,(int)i==selectedFace_?.20f:.68f,1);glBegin(GL_POLYGON);if(!mesh_.smoothShading)glNormal3d(normal.x,normal.y,normal.z);for(int idx:f){auto p=mesh_.vertices[idx];if(mesh_.smoothShading){auto vn=geo::normalized(p);glNormal3d(vn.x,vn.y,vn.z);}glVertex3d(p.x,p.y,p.z);}glEnd();}
    }
    if(!solid)glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glDisable(GL_LIGHTING);glDisable(GL_COLOR_MATERIAL);glDisable(GL_CULL_FACE);
    if(wire){
        // First draw occluded edge fragments as dashed construction lines,
        // then draw the visible fragments as continuous strokes.
        glDepthMask(GL_FALSE);glEnable(GL_LINE_STIPPLE);glLineStipple(1,0x00FF);glDepthFunc(GL_GREATER);glLineWidth(1.4f);glColor3f(.48f,.58f,.68f);
        glBegin(GL_LINES);for(size_t i=0;i<mesh_.edges.size();++i){if((int)i==selectedEdge_)continue;auto e=mesh_.edges[i];auto a=mesh_.vertices[e[0]],b=mesh_.vertices[e[1]];glVertex3d(a.x,a.y,a.z);glVertex3d(b.x,b.y,b.z);}glEnd();glDisable(GL_LINE_STIPPLE);glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);glLineWidth(1.9f);
        glColor3f(.30f,.86f,.83f);glBegin(GL_LINES);for(size_t i=0;i<mesh_.edges.size();++i){if((int)i==selectedEdge_)continue;auto e=mesh_.edges[i];auto a=mesh_.vertices[e[0]],b=mesh_.vertices[e[1]];glVertex3d(a.x,a.y,a.z);glVertex3d(b.x,b.y,b.z);}glEnd();
    }
    if(selectedEdge_>=0&&selectedEdge_<(int)mesh_.edges.size()){auto e=mesh_.edges[selectedEdge_];auto a=mesh_.vertices[e[0]],b=mesh_.vertices[e[1]];glLineWidth(3.5f);glColor3f(1,.68f,.25f);glBegin(GL_LINES);glVertex3d(a.x,a.y,a.z);glVertex3d(b.x,b.y,b.z);glEnd();}
    // Points and labels use the same depth buffer as the model. A point behind
    // a face is hidden; a point on a visible vertex survives LEQUAL.
    glDepthFunc(GL_LEQUAL);glPointSize(7);glColor3f(.92f,.97f,1);glBegin(GL_POINTS);for(auto p:mesh_.vertices)glVertex3d(p.x,p.y,p.z);glEnd();
    if(selectedVertex_>=0&&selectedVertex_<(int)mesh_.vertices.size()){auto p=mesh_.vertices[selectedVertex_];glPointSize(13);glColor3f(1,.65f,.22f);glBegin(GL_POINTS);glVertex3d(p.x,p.y,p.z);glEnd();}
    for(size_t i=0;i<customPoints_.size();++i){auto p=customPoints_[i];glPointSize((int)i==selectedPoint_?13:9);glColor3f((int)i==selectedPoint_?1.0f:.98f,(int)i==selectedPoint_?.65f:.48f,.22f);glBegin(GL_POINTS);glVertex3d(p.x,p.y,p.z);glEnd();}
    const auto labelOffset=cameraBasis(yaw_,pitch_).eye*.025;
    if(labels_&&mesh_.vertices.size()<=48){for(size_t i=0;i<mesh_.vertices.size();++i){auto p=mesh_.vertices[i];std::string label(1,char('A'+(i%26)));if(coordinateLabels_)label+="("+geo::format(p.x,1)+","+geo::format(p.y,1)+","+geo::format(p.z,1)+")";drawText3D(p+labelOffset,label);}}
    if(labels_){for(size_t i=0;i<customPoints_.size();++i){auto p=customPoints_[i];std::string label="P"+std::to_string(i+1);if(coordinateLabels_)label+="("+geo::format(p.x,1)+","+geo::format(p.y,1)+","+geo::format(p.z,1)+")";drawText3D(p+labelOffset,label);}}
    glDepthFunc(GL_LESS);
}
void GeometryViewport::drawVector(){if(!vectorVisible_)return;geo::Vec3 d=vectorB_-vectorA_;double len=geo::length(d);if(len<1e-9)return;geo::Vec3 dir=d*(1/len),ref=std::abs(dir.z)<.85?geo::Vec3{0,0,1}:geo::Vec3{0,1,0};geo::Vec3 side=geo::normalized(geo::cross(dir,ref))*.12;geo::Vec3 back=vectorB_-dir*.28;glLineWidth(3);glColor3f(1,.68f,.30f);glBegin(GL_LINES);glVertex3d(vectorA_.x,vectorA_.y,vectorA_.z);glVertex3d(vectorB_.x,vectorB_.y,vectorB_.z);glVertex3d(vectorB_.x,vectorB_.y,vectorB_.z);glVertex3d((back+side).x,(back+side).y,(back+side).z);glVertex3d(vectorB_.x,vectorB_.y,vectorB_.z);glVertex3d((back-side).x,(back-side).y,(back-side).z);glEnd();glPointSize(8);glBegin(GL_POINTS);glVertex3d(vectorA_.x,vectorA_.y,vectorA_.z);glVertex3d(vectorB_.x,vectorB_.y,vectorB_.z);glEnd();drawText3D(vectorA_+geo::Vec3{.1,.1,.1},"A");drawText3D(vectorB_+geo::Vec3{.1,.1,.1},"B");}
void GeometryViewport::drawConstructionLines(){for(size_t i=0;i<constructionLines_.size();++i){auto line=constructionLines_[i];geo::Vec3 d=line.b-line.a;double len=geo::length(d);if(len<1e-9)continue;geo::Vec3 u=d*(1/len),a=line.a,b=line.b;if(line.type==Tool::Line){a=line.a-u*40;b=line.b+u*40;}else if(line.type==Tool::Ray)b=line.a+u*40;glLineWidth(line.type==Tool::Segment?3:2);glColor3f(line.type==Tool::Segment?.98f:.72f,line.type==Tool::Segment?.69f:.57f,.30f);glBegin(GL_LINES);glVertex3d(a.x,a.y,a.z);glVertex3d(b.x,b.y,b.z);glEnd();std::string label=(line.type==Tool::Segment?"S":line.type==Tool::Line?"L":"R")+std::to_string(i+1);drawText3D((line.a+line.b)*.5+geo::Vec3{.08,.08,.08},label);}if(pendingLineStart_){glPointSize(10);glColor3f(1,.75f,.3f);glBegin(GL_POINTS);glVertex3d(pendingLineStart_->x,pendingLineStart_->y,pendingLineStart_->z);glEnd();drawText3D(*pendingLineStart_+geo::Vec3{.1,.1,.1},"A?");}}

void GeometryViewport::drawAxisGizmo(int width){
    const auto basis=cameraBasis(yaw_,pitch_);
    struct Axis{double x,y;float r,g,b;const char* label;};
    Axis a[]={{basis.right.x,basis.up.x,.98f,.36f,.42f,"X"},{basis.right.y,basis.up.y,.35f,.88f,.57f,"Y"},{basis.right.z,basis.up.z,.38f,.65f,1,"Z"}};
    const double cx=width-66,cy0=66,len=34;
    glDisable(GL_DEPTH_TEST);glLineWidth(3);glBegin(GL_LINES);for(auto q:a){glColor3f(q.r,q.g,q.b);glVertex2d(cx,cy0);glVertex2d(cx+q.x*len,cy0-q.y*len);}glEnd();
    for(auto q:a){glColor3f(q.r,q.g,q.b);drawText2D(cx+q.x*(len+8)-3,cy0-q.y*(len+8)+4,q.label);}
    glEnable(GL_DEPTH_TEST);
}

void GeometryViewport::render(){const wxSize sz=GetClientSize();if(sz.x<=0||sz.y<=0)return;glViewport(0,0,sz.x,sz.y);glClearColor(.025f,.039f,.055f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_LIGHTING);glDisable(GL_CULL_FACE);glMatrixMode(GL_PROJECTION);glLoadIdentity();double aspect=double(sz.x)/sz.y,span=10.0;if(perspective_){const double nearPlane=1.0,top=nearPlane*std::tan(22.5*PI/180.0),right=top*aspect;glFrustum(-right,right,-top,top,nearPlane,100.0);}else glOrtho(-span*aspect,span*aspect,-span,span,.1,100);glMatrixMode(GL_MODELVIEW);glLoadIdentity();glTranslated(panX_,panY_,-30);const auto basis=cameraBasis(yaw_,pitch_);const double f[16]={basis.right.x,basis.up.x,basis.eye.x,0,basis.right.y,basis.up.y,basis.eye.y,0,basis.right.z,basis.up.z,basis.eye.z,0,0,0,0,1};glMultMatrixd(f);
#ifdef _WIN32
    if(!fontBase_){fontBase_=glGenLists(96);HDC hdc=wglGetCurrentDC();HFONT font=CreateFontA(-12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_MODERN,"Consolas");HGDIOBJ old=SelectObject(hdc,font);wglUseFontBitmapsA(hdc,32,96,fontBase_);SelectObject(hdc,old);DeleteObject(font);}
#endif
    glScaled(zoom_,zoom_,zoom_);drawGrid();drawAxes();drawMesh();drawPlaneAndSection();drawConstructionLines();drawVector();glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,sz.x,sz.y,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glDisable(GL_DEPTH_TEST);glColor3f(.82f,.87f,.92f);drawText2D(12,20,"MMB ORBIT  /  RMB PAN  /  WHEEL ZOOM  /  LMB ACTIVE TOOL");drawText2D(12,sz.y-12,"0 ISO  1 TOP  2 FRONT  3 RIGHT     G GRID  A AXES  L LABELS  R RESET");drawAxisGizmo(sz.x);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);}
