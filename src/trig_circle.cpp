#include "trig_circle.hpp"
#include "geometry.hpp"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>

namespace { constexpr double PI=3.14159265358979323846;
wxString piLabel(double d){std::string s=geo::exactRadians(d);wxString w=wxString::FromUTF8(s);w.Replace("pi","π");return w;}
wxString baseExact(int deg){switch(deg){case 0:return "0";case 30:return "1/2";case 45:return wxString::FromUTF8("√2/2");case 60:return wxString::FromUTF8("√3/2");case 90:return "1";default:return {};}}
wxString exactValue(double d,bool cosine){double a=std::fmod(d,360.0);if(a<0)a+=360;double ref=0;int sinSign=1,cosSign=1;if(a<=90){ref=a;}else if(a<=180){ref=180-a;cosSign=-1;}else if(a<=270){ref=a-180;sinSign=-1;cosSign=-1;}else{ref=360-a;sinSign=-1;}int sign=cosine?cosSign:sinSign;int k=(int)std::round(cosine?90-ref:ref);wxString base=baseExact(k);if(!base.empty()){if(base=="0")return base;return sign<0?wxString("-")+base:base;}double v=cosine?std::cos(d*PI/180):std::sin(d*PI/180);return wxString::Format("%.3f",v);}
}
TrigCirclePanel::TrigCirclePanel(wxWindow* parent):wxPanel(parent,wxID_ANY){SetBackgroundStyle(wxBG_STYLE_PAINT);Bind(wxEVT_PAINT,&TrigCirclePanel::onPaint,this);SetMinSize({330,320});}
void TrigCirclePanel::setAngle(double d){degrees_=d;Refresh(false);}
void TrigCirclePanel::onPaint(wxPaintEvent&){wxAutoBufferedPaintDC dc(this);dc.SetBackground(wxBrush(wxColour(8,14,21)));dc.Clear();auto sz=GetClientSize();int cx=sz.x/2,cy=sz.y/2;int r=std::max(75,std::min(sz.x/2-52,sz.y/2-38));wxFont font(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL);dc.SetFont(font);
    dc.SetPen(wxPen(wxColour(31,48,62),1));for(int i=-4;i<=4;++i){dc.DrawLine(cx+i*r/4,cy-r-18,cx+i*r/4,cy+r+18);dc.DrawLine(cx-r-18,cy+i*r/4,cx+r+18,cy+i*r/4);}
    dc.SetPen(wxPen(wxColour(100,124,143),1.5));dc.DrawLine(cx-r-22,cy,cx+r+24,cy);dc.DrawLine(cx,cy+r+22,cx,cy-r-24);dc.DrawLine(cx+r+24,cy,cx+r+16,cy-5);dc.DrawLine(cx+r+24,cy,cx+r+16,cy+5);dc.DrawLine(cx,cy-r-24,cx-5,cy-r-16);dc.DrawLine(cx,cy-r-24,cx+5,cy-r-16);
    dc.SetTextForeground(wxColour(137,157,174));dc.DrawText(wxString::FromUTF8("−1"),cx-r-16,cy+8);dc.DrawText("1",cx+r-2,cy+8);dc.DrawText("1",cx+8,cy-r-9);dc.DrawText(wxString::FromUTF8("−1"),cx+8,cy+r-5);dc.DrawText(wxString::FromUTF8("cos θ"),cx+r+17,cy+24);dc.DrawText(wxString::FromUTF8("sin θ"),cx+18,cy-r-31);
    dc.SetPen(wxPen(wxColour(71,94,112),1.5));dc.SetBrush(*wxTRANSPARENT_BRUSH);dc.DrawCircle(cx,cy,r);
    const int degs[]={0,30,45,60,90,120,135,150,180,210,225,240,270,300,315,330};
    for(int d:degs){double t=d*PI/180;int x=cx+(int)std::round(r*std::cos(t)),y=cy-(int)std::round(r*std::sin(t));dc.SetPen(wxPen(wxColour(83,107,126),1));dc.DrawLine(cx+(int)(r*.94*std::cos(t)),cy-(int)(r*.94*std::sin(t)),cx+(int)(r*1.04*std::cos(t)),cy-(int)(r*1.04*std::sin(t)));dc.SetTextForeground(wxColour(148,169,185));wxString label=piLabel(d);wxSize ts=dc.GetTextExtent(label);dc.DrawText(label,x-ts.x/2,y-ts.y/2);}
    double t=std::fmod(degrees_,360.0)*PI/180;int px=cx+(int)std::round(r*std::cos(t)),py=cy-(int)std::round(r*std::sin(t));wxColour orange(255,174,91),cyan(81,215,197),purple(151,139,255);
    dc.SetPen(wxPen(orange,3));dc.DrawLine(cx,cy,px,py);dc.SetPen(wxPen(cyan,2,wxPENSTYLE_SHORT_DASH));dc.DrawLine(px,py,px,cy);dc.DrawLine(px,py,cx,py);dc.SetBrush(wxBrush(orange));dc.SetPen(wxPen(orange,1));dc.DrawCircle(px,py,5);
    wxPoint arcPoints[25];for(int i=0;i<25;++i){double angle=t*i/24.0;arcPoints[i]={cx+(int)std::round(27*std::cos(angle)),cy-(int)std::round(27*std::sin(angle))};}dc.SetPen(wxPen(orange,2));dc.DrawLines(25,arcPoints);
    dc.SetTextForeground(orange);dc.DrawText(wxString::FromUTF8("θ = ")+piLabel(degrees_),cx+34,cy-36);
    wxString s=wxString::Format(wxString::FromUTF8("cos θ = %s    sin θ = %s"),exactValue(degrees_,true),exactValue(degrees_,false));dc.SetTextForeground(cyan);dc.DrawText(s,12,sz.y-62);
    wxString tan=std::abs(std::cos(t))<1e-8?"undefined":wxString::Format("%.4f",std::tan(t));dc.SetTextForeground(purple);dc.DrawText(wxString::FromUTF8("tan θ = ")+tan+"     P = ("+wxString::Format("%.4f",std::cos(t))+", "+wxString::Format("%.4f",std::sin(t))+")",12,sz.y-43);dc.SetTextForeground(wxColour(115,139,157));dc.DrawText(wxString::FromUTF8("π = 3.14159265     2π = 6.28318531"),12,sz.y-24);
}
