#pragma once
#include "geometry.hpp"
#include <wx/glcanvas.h>
#include <wx/string.h>
#include <optional>
#include <functional>

class GeometryViewport final : public wxGLCanvas {
public:
    enum class Tool { Orbit, Select, Point, Segment, Line, Ray };
    enum class DrawMode { Solid, Wireframe, SolidWire };
    GeometryViewport(wxWindow* parent);
    ~GeometryViewport() override;
    void setMesh(const geo::Mesh& mesh);
    void setTool(Tool tool);
    void setDrawMode(DrawMode mode) { drawMode_=mode; Refresh(false); }
    DrawMode drawMode() const { return drawMode_; }
    void setPerspective(bool value) { perspective_=value; Refresh(false); }
    bool perspective() const { return perspective_; }
    void setGrid(bool value); void setAxes(bool value); void setLabels(bool value);
    bool gridEnabled() const { return grid_; } bool axesEnabled() const { return axes_; } bool labelsEnabled() const { return labels_; }
    bool coordinateLabelsEnabled() const { return coordinateLabels_; } bool snapEnabled() const { return snapToGrid_; }
    double orbitSensitivity() const { return orbitSensitivity_; }
    void setCoordinateLabels(bool value);
    void setSnapToGrid(bool value);
    void setPlane(geo::Vec3 normal,double offset,bool visible,bool section);
    void setVector(geo::Vec3 a,geo::Vec3 b,bool visible);
    void setView(const wxString& view);
    void resetCamera();
    void deleteSelectedPoint();
    void selectCustomPoint(int index);
    void addCustomPoint(geo::Vec3 point);
    void clearConstructionLines();
    void clearCustomPoints();
    void setOrbitSensitivity(double value) { orbitSensitivity_=value; }
    const std::vector<geo::Vec3>& customPoints() const { return customPoints_; }
    int selectedPoint() const { return selectedPoint_; }
    int selectedFace() const { return selectedFace_; }
    int selectedEdge() const { return selectedEdge_; }
    wxString selectionDescription() const;
    void setOnSelection(std::function<void()> callback) { onSelection_=std::move(callback); }
    void setOnPointAdded(std::function<void()> callback) { onPointAdded_=std::move(callback); }
private:
    wxGLContext* context_{};
    geo::Mesh mesh_;
    std::vector<geo::Vec3> customPoints_;
    struct ConstructionLine { geo::Vec3 a,b; Tool type; };
    std::vector<ConstructionLine> constructionLines_;
    std::optional<geo::Vec3> pendingLineStart_;
    Tool tool_=Tool::Orbit;
    DrawMode drawMode_=DrawMode::SolidWire;
    double yaw_=-45,pitch_=32,zoom_=1.0,panX_=0,panY_=0,orbitSensitivity_=.55;
    bool grid_=true,axes_=true,labels_=true,coordinateLabels_=false,snapToGrid_=true,planeVisible_=false,sectionVisible_=true,vectorVisible_=false,perspective_=true;
    geo::Vec3 planeNormal_{0,0,1}; double planeOffset_=0;
    geo::Vec3 vectorA_{0,0,0},vectorB_{2,2,1};
    int selectedPoint_=-1,selectedVertex_=-1,selectedEdge_=-1,selectedFace_=-1;
    wxPoint lastMouse_; bool dragging_=false,panDrag_=false;
    GLuint fontBase_=0;
    std::function<void()> onSelection_,onPointAdded_;
    void onPaint(wxPaintEvent&); void onSize(wxSizeEvent&); void onMouse(wxMouseEvent&); void onWheel(wxMouseEvent&);
    void render(); void drawText3D(geo::Vec3 p,const std::string& text); void drawText2D(double x,double y,const std::string& text);
    void drawGrid(); void drawAxes(); void drawPlaneAndSection(); void drawMesh(); void drawVector(); void drawConstructionLines(); void drawAxisGizmo(int width);
    wxPoint project(geo::Vec3 p) const; std::optional<geo::Vec3> worldOnXY(wxPoint p) const; void pick(wxPoint p);
    geo::Vec3 planeNormal() const;
    wxDECLARE_NO_COPY_CLASS(GeometryViewport);
};
