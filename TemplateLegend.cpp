/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/colordlg.h>
#include <wx/colourdata.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcgraph.h>

#include "logger.h"
#include "GdaConst.h"
#include "TemplateCanvas.h"
#include "TemplateFrame.h"
#include "TemplateLegend.h"

///////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////

GdaLegendLabel::GdaLegendLabel(wxString _text, wxPoint _pos, wxSize _sz)
: bbox(_pos, _sz)
{
    text = _text;
    position = _pos;
    size = _sz;
    isMoving = false;
}

GdaLegendLabel::~GdaLegendLabel()
{
    
}

const wxRect& GdaLegendLabel::getBBox()
{
    return bbox;
}

void GdaLegendLabel::move(const wxPoint& new_pos)
{
    tmp_position = new_pos;
}

void GdaLegendLabel::reset()
{
    
}

bool GdaLegendLabel::intersect( GdaLegendLabel& another_lbl)
{
    
    
    return bbox.Intersects(another_lbl.getBBox());
}

bool  GdaLegendLabel::contains(const wxPoint& cur_pos)
{
    return bbox.Contains(cur_pos);
}

void GdaLegendLabel::draw(wxDC& dc)
{
    dc.DrawText(text, position.x, position.y);
    //dc.DrawRectangle(bbox);
}

void GdaLegendLabel::drawMove(wxDC& dc)
{
    dc.DrawText(text, tmp_position.x, tmp_position.y);
    dc.DrawRectangle(tmp_position, size);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
const int TemplateLegend::ID_CATEGORY_COLOR = wxID_HIGHEST + 1;

IMPLEMENT_ABSTRACT_CLASS(TemplateLegend, wxScrolledWindow)

BEGIN_EVENT_TABLE(TemplateLegend, wxScrolledWindow)
	EVT_MENU(TemplateLegend::ID_CATEGORY_COLOR, TemplateLegend::OnCategoryColor)
	EVT_MOUSE_EVENTS(TemplateLegend::OnEvent)
END_EVENT_TABLE()

TemplateLegend::TemplateLegend(wxWindow *parent,
							   TemplateCanvas* template_canvas_s,
							   const wxPoint& pos, const wxSize& size)
: wxScrolledWindow(parent, wxID_ANY, pos, size,
				   wxBORDER_SUNKEN | wxVSCROLL | wxHSCROLL),
legend_background_color(GdaConst::legend_background_color),
template_canvas(template_canvas_s),
isLeftDown(false),
select_label(NULL)
{
	SetBackgroundColour(GdaConst::legend_background_color);
    d_rect = 20;
    px = 10;
    py = 40;
    m_w = 15;
    m_l = 20;
    
    wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
    box->Add(this, 1, wxEXPAND|wxALL);
    parent->SetSizer(box);
}

TemplateLegend::~TemplateLegend()
{
    for (int i=0; i<labels.size(); i++) {
        delete labels[i];
    }
    labels.clear();
}

void TemplateLegend::OnEvent(wxMouseEvent& event)
{
	if (!template_canvas) return;
	
	int cat_clicked = GetCategoryClick(event);
	
    if (event.RightUp()) {
        wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_VIEW_MENU_LEGEND");
		AddCategoryColorToMenu(optMenu, cat_clicked);
    	wxMenuItem* mi = optMenu->FindItem(XRCID("ID_LEGEND_USE_SCI_NOTATION"));
    	if (mi && mi->IsCheckable()) {
            mi->Check(template_canvas->useScientificNotation);
        }
        PopupMenu(optMenu, event.GetPosition());
        return;
    }
	
    if (event.LeftDown()) {
        isLeftDown = true;
        for (int i=0;i<labels.size();i++) {
            if (labels[i]->contains(event.GetPosition())){
                select_label = labels[i];
                break;
            }
        }
    } else if (event.Moving()) {
        if (isLeftDown) {
            isLeftMove = true;
            // moving
            if (select_label) {
                select_label->move(event.GetPosition());
            }
        }
    } else if (event.LeftUp()) {
        if (isLeftMove) {
            isLeftMove = false;
            // stop move
            if (select_label) {
                for (int i=0; i<labels.size(); i++) {
                    if (labels[i] != select_label) {
                        if (select_label->intersect(*labels[i])){
                            // exchange
                        }
                    }
                }
            }
            select_label = NULL;
        } else {
            // only left click
            if (cat_clicked != -1) {
                SelectAllInCategory(cat_clicked, event.ShiftDown());
            }
        }
        isLeftDown = false;
    }
}

int TemplateLegend::GetCategoryClick(wxMouseEvent& event)
{
	wxPoint pt(event.GetPosition());
	int x, y;
	CalcUnscrolledPosition(pt.x, pt.y, &x, &y);
	int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
	
	int cur_y = py;
	for (int i = 0; i<num_cats; i++) {
		if ((x > px) && (x < px + m_l) &&
			(y > cur_y - 8) && (y < cur_y + m_w))
		{
			return i;
		} else if ((x > px + m_l) &&
				   (y > cur_y - 8) && (y < cur_y + m_w)) {
			return -1;
		}
		cur_y += d_rect;
	}
	
	return -1;
}

void TemplateLegend::AddCategoryColorToMenu(wxMenu* menu, int cat_clicked)
{
	int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
	if (cat_clicked < 0 || cat_clicked >= num_cats) return;
	wxString s;
	s << _("Color for Category");
	wxString cat_label = template_canvas->cat_data.GetCategoryLabel(c_ts, cat_clicked);
	if (!cat_label.IsEmpty())
        s << ": " << cat_label;
    
	menu->Prepend(ID_CATEGORY_COLOR, s, s);
	opt_menu_cat = cat_clicked;
}

void TemplateLegend::OnCategoryColor(wxCommandEvent& event)
{
	int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
	if (opt_menu_cat < 0 || opt_menu_cat >= num_cats) return;
	
	wxColour col = template_canvas->cat_data.GetCategoryColor(c_ts, opt_menu_cat);
	wxColourData data;
	data.SetColour(col);
	data.SetChooseFull(true);
	int ki;
	for (ki = 0; ki < 16; ki++) {
		wxColour colour(ki * 16, ki * 16, ki * 16);
		data.SetCustomColour(ki, colour);
	}
	
	wxColourDialog dialog(this, &data);
	dialog.SetTitle(_("Choose Cateogry Color"));
	if (dialog.ShowModal() == wxID_OK) {
		wxColourData retData = dialog.GetColourData();
		for (int ts=0; ts<template_canvas->cat_data.GetCanvasTmSteps(); ts++) {
			if (num_cats == template_canvas->cat_data.GetNumCategories(ts)) {
				template_canvas->cat_data.SetCategoryColor(ts, opt_menu_cat,
														   retData.GetColour());
			}
		}
		template_canvas->invalidateBms();
		template_canvas->Refresh();
		Refresh();
	}
}

void TemplateLegend::OnDraw(wxDC& dc)
{
	if (!template_canvas)
        return;
    
    dc.SetFont(*GdaConst::small_font);
    dc.DrawText(template_canvas->GetCategoriesTitle(), px, 13);
	
	int time = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int cur_y = py;
	int numRect = template_canvas->cat_data.GetNumCategories(time);
	
    dc.SetPen(*wxBLACK_PEN);
    
    if (labels.size() != numRect) {
        for (int i=0; i<labels.size(); i++){
            delete labels[i];
        }
        labels.clear();
        
        int init_y = py;
        for (int i=0; i<numRect; i++) {
            wxString lbl = template_canvas->cat_data.GetCatLblWithCnt(time, i);
            wxPoint pt( px + m_l + 10, init_y - (m_w / 2));
            wxSize sz = dc.GetTextExtent(lbl);
            labels.push_back(new GdaLegendLabel(lbl, pt, sz));
            init_y += d_rect;
        }
    }
    
	for (int i=0; i<numRect; i++) {
        wxColour clr = template_canvas->cat_data.GetCategoryColor(time, i);
        if (clr.IsOk())
            dc.SetBrush(clr);
        else
            dc.SetBrush(*wxBLACK_BRUSH);
		dc.DrawRectangle(px, cur_y - 8, m_l, m_w);
		cur_y += d_rect;
        labels[i]->draw(dc);
	}
    
    if ( select_label ) {
        select_label->drawMove(dc);
    }
}

void TemplateLegend::RenderToDC(wxDC& dc, double scale)
{
	if (template_canvas == NULL)
        return;
    
    wxFont* fnt = wxFont::New(12 / scale, wxFONTFAMILY_SWISS,
                              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                              wxEmptyString, wxFONTENCODING_DEFAULT);
    dc.SetFont(*fnt);
  
	wxString ttl = template_canvas->GetCategoriesTitle();
	ttl = ttl.BeforeFirst(':');
    dc.DrawText(ttl, px / scale, 13 / scale);
	
	int time = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int cur_y = py;
	int numRect = template_canvas->cat_data.GetNumCategories(time);
	
    dc.SetPen(*wxBLACK_PEN);
	for (int i=0; i<numRect; i++) {
        wxColour clr = template_canvas->cat_data.GetCategoryColor(time, i);
        if (clr.IsOk())
            dc.SetBrush(clr);
        else
            dc.SetBrush(*wxBLACK_BRUSH);
        
		dc.DrawText(template_canvas->cat_data.GetCatLblWithCnt(time, i),
					(px + m_l + 10) / scale, (cur_y - (m_w / 2)) / scale);
		dc.DrawRectangle(px / scale, (cur_y - 8) / scale,
                         m_l / scale, m_w / scale);
		cur_y += d_rect;
	}
}


void TemplateLegend::SelectAllInCategory(int category,
										 bool add_to_selection)
{
	template_canvas->SelectAllInCategory(category, add_to_selection);
}

