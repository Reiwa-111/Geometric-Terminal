#pragma once
#include "viewport.hpp"
#include "trig_circle.hpp"
#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <array>

class MainFrame final : public wxFrame {
public:
    MainFrame();
private:
    GeometryViewport* view_{};
    geo::Mesh currentMesh_;
    wxNotebook* notebook_{};
    wxChoice* drawModeChoice_{}; wxChoice* projectionChoice_{};
    wxChoice* shape_{}; std::array<wxSpinCtrlDouble*,4> dims_{}; wxSpinCtrl* sides_{}; wxStaticText* sidesLabel_{};
    std::array<wxStaticText*,4> dimLabels_{}; wxTextCtrl* metrics_{}; wxStaticText* counts_{};
    wxChoice* planeAxis_{}; wxSpinCtrlDouble* planeOffset_{}; std::array<wxSpinCtrlDouble*,3> planeNormalDims_{}; std::array<wxStaticText*,3> planeNormalLabels_{}; wxCheckBox* planeCheck_{}; wxCheckBox* sectionCheck_{}; wxStaticText* sectionInfo_{};
    wxCheckBox* vectorCheck_{}; wxCheckBox* coordsCheck_{}; wxCheckBox* snapCheck_{}; std::array<wxSpinCtrlDouble*,6> vectorDims_{}; std::array<wxSpinCtrlDouble*,3> newPointDims_{}; wxListBox* points_{}; wxStaticText* selection_{}; wxStaticText* vectorInfo_{};
    TrigCirclePanel* trig_{}; wxSlider* angleSlider_{}; wxSpinCtrlDouble* angleInput_{}; wxStaticText* angleReadout_{};
    void buildToolbar(wxWindow* parent,wxBoxSizer* root); wxWindow* buildShapePage(); wxWindow* buildConstructionPage(); wxWindow* buildTrigPage(); wxWindow* buildHelpPage();
    void updateShape(); void updateConstruction(); void updateVector(); void updateAngle(); void refreshSelection(); void refreshPoints();
    void setTool(GeometryViewport::Tool tool); void onKey(wxKeyEvent& event); void setView(const wxString& name); void showSettings();
};
