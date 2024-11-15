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

#ifndef _TRANSPONDERRELEASEIMPL_H
#define _TRANSPONDERRELEASEIMPL_H

#include "transponderReleaseDlg.h"

class transponderReleaseDlgImpl : public transponderReleaseDlg {
    private:

    public:

        transponderReleaseDlgImpl( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Place Transponder"), const wxPoint& pos = wxDefaultPosition, 
            const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE);

        ~transponderReleaseDlgImpl() = default;
        
        void updateID(int id);
        
    protected:

};

#endif // _TRANSPONDERRELEASEIMPL_H
