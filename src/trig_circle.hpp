#pragma once
#include <wx/panel.h>

class TrigCirclePanel final : public wxPanel {
public:
    explicit TrigCirclePanel(wxWindow* parent);
    void setAngle(double degrees);
private:
    double degrees_=45;
    void onPaint(wxPaintEvent&);
    wxDECLARE_NO_COPY_CLASS(TrigCirclePanel);
};
