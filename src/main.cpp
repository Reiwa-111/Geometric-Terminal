#include "main_frame.hpp"
#include <wx/wx.h>

class GeoTerminalApp final : public wxApp {
public:
    bool OnInit() override {
        wxInitAllImageHandlers();
        auto* frame=new MainFrame();
        frame->Show();
        return true;
    }
};
wxIMPLEMENT_APP(GeoTerminalApp);
