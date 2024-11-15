/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Ropeless Plugin
 * Author:   Colin Vincent
 *
 ******************************************************************************
 * This file is part of the Ropeless plugin
 * (https://github.com/bdbcat/ropeless_pi).
 *   Copyright (C) 2024 by Ropeless Systems, Inc. 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3, or (at your option) any later
 * version of the license.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif

#include "transponderReleaseDlgImpl.h"

transponderReleaseDlgImpl::transponderReleaseDlgImpl(wxWindow* parent, int id, 
	const wxString& title, const wxPoint& pos, const wxSize& size, long style) : transponderReleaseDlg(parent, id, title, pos, size, style)
{

	wxLogMessage("Creating transponderReleaseDlg!");

}

void transponderReleaseDlgImpl::updateID(int id)
{	
	wxLogMessage("Setting Release ID String!");
	wxString idStr;
	idStr.Printf("%d",id);
	m_staticText21->SetLabel(idStr);
}