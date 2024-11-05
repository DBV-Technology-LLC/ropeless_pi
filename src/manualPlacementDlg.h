///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-45-gfbbc9177)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class manualPlacementDlg
///////////////////////////////////////////////////////////////////////////////
class manualPlacementDlg : public wxDialog
{
	private:

	protected:
		wxStaticText* m_staticText2;
		wxStaticText* m_staticText7;
		wxStaticText* m_staticText21;
		wxStaticText* m_staticText8;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_textCtrl12;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_textCtrl121;
		wxCheckBox* m_checkBox1;
		wxStaticText* m_staticText10;
		wxStaticText* m_staticText111;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void idOnChar( wxKeyEvent& event ) { event.Skip(); }
		virtual void pairOnChar( wxKeyEvent& event ) { event.Skip(); }
		virtual void ownedChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void cancelPlaceTransponder( wxCommandEvent& event ) { event.Skip(); }
		virtual void okPlaceTransponder( wxCommandEvent& event ) { event.Skip(); }


	public:

		manualPlacementDlg( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Place Transponder"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );

		~manualPlacementDlg();

};

