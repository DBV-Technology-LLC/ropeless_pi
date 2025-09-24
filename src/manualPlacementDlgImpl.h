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

#ifndef _MANUALPLACEMENTIMPL_H_
#define _MANUALPLACEMENTIMPL_H_

#include "manualPlacementDlg.h"
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>

// Forward declaration
struct transponder_state;

/// Implementation of the GUI functionality for Preferences dialog.
/// To obtain \c MainConfigFrame information use \c wxFormBuilder to open \c
/// dashboardsk.fbp
class manualPlacementDlgImpl : public manualPlacementDlg {
    private:

        void OnChar(wxKeyEvent& event);
        void AddPositionSourceControl();
        void OnPositionSourceChanged(wxCommandEvent& event);
        void AddDeviceTypeControl();
        void OnDeviceTypeChanged(wxCommandEvent& event);
        void AddTrawlSelectionControl();
        void OnTrawlSelectionChanged(wxCommandEvent& event);
        void UpdateTrawlPosControl();
        void PopulateTrawlDropdown();
        void ReorganizeLayout();
        uint8_t GetMfgCodeFromSelection(int selection);

    public:

        manualPlacementDlgImpl( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Place Transponder"), const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE, const wxString& latStr = _(""),
            const wxString& lonStr = _(""), const wxString& utcStr = _(""), transponder_state* existingState = nullptr );
        ~manualPlacementDlgImpl() = default;

        bool isOwned;
        int markID;
        int pairId;             // Keep for backward compatibility
        int selectedTrawlId;    // Selected trawl ID from dropdown
        int trawlPosition;      // Trawl position value
        bool valid;
        int positionSource;
        int deviceType;
        
        // Position source dropdown control
        wxChoice* m_choicePositionSource;
        
        // Device type dropdown control
        wxChoice* m_choiceDeviceType;
        
        // Trawl selection dropdown control
        wxChoice* m_choiceTrawlId;

        // Trawl position spin control
        wxSpinCtrl* m_spinCtrlTrawlPos;
        
    protected:

        virtual void cancelPlaceTransponder( wxCommandEvent& event );
        virtual void okPlaceTransponder( wxCommandEvent& event );
        virtual void idOnChar( wxKeyEvent& event );
        virtual void pairOnChar( wxKeyEvent& event );
        virtual void ownedChecked( wxCommandEvent& event );
};

#endif // _MANUALPLACEMENTIMPL_H_
