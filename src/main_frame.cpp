#include "main_frame.hpp"
#include "geometry.hpp"
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/accel.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <wx/settings.h>
#include <wx/dialog.h>
#include <wx/statbox.h>
#include <cmath>

namespace {
enum { ID_TOOL_ORBIT=wxID_HIGHEST+1, ID_TOOL_SELECT, ID_TOOL_POINT, ID_TOOL_SEGMENT, ID_TOOL_LINE, ID_TOOL_RAY, ID_VIEW_ISO, ID_VIEW_TOP,
       ID_VIEW_FRONT, ID_VIEW_RIGHT, ID_GRID, ID_AXES, ID_LABELS, ID_DELETE_POINT, ID_SHAPE,
       ID_PLANE_AXIS, ID_PLANE_OFFSET, ID_PLANE_SHOW, ID_SECTION_SHOW, ID_VECTOR_SHOW, ID_ANGLE, ID_RESET, ID_CLEAR_POINTS, ID_SETTINGS };
const wxColour BG(11,17,24), PANEL(15,25,35), INPUT_COLOR(8,18,27), FG_COLOR(218,232,239), MUTED(135,159,176), ACCENT(85,214,194);

wxString U8(const char* value) { return wxString::FromUTF8(value); }
wxStaticText* heading(wxWindow* parent,const char* value) {
    auto* s=new wxStaticText(parent,wxID_ANY,U8(value));
    s->SetForegroundColour(ACCENT);
    s->SetFont(wxFont(9,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));
    return s;
}
wxSpinCtrlDouble* spin(wxWindow* parent,double value,double min=.01,double max=1000) {
    auto* s=new wxSpinCtrlDouble(parent,wxID_ANY,wxEmptyString,wxDefaultPosition,{95,-1},wxSP_ARROW_KEYS,min,max,value,.25);
    s->SetDigits(2); return s;
}
wxString piText(double d) { wxString s=wxString::FromUTF8(geo::exactRadians(d)); s.Replace("pi","π"); return s; }

void applyTheme(wxWindow* window) {
    if (dynamic_cast<wxPanel*>(window) || dynamic_cast<wxNotebook*>(window) || dynamic_cast<wxSplitterWindow*>(window)) window->SetBackgroundColour(BG);
    for (wxWindow* child : window->GetChildren()) {
        child->SetBackgroundColour(PANEL);
        if (auto* b=dynamic_cast<wxButton*>(child)) { b->SetBackgroundColour(wxColour(20,38,48)); b->SetForegroundColour(ACCENT); }
        if (dynamic_cast<wxChoice*>(child) || dynamic_cast<wxSpinCtrl*>(child) || dynamic_cast<wxSpinCtrlDouble*>(child) || dynamic_cast<wxListBox*>(child) || dynamic_cast<wxTextCtrl*>(child)) {
            child->SetBackgroundColour(INPUT_COLOR); child->SetForegroundColour(FG_COLOR);
        }
        if (auto* label=dynamic_cast<wxStaticText*>(child)) {
            if(label->GetForegroundColour()==wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT))label->SetForegroundColour(FG_COLOR);
        }
        if (auto* group=dynamic_cast<wxStaticBox*>(child)) group->SetForegroundColour(FG_COLOR);
        if (dynamic_cast<wxCheckBox*>(child)) child->SetForegroundColour(FG_COLOR);
        applyTheme(child);
    }
}
}

MainFrame::MainFrame()
    : wxFrame(nullptr,wxID_ANY,"GEOTERMINAL 2.2 // Interactive Geometry Workbench",wxDefaultPosition,{1500,940},wxDEFAULT_FRAME_STYLE|wxCLIP_CHILDREN) {
    SetMinSize({1120,720}); SetBackgroundColour(BG); CreateStatusBar(); GetStatusBar()->SetBackgroundColour(PANEL); GetStatusBar()->SetForegroundColour(ACCENT); SetStatusText("READY  /  C++20 · wxWidgets · OpenGL");
    auto* rootPanel=new wxPanel(this); auto* root=new wxBoxSizer(wxVERTICAL); rootPanel->SetSizer(root);

    auto* brand=new wxPanel(rootPanel); brand->SetBackgroundColour(BG); auto* bh=new wxBoxSizer(wxHORIZONTAL);
    auto* title=new wxStaticText(brand,wxID_ANY,"GEO TERMINAL"); title->SetFont(wxFont(19,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD)); title->SetForegroundColour(FG_COLOR);
    bh->Add(title,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,18);
    auto* sub=new wxStaticText(brand,wxID_ANY,U8("SPATIAL GEOMETRY LAB  /  CONSTRUCT · INTERSECT · MEASURE"));
    sub->SetForegroundColour(MUTED); sub->SetFont(wxFont(9,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL)); bh->Add(sub,0,wxALIGN_CENTER_VERTICAL);
    bh->AddStretchSpacer(); auto* version=new wxStaticText(brand,wxID_ANY,"NATIVE BUILD 2.2"); version->SetForegroundColour(ACCENT);
    bh->Add(version,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,10); brand->SetSizer(bh); root->Add(brand,0,wxEXPAND|wxALL,12);

    buildToolbar(rootPanel,root);
    auto* splitter=new wxSplitterWindow(rootPanel,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSP_LIVE_UPDATE|wxSP_3D);
    view_=new GeometryViewport(splitter); notebook_=new wxNotebook(splitter,wxID_ANY);
    notebook_->AddPage(buildShapePage(),U8("ФИГУРА"));
    notebook_->AddPage(buildConstructionPage(),U8("ПОСТРОЕНИЯ"));
    notebook_->AddPage(buildTrigPage(),U8("ТРИГОНОМЕТРИЯ"));
    notebook_->AddPage(buildHelpPage(),U8("СПРАВКА"));
    splitter->SplitVertically(view_,notebook_,-405); splitter->SetMinimumPaneSize(320); root->Add(splitter,1,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,12);
    view_->setOnSelection([this]{refreshSelection();}); view_->setOnPointAdded([this]{refreshPoints();refreshSelection();});

    auto* menuBar=new wxMenuBar(); auto* file=new wxMenu(); file->Append(wxID_EXIT,U8("Выход\tAlt+F4")); menuBar->Append(file,U8("Файл"));
    auto* camera=new wxMenu(); camera->Append(ID_VIEW_ISO,"Изометрия [0]"); camera->Append(ID_VIEW_TOP,"Сверху XY [1]"); camera->Append(ID_VIEW_FRONT,"Спереди XZ [2]"); camera->Append(ID_VIEW_RIGHT,"Справа YZ [3]"); camera->AppendSeparator();
    camera->AppendCheckItem(ID_GRID,U8("Сетка [G]"))->Check(true); camera->AppendCheckItem(ID_AXES,U8("Оси [A]"))->Check(true); camera->AppendCheckItem(ID_LABELS,U8("Подписи [L]"))->Check(true); camera->Append(ID_TOOL_ORBIT,U8("Вращение [O]")); menuBar->Append(camera,U8("Вид"));
    auto* tools=new wxMenu(); tools->Append(ID_TOOL_SELECT,U8("Выбор [S]")); tools->Append(ID_TOOL_POINT,U8("Добавить точку [P]")); tools->Append(ID_TOOL_SEGMENT,U8("Отрезок [T]")); tools->Append(ID_TOOL_LINE,U8("Прямая [N]")); tools->Append(ID_TOOL_RAY,U8("Луч [Y]")); tools->Append(ID_DELETE_POINT,U8("Удалить выбранную точку [Delete]")); tools->Append(ID_CLEAR_POINTS,U8("Очистить все пользовательские точки")); menuBar->Append(tools,U8("Инструменты"));
    auto* settings=new wxMenu(); settings->Append(ID_SETTINGS,U8("Параметры интерфейса и управления...")); menuBar->Append(settings,U8("Настройки")); SetMenuBar(menuBar);
    Bind(wxEVT_MENU,[this](wxCommandEvent&){Close();},wxID_EXIT);
    Bind(wxEVT_MENU,[this](wxCommandEvent&){setView("ISO");},ID_VIEW_ISO); Bind(wxEVT_MENU,[this](wxCommandEvent&){setView("TOP");},ID_VIEW_TOP);
    Bind(wxEVT_MENU,[this](wxCommandEvent&){setView("FRONT");},ID_VIEW_FRONT); Bind(wxEVT_MENU,[this](wxCommandEvent&){setView("RIGHT");},ID_VIEW_RIGHT);
    Bind(wxEVT_MENU,[this](wxCommandEvent&e){view_->setGrid(e.IsChecked());},ID_GRID); Bind(wxEVT_MENU,[this](wxCommandEvent&e){view_->setAxes(e.IsChecked());},ID_AXES);
    Bind(wxEVT_MENU,[this](wxCommandEvent&e){view_->setLabels(e.IsChecked());},ID_LABELS); Bind(wxEVT_MENU,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Orbit);},ID_TOOL_ORBIT);
    Bind(wxEVT_MENU,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Select);},ID_TOOL_SELECT); Bind(wxEVT_MENU,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Point);},ID_TOOL_POINT);
    Bind(wxEVT_MENU,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Segment);},ID_TOOL_SEGMENT); Bind(wxEVT_MENU,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Line);},ID_TOOL_LINE); Bind(wxEVT_MENU,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Ray);},ID_TOOL_RAY);
    Bind(wxEVT_MENU,[this](wxCommandEvent&){view_->deleteSelectedPoint();refreshPoints();},ID_DELETE_POINT); Bind(wxEVT_MENU,[this](wxCommandEvent&){view_->clearCustomPoints();refreshPoints();},ID_CLEAR_POINTS); Bind(wxEVT_MENU,[this](wxCommandEvent&){view_->resetCamera();},ID_RESET); Bind(wxEVT_MENU,[this](wxCommandEvent&){showSettings();},ID_SETTINGS);
    Bind(wxEVT_CHAR_HOOK,&MainFrame::onKey,this);
    applyTheme(rootPanel); updateShape(); updateConstruction(); Centre();
}

void MainFrame::buildToolbar(wxWindow* parent,wxBoxSizer* root) {
    auto* bar=new wxPanel(parent); bar->SetBackgroundColour(PANEL); auto* s=new wxBoxSizer(wxHORIZONTAL);
    auto add=[&](const wxString& label,int id,auto fn){auto* b=new wxButton(bar,id,label,wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);b->SetBackgroundColour(wxColour(20,38,48));b->SetForegroundColour(ACCENT);b->Bind(wxEVT_BUTTON,fn);s->Add(b,0,wxALL,3);};
    add("NAV [O]",ID_TOOL_ORBIT,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Orbit);});
    add("SELECT [S]",ID_TOOL_SELECT,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Select);});
    add("POINT [P]",ID_TOOL_POINT,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Point);});
    add("SEGMENT [T]",ID_TOOL_SEGMENT,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Segment);});
    add("LINE [N]",ID_TOOL_LINE,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Line);});
    add("RAY [Y]",ID_TOOL_RAY,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Ray);});
    s->Add(new wxStaticLine(bar,wxID_ANY,wxDefaultPosition,{1,24}),0,wxALL,5);
    add("ISO [0]",ID_VIEW_ISO,[this](wxCommandEvent&){setView("ISO");}); add("TOP [1]",ID_VIEW_TOP,[this](wxCommandEvent&){setView("TOP");});
    add("FRONT [2]",ID_VIEW_FRONT,[this](wxCommandEvent&){setView("FRONT");}); add("RIGHT [3]",ID_VIEW_RIGHT,[this](wxCommandEvent&){setView("RIGHT");});
    add("RESET [R]",ID_RESET,[this](wxCommandEvent&){view_->resetCamera();});
    s->Add(new wxStaticText(bar,wxID_ANY,U8("РЕЖИМ:")),0,wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT,8);
    drawModeChoice_=new wxChoice(bar,wxID_ANY);drawModeChoice_->Append(U8("Shaded"));drawModeChoice_->Append(U8("Wireframe"));drawModeChoice_->Append(U8("Shaded + Wire"));drawModeChoice_->SetSelection(2);drawModeChoice_->Bind(wxEVT_CHOICE,[this](wxCommandEvent& e){view_->setDrawMode(static_cast<GeometryViewport::DrawMode>(e.GetSelection()));});s->Add(drawModeChoice_,0,wxALIGN_CENTER_VERTICAL|wxALL,3);
    s->Add(new wxStaticText(bar,wxID_ANY,U8("КАМЕРА:")),0,wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT,6);
    projectionChoice_=new wxChoice(bar,wxID_ANY);projectionChoice_->Append(U8("Perspective"));projectionChoice_->Append(U8("Orthographic"));projectionChoice_->SetSelection(0);projectionChoice_->Bind(wxEVT_CHOICE,[this](wxCommandEvent& e){view_->setPerspective(e.GetSelection()==0);});s->Add(projectionChoice_,0,wxALIGN_CENTER_VERTICAL|wxALL,3);
    bar->SetSizer(s); root->Add(bar,0,wxEXPAND|wxLEFT|wxRIGHT,12);
}

wxWindow* MainFrame::buildShapePage() {
    auto* page=new wxPanel(notebook_); auto* root=new wxBoxSizer(wxVERTICAL); page->SetSizer(root);
    root->Add(heading(page,"01 / ГЕОМЕТРИЧЕСКИЙ ОБЪЕКТ"),0,wxALL,10);
    wxArrayString shapes; for(auto s:{"Isosceles triangle","Right triangle","Square","Rectangle","Circle","Regular polygon","Cube","Cuboid","Tetrahedron","Octahedron","Cylinder","Cone","Prism","Pyramid","Sphere"}) shapes.Add(wxString::FromUTF8(s));
    shape_=new wxChoice(page,ID_SHAPE,wxDefaultPosition,wxDefaultSize,shapes); shape_->SetSelection(6); root->Add(shape_,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);
    shape_->Bind(wxEVT_CHOICE,[this](wxCommandEvent&){updateShape();}); root->Add(heading(page,"02 / ПАРАМЕТРЫ МОДЕЛИ"),0,wxLEFT|wxRIGHT|wxTOP,10);
    auto* grid=new wxFlexGridSizer(2,7,8); const char* names[]={"a / сторона или диаметр","b / сторона B","c / сторона C","h / высота"};
    for(int i=0;i<4;++i){dimLabels_[i]=new wxStaticText(page,wxID_ANY,U8(names[i]));dimLabels_[i]->SetForegroundColour(MUTED);dims_[i]=spin(page,i==0?2:3);grid->Add(dimLabels_[i],0,wxALIGN_CENTER_VERTICAL);grid->Add(dims_[i],0,wxEXPAND);dims_[i]->Bind(wxEVT_SPINCTRLDOUBLE,[this](wxCommandEvent&){updateShape();});}
    sidesLabel_=new wxStaticText(page,wxID_ANY,U8("Сторон основания n"));sidesLabel_->SetForegroundColour(MUTED);grid->Add(sidesLabel_,0,wxALIGN_CENTER_VERTICAL); sides_=new wxSpinCtrl(page,wxID_ANY,"6",wxDefaultPosition,{95,-1},wxSP_ARROW_KEYS,3,64,6); grid->Add(sides_);
    sides_->Bind(wxEVT_SPINCTRL,[this](wxCommandEvent&){updateShape();}); root->Add(grid,0,wxEXPAND|wxALL,10);
    auto* hint=new wxStaticText(page,wxID_ANY,U8("Изменяй параметры: модель, измерения и сечение обновляются сразу.\nДля окружности и сферы параметр a означает диаметр."));hint->SetForegroundColour(MUTED);hint->Wrap(340);root->Add(hint,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    root->Add(heading(page,"03 / АЛГЕБРА И ТОПОЛОГИЯ"),0,wxLEFT|wxRIGHT|wxTOP,10);metrics_=new wxTextCtrl(page,wxID_ANY,"—",wxDefaultPosition,wxSize(-1,140),wxTE_MULTILINE|wxTE_READONLY|wxBORDER_NONE);metrics_->SetBackgroundColour(INPUT_COLOR);metrics_->SetForegroundColour(FG_COLOR);metrics_->SetFont(wxFont(10,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));metrics_->SetMinSize(wxSize(-1,140));root->Add(metrics_,0,wxEXPAND|wxALL,10);
    counts_=new wxStaticText(page,wxID_ANY,"V: 0   E: 0   F: 0");counts_->SetForegroundColour(ACCENT);root->Add(counts_,0,wxLEFT|wxRIGHT|wxBOTTOM,10);root->AddStretchSpacer();
    auto* reset=new wxButton(page,wxID_ANY,U8("СБРОСИТЬ КАМЕРУ  [R]"));reset->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){view_->resetCamera();});root->Add(reset,0,wxEXPAND|wxALL,10);return page;
}

wxWindow* MainFrame::buildConstructionPage() {
    auto* page=new wxScrolledWindow(notebook_,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxVSCROLL);page->SetScrollRate(0,12); auto* root=new wxBoxSizer(wxVERTICAL); page->SetSizer(root);
    root->Add(heading(page,"01 / ИНСТРУМЕНТЫ ПОСТРОЕНИЯ"),0,wxALL,10);auto* tools=new wxGridSizer(3,3,5);
    auto* select=new wxButton(page,wxID_ANY,U8("Выбор [S]"));auto* point=new wxButton(page,wxID_ANY,U8("Точка [P]"));auto* orbit=new wxButton(page,wxID_ANY,U8("Орбита [O]"));auto* segment=new wxButton(page,wxID_ANY,U8("Отрезок [T]"));auto* line=new wxButton(page,wxID_ANY,U8("Прямая [N]"));auto* ray=new wxButton(page,wxID_ANY,U8("Луч [Y]"));auto* del=new wxButton(page,wxID_ANY,U8("Удалить точку"));auto* clearLines=new wxButton(page,wxID_ANY,U8("Очистить линии"));auto* clearPoints=new wxButton(page,wxID_ANY,U8("Очистить точки"));
    for(auto* b:{select,point,orbit,segment,line,ray,del,clearLines,clearPoints}) tools->Add(b,1,wxEXPAND);
    root->Add(tools,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);
    select->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Select);});point->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Point);});orbit->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Orbit);});segment->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Segment);});line->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Line);});ray->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){setTool(GeometryViewport::Tool::Ray);});del->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){view_->deleteSelectedPoint();refreshPoints();});clearLines->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){view_->clearConstructionLines();});clearPoints->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){view_->clearCustomPoints();refreshPoints();});
    selection_=new wxStaticText(page,wxID_ANY,U8("Ничего не выбрано"));selection_->SetForegroundColour(wxColour(255,177,93));selection_->Wrap(350);root->Add(selection_,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);
    coordsCheck_=new wxCheckBox(page,wxID_ANY,U8("Показывать координаты подписанных точек"));coordsCheck_->Bind(wxEVT_CHECKBOX,[this](wxCommandEvent&e){view_->setCoordinateLabels(e.IsChecked());});root->Add(coordsCheck_,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    snapCheck_=new wxCheckBox(page,wxID_ANY,U8("Привязка создаваемых точек к сетке 0.5"));snapCheck_->SetValue(true);snapCheck_->Bind(wxEVT_CHECKBOX,[this](wxCommandEvent&e){view_->setSnapToGrid(e.IsChecked());});root->Add(snapCheck_,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    auto* pointGrid=new wxFlexGridSizer(2,4,4);const char* xyz[]={"X","Y","Z"};for(int i=0;i<3;++i){auto* l=new wxStaticText(page,wxID_ANY,xyz[i]);l->SetForegroundColour(MUTED);pointGrid->Add(l,0,wxALIGN_CENTER_VERTICAL);newPointDims_[i]=spin(page,0,-100,100);pointGrid->Add(newPointDims_[i],0,wxEXPAND);}root->Add(pointGrid,0,wxEXPAND|wxLEFT|wxRIGHT,10);
    auto* addXYZ=new wxButton(page,wxID_ANY,U8("Создать точку по координатам XYZ"));addXYZ->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){view_->addCustomPoint({newPointDims_[0]->GetValue(),newPointDims_[1]->GetValue(),newPointDims_[2]->GetValue()});});root->Add(addXYZ,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);
    root->Add(heading(page,"02 / СЕКУЩАЯ ПЛОСКОСТЬ"),0,wxLEFT|wxRIGHT|wxTOP,10);auto* planeRow=new wxBoxSizer(wxHORIZONTAL);
    planeAxis_=new wxChoice(page,ID_PLANE_AXIS);planeAxis_->Append("XY  /  Z = d");planeAxis_->Append("XZ  /  Y = d");planeAxis_->Append("YZ  /  X = d");planeAxis_->Append("Custom  /  n·x = d");planeAxis_->SetSelection(0);planeRow->Add(planeAxis_,1,wxRIGHT,5);planeOffset_=spin(page,0,-20,20);planeRow->Add(planeOffset_,0);root->Add(planeRow,0,wxEXPAND|wxALL,10);
    auto* normalGrid=new wxFlexGridSizer(2,4,4);const char* normalNames[]={"nx","ny","nz"};double normalDefaults[]={0,0,1};for(int i=0;i<3;++i){planeNormalLabels_[i]=new wxStaticText(page,wxID_ANY,normalNames[i]);planeNormalLabels_[i]->SetForegroundColour(MUTED);normalGrid->Add(planeNormalLabels_[i],0,wxALIGN_CENTER_VERTICAL);planeNormalDims_[i]=spin(page,normalDefaults[i],-10,10);normalGrid->Add(planeNormalDims_[i],0,wxEXPAND);planeNormalDims_[i]->Bind(wxEVT_SPINCTRLDOUBLE,[this](wxCommandEvent&){updateConstruction();});}root->Add(normalGrid,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    auto* throughThree=new wxButton(page,wxID_ANY,U8("Плоскость через первые 3 точки"));root->Add(throughThree,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);throughThree->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){const auto& p=view_->customPoints();if(p.size()<3){SetStatusText(U8("Нужно создать минимум три точки"));return;}geo::Vec3 n=geo::cross(p[1]-p[0],p[2]-p[0]);try{n=geo::normalized(n);}catch(const std::invalid_argument&){SetStatusText(U8("Точки лежат на одной прямой"));return;}planeAxis_->SetSelection(3);planeNormalDims_[0]->SetValue(n.x);planeNormalDims_[1]->SetValue(n.y);planeNormalDims_[2]->SetValue(n.z);planeOffset_->SetValue(geo::dot(n,p[0]));planeCheck_->SetValue(true);updateConstruction();});
    planeCheck_=new wxCheckBox(page,ID_PLANE_SHOW,U8("Показывать плоскость"));sectionCheck_=new wxCheckBox(page,ID_SECTION_SHOW,U8("Показывать контур сечения"));sectionCheck_->SetValue(true);root->Add(planeCheck_,0,wxLEFT|wxRIGHT|wxBOTTOM,10);root->Add(sectionCheck_,0,wxLEFT|wxRIGHT|wxBOTTOM,10);sectionInfo_=new wxStaticText(page,wxID_ANY,U8("Сечение: плоскость не активна"));sectionInfo_->SetForegroundColour(ACCENT);sectionInfo_->Wrap(350);root->Add(sectionInfo_,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    planeAxis_->Bind(wxEVT_CHOICE,[this](wxCommandEvent&){updateConstruction();});planeOffset_->Bind(wxEVT_SPINCTRLDOUBLE,[this](wxCommandEvent&){updateConstruction();});planeCheck_->Bind(wxEVT_CHECKBOX,[this](wxCommandEvent&){updateConstruction();});sectionCheck_->Bind(wxEVT_CHECKBOX,[this](wxCommandEvent&){updateConstruction();});
    root->Add(heading(page,"03 / ВЕКТОР AB"),0,wxLEFT|wxRIGHT|wxTOP,10);vectorCheck_=new wxCheckBox(page,ID_VECTOR_SHOW,U8("Показывать вектор AB"));vectorCheck_->SetValue(true);root->Add(vectorCheck_,0,wxALL,10);
    auto* vg=new wxFlexGridSizer(4,4,4);const char* labels[]={"Ax","Ay","Az","Bx","By","Bz"};double def[]={0,0,0,2,2,1};
    for(int i=0;i<6;++i){auto* l=new wxStaticText(page,wxID_ANY,labels[i]);l->SetForegroundColour(MUTED);vg->Add(l,0,wxALIGN_CENTER_VERTICAL);vectorDims_[i]=spin(page,def[i],-100,100);vg->Add(vectorDims_[i],0,wxEXPAND);vectorDims_[i]->Bind(wxEVT_SPINCTRLDOUBLE,[this](wxCommandEvent&){updateVector();});}
    vectorCheck_->SetValue(false);root->Add(vg,0,wxEXPAND|wxLEFT|wxRIGHT,10);vectorCheck_->Bind(wxEVT_CHECKBOX,[this](wxCommandEvent&){updateVector();});vectorInfo_=new wxStaticText(page,wxID_ANY,"—");vectorInfo_->SetForegroundColour(ACCENT);root->Add(vectorInfo_,0,wxALL,10);
    root->Add(heading(page,"04 / СОЗДАННЫЕ ТОЧКИ"),0,wxLEFT|wxRIGHT|wxTOP,10);points_=new wxListBox(page,wxID_ANY,wxDefaultPosition,{-1,105});root->Add(points_,0,wxEXPAND|wxALL,10);
    points_->Bind(wxEVT_LISTBOX,[this](wxCommandEvent&e){view_->selectCustomPoint(e.GetSelection());refreshSelection();});page->FitInside();return page;
}

wxWindow* MainFrame::buildTrigPage() {
    auto* page=new wxPanel(notebook_);auto* root=new wxBoxSizer(wxVERTICAL);page->SetSizer(root);root->Add(heading(page,"UNIT CIRCLE / ЕДИНИЧНАЯ ОКРУЖНОСТЬ"),0,wxALL,10);
    trig_=new TrigCirclePanel(page);root->Add(trig_,1,wxEXPAND|wxLEFT|wxRIGHT,8);angleSlider_=new wxSlider(page,ID_ANGLE,45,-720,720,wxDefaultPosition,wxDefaultSize,wxSL_HORIZONTAL|wxSL_LABELS);root->Add(angleSlider_,0,wxEXPAND|wxLEFT|wxRIGHT|wxTOP,10);
    auto* angleRow=new wxBoxSizer(wxHORIZONTAL);auto* angleLabel=new wxStaticText(page,wxID_ANY,U8("Угол θ, градусы:"));angleLabel->SetForegroundColour(MUTED);angleRow->Add(angleLabel,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);angleInput_=spin(page,45,-36000,36000);angleInput_->SetDigits(3);angleInput_->SetIncrement(1);angleRow->Add(angleInput_,1,wxRIGHT,8);root->Add(angleRow,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,8);
    angleReadout_=new wxStaticText(page,wxID_ANY,U8("θ = π/4"));angleReadout_->SetFont(wxFont(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));angleReadout_->SetForegroundColour(ACCENT);root->Add(angleReadout_,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    auto angleChanged=[this](wxCommandEvent&){double d=angleInput_->GetValue();angleSlider_->SetValue(std::clamp((int)std::lround(d),-720,720));updateAngle();};
    angleSlider_->Bind(wxEVT_SLIDER,[this](wxCommandEvent&){angleInput_->SetValue(angleSlider_->GetValue());updateAngle();});angleInput_->Bind(wxEVT_SPINCTRLDOUBLE,angleChanged);angleInput_->Bind(wxEVT_TEXT,angleChanged);
    auto* info=new wxStaticText(page,wxID_ANY,U8("Вводи отрицательные и большие углы: например −90°, 450°, 1080°. Ползунок ограничен диапазоном ±720°, числовое поле — ±36000°.\nРадианы: 180° = π, 360° = 2π, 90° = π/2.\nКоординаты точки: (cos θ, sin θ)."));info->SetForegroundColour(MUTED);info->Wrap(350);root->Add(info,0,wxALL,10);updateAngle();return page;
}

wxWindow* MainFrame::buildHelpPage() {
    auto* page=new wxPanel(notebook_);auto* root=new wxBoxSizer(wxVERTICAL);page->SetSizer(root);root->Add(heading(page,"УПРАВЛЕНИЕ VIEWPORT"),0,wxALL,10);
    const wxString text=U8("НАВИГАЦИЯ\nСКМ + движение              — вращение камеры в любом инструменте\nПКМ + движение              — панорамирование\nКолесо мыши                 — масштабирование\nЛКМ                         — действие активного инструмента\nВ режиме NAV ЛКМ + движение — вращение камеры\n\n0  Изометрия       1  Вид XY сверху\n2  Вид XZ спереди   3  Вид YZ справа\nG  Сетка (переключатель)    A  Оси (переключатель)\nL  Подписи (переключатель)  R  Сброс камеры\nO  Режим NAV      S  Выбор объекта\nP  Точка           T  Отрезок\nN  Прямая          Y  Луч\nDelete  Удалить выбранную точку\nEsc  Вернуться к NAV\n\nРЕЖИМЫ ОТОБРАЖЕНИЯ\nShaded — объёмные поверхности с направленным освещением и бликами.\nWireframe — видимые рёбра сплошные, скрытые пунктирные.\nShaded + Wire — объёмные поверхности вместе с рёбрами.\nВ Wireframe глубина поверхностей используется для определения скрытых линий.\nТочки и подписи скрываются поверхностями, если находятся за ними.\nОриентир осей X/Y/Z находится в правом верхнем углу сцены.\nPerspective / Orthographic переключается в верхней панели.\n\nГорячие клавиши не перехватывают ввод в числовых и текстовых полях.\nИнструменты построения используют два щелчка по рабочей плоскости XY.\nВ режиме выбора можно выбрать вершину, ребро или грань.\nТочки вводятся по XYZ, включая отрицательные координаты.\nКнопка «Очистить точки» удаляет все пользовательские точки.\nПлоскость задаётся нормалью n и смещением d либо первыми тремя точками.\nНастройки: видимость сетки/осей/подписей, привязка и чувствительность камеры.\n\nТРИГОНОМЕТРИЯ\nУгол можно вводить числом, включая отрицательные значения и обороты >360°.\nПолзунок — быстрый способ изменения угла; числовое поле — точный ввод.\nСечение: пересечение плоскости n·x=d с рёбрами сетки.\nВектор AB = B−A; длина, середина и направляющие косинусы.\nТригонометрический круг показывает (cos θ, sin θ) и точные π-углы.");
    auto* help=new wxStaticText(page,wxID_ANY,text);help->SetFont(wxFont(9,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));help->SetForegroundColour(FG_COLOR);help->Wrap(360);root->Add(help,0,wxALL,12);root->AddStretchSpacer();return page;
}

void MainFrame::updateShape() {
    int i=shape_->GetSelection();if(i<0)return;geo::Params p{dims_[0]->GetValue(),dims_[1]->GetValue(),dims_[2]->GetValue(),dims_[3]->GetValue(),sides_->GetValue()};
    static const char* names[]={"Triangle","Right triangle","Square","Rectangle","Circle","Regular polygon","Cube","Cuboid","Tetrahedron","Octahedron","Cylinder","Cone","Prism","Pyramid","Sphere"};
    geo::Mesh m;try{m=geo::makeMesh(names[i],p);}catch(const std::exception&e){SetStatusText(wxString::FromUTF8(e.what()));return;}currentMesh_=m;view_->setMesh(m);
    wxString out;for(auto [name,value]:m.metrics)out+=wxString::Format("%-22s %s\n",wxString::FromUTF8(name),wxString::Format("%.4f",value));metrics_->SetValue(out);metrics_->SetMinSize(wxSize(-1,std::max(140,std::max(1,(int)m.metrics.size())*22+8)));
    counts_->SetLabel(wxString::Format(U8("ВЕРШИНЫ: %lu     РЁБРА: %lu     ГРАНИ: %lu"),(unsigned long)m.vertices.size(),(unsigned long)m.edges.size(),(unsigned long)m.faces.size()));
    static const char* aNames[]={"a / основание","a / катет","a / сторона","a / ширина","a / диаметр","a / сторона","a / ребро","a / длина","a / ребро","a / ребро","a / диаметр","a / диаметр","a / сторона основания","a / сторона основания","a / диаметр"};
    dimLabels_[0]->SetLabel(U8(aNames[i]));
    dimLabels_[1]->SetLabel(U8(i==1?"b / второй катет":i==3?"b / высота прямоугольника":i==7?"b / ширина":"b / параметр B"));
    dimLabels_[2]->SetLabel(U8("c / высота"));
    dimLabels_[3]->SetLabel(U8(i==0?"h / высота":i==10||i==11||i==12||i==13?"h / высота тела":"h / параметр"));
    bool vis[4]={true,false,false,false};if(i==1||i==3||i==7)vis[1]=true;if(i==7)vis[2]=true;if(i==0||(i>=10&&i<=13))vis[3]=true;
    for(int j=0;j<4;++j){dimLabels_[j]->Show(vis[j]);dims_[j]->Show(vis[j]);}bool nvis=i==5||i==12||i==13;sidesLabel_->Show(nvis);sides_->Show(nvis);Layout();updateConstruction();
}
void MainFrame::updateConstruction(){int sel=planeAxis_->GetSelection();geo::Vec3 n=sel==1?geo::Vec3{0,1,0}:sel==2?geo::Vec3{1,0,0}:sel==3?geo::Vec3{planeNormalDims_[0]->GetValue(),planeNormalDims_[1]->GetValue(),planeNormalDims_[2]->GetValue()}:geo::Vec3{0,0,1};bool custom=sel==3;for(int i=0;i<3;++i){planeNormalLabels_[i]->Show(custom);planeNormalDims_[i]->Show(custom);}double d=planeOffset_->GetValue();view_->setPlane(n,d,planeCheck_->GetValue(),sectionCheck_->GetValue());try{n=geo::normalized(n);auto cut=geo::intersectPlane(currentMesh_,n,d);if(cut.size()<3)sectionInfo_->SetLabel(U8("Сечение: плоская область не образуется"));else{geo::Vec3 center{};for(auto p:cut)center=center+p;center=center*(1.0/cut.size());geo::Vec3 ref=std::abs(n.x)<.8?geo::Vec3{1,0,0}:geo::Vec3{0,1,0};geo::Vec3 u=geo::normalized(geo::cross(n,ref)),v=geo::cross(n,u);double area=0,perimeter=0;for(size_t i=0;i<cut.size();++i){auto a=cut[i]-center,b=cut[(i+1)%cut.size()]-center;area+=geo::dot(a,u)*geo::dot(b,v)-geo::dot(b,u)*geo::dot(a,v);perimeter+=geo::length(cut[(i+1)%cut.size()]-cut[i]);}sectionInfo_->SetLabel(wxString::Format(U8("Сечение: %lu вершин   S = %.4f   P = %.4f"),(unsigned long)cut.size(),std::abs(area)/2,perimeter));}}catch(const std::invalid_argument&){sectionInfo_->SetLabel(U8("Сечение: нормаль не задана"));}updateVector();Layout();if(auto* scrolled=dynamic_cast<wxScrolledWindow*>(notebook_->GetPage(1)))scrolled->FitInside();}
void MainFrame::updateVector(){geo::Vec3 a{vectorDims_[0]->GetValue(),vectorDims_[1]->GetValue(),vectorDims_[2]->GetValue()},b{vectorDims_[3]->GetValue(),vectorDims_[4]->GetValue(),vectorDims_[5]->GetValue()};geo::Vec3 d=b-a,mid=(a+b)*.5;double len=geo::length(d);view_->setVector(a,b,vectorCheck_->GetValue());if(len<1e-10){vectorInfo_->SetLabel(U8("A и B совпадают: нулевой вектор"));return;}double ax=std::acos(std::clamp(d.x/len,-1.0,1.0))*180/3.14159265358979323846,ay=std::acos(std::clamp(d.y/len,-1.0,1.0))*180/3.14159265358979323846,az=std::acos(std::clamp(d.z/len,-1.0,1.0))*180/3.14159265358979323846;vectorInfo_->SetLabel(wxString::Format(U8("AB = (%.2f, %.2f, %.2f)\n|AB| = %.4f   M = (%.2f, %.2f, %.2f)\nУглы с осями: α %.2f°  β %.2f°  γ %.2f°"),d.x,d.y,d.z,len,mid.x,mid.y,mid.z,ax,ay,az));}
void MainFrame::updateAngle(){double d=angleInput_?angleInput_->GetValue():angleSlider_->GetValue(),r=d*3.14159265358979323846/180;trig_->setAngle(d);angleReadout_->SetLabel(wxString::Format(U8("θ = %s    %.5f rad    sin = %.5f    cos = %.5f"),piText(d),r,std::sin(r),std::cos(r)));}
void MainFrame::refreshSelection(){selection_->SetLabel(view_->selectionDescription());SetStatusText(view_->selectionDescription());}
void MainFrame::refreshPoints(){points_->Clear();int i=1;for(auto p:view_->customPoints())points_->Append(wxString::Format("P%d  (%.2f, %.2f, %.2f)",i++,p.x,p.y,p.z));if(view_->selectedPoint()>=0)points_->SetSelection(view_->selectedPoint());}
void MainFrame::setTool(GeometryViewport::Tool t){view_->setTool(t);switch(t){case GeometryViewport::Tool::Orbit:SetStatusText("TOOL: ORBIT");break;case GeometryViewport::Tool::Select:SetStatusText("TOOL: SELECT VERTEX / EDGE / FACE");break;case GeometryViewport::Tool::Point:SetStatusText("TOOL: CREATE POINT ON XY");break;case GeometryViewport::Tool::Segment:SetStatusText("TOOL: SEGMENT  /  CLICK TWO ENDPOINTS");break;case GeometryViewport::Tool::Line:SetStatusText("TOOL: LINE  /  CLICK TWO POINTS");break;case GeometryViewport::Tool::Ray:SetStatusText("TOOL: RAY  /  CLICK ORIGIN THEN DIRECTION");break;}}
void MainFrame::setView(const wxString& v){view_->setView(v);SetStatusText("CAMERA: "+v);}
void MainFrame::showSettings(){
    wxDialog dialog(this,wxID_ANY,U8("Настройки GeoTerminal"),wxDefaultPosition,{430,390},wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
    auto* root=new wxBoxSizer(wxVERTICAL);
    auto* viewGroup=new wxStaticBoxSizer(wxVERTICAL,&dialog,U8("Отображение"));
    auto* grid=new wxCheckBox(&dialog,wxID_ANY,U8("Координатная сетка"));grid->SetValue(view_->gridEnabled());viewGroup->Add(grid,0,wxALL,5);
    auto* axes=new wxCheckBox(&dialog,wxID_ANY,U8("Оси координат"));axes->SetValue(view_->axesEnabled());viewGroup->Add(axes,0,wxALL,5);
    auto* labels=new wxCheckBox(&dialog,wxID_ANY,U8("Подписи вершин и точек"));labels->SetValue(view_->labelsEnabled());viewGroup->Add(labels,0,wxALL,5);
    auto* coords=new wxCheckBox(&dialog,wxID_ANY,U8("Показывать координаты рядом с точками"));coords->SetValue(view_->coordinateLabelsEnabled());viewGroup->Add(coords,0,wxALL,5);
    auto* snap=new wxCheckBox(&dialog,wxID_ANY,U8("Привязка новых точек к сетке 0.5"));snap->SetValue(view_->snapEnabled());viewGroup->Add(snap,0,wxALL,5);
    root->Add(viewGroup,0,wxEXPAND|wxALL,10);
    auto* cameraGroup=new wxStaticBoxSizer(wxVERTICAL,&dialog,U8("Камера"));
    auto* sensitivityRow=new wxBoxSizer(wxHORIZONTAL);sensitivityRow->Add(new wxStaticText(&dialog,wxID_ANY,U8("Чувствительность вращения")),1,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);
    auto* sensitivity=spin(&dialog,view_->orbitSensitivity(),.1,2.0);sensitivity->SetDigits(2);sensitivity->SetIncrement(.05);sensitivityRow->Add(sensitivity,0);cameraGroup->Add(sensitivityRow,0,wxEXPAND|wxALL,5);
    root->Add(cameraGroup,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);
    auto* buttons=new wxStdDialogButtonSizer();buttons->AddButton(new wxButton(&dialog,wxID_OK));buttons->AddButton(new wxButton(&dialog,wxID_CANCEL));buttons->Realize();root->Add(buttons,0,wxEXPAND|wxALL,10);dialog.SetSizerAndFit(root);applyTheme(&dialog);
    if(dialog.ShowModal()==wxID_OK){view_->setGrid(grid->GetValue());view_->setAxes(axes->GetValue());view_->setLabels(labels->GetValue());view_->setCoordinateLabels(coords->GetValue());view_->setSnapToGrid(snap->GetValue());view_->setOrbitSensitivity(sensitivity->GetValue());}
}

void MainFrame::onKey(wxKeyEvent& e){
    wxWindow* focus=wxWindow::FindFocus();
    for(wxWindow* w=focus;w&&w!=this;w=w->GetParent())if(dynamic_cast<wxTextCtrl*>(w)||dynamic_cast<wxSpinCtrlDouble*>(w)||dynamic_cast<wxSpinCtrl*>(w)){e.Skip();return;}
    if(e.ControlDown()||e.AltDown()||e.MetaDown()){e.Skip();return;}
    int key=e.GetKeyCode();if(key>='a'&&key<='z')key-=32;
    switch(key){
        case '0':setView("ISO");return;case '1':setView("TOP");return;case '2':setView("FRONT");return;case '3':setView("RIGHT");return;
        case 'G':{auto* item=GetMenuBar()->FindItem(ID_GRID);bool v=item?!item->IsChecked():!view_->gridEnabled();if(item)item->Check(v);view_->setGrid(v);return;}
        case 'A':{auto* item=GetMenuBar()->FindItem(ID_AXES);bool v=item?!item->IsChecked():!view_->axesEnabled();if(item)item->Check(v);view_->setAxes(v);return;}
        case 'L':{auto* item=GetMenuBar()->FindItem(ID_LABELS);bool v=item?!item->IsChecked():!view_->labelsEnabled();if(item)item->Check(v);view_->setLabels(v);return;}
        case 'O':setTool(GeometryViewport::Tool::Orbit);return;case 'S':setTool(GeometryViewport::Tool::Select);return;case 'P':setTool(GeometryViewport::Tool::Point);return;
        case 'T':setTool(GeometryViewport::Tool::Segment);return;case 'N':setTool(GeometryViewport::Tool::Line);return;case 'Y':setTool(GeometryViewport::Tool::Ray);return;
        case 'R':view_->resetCamera();return;case WXK_DELETE:view_->deleteSelectedPoint();refreshPoints();return;case WXK_ESCAPE:setTool(GeometryViewport::Tool::Orbit);return;
        default:e.Skip();return;
    }
}
