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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class transponderReleaseDlg
///////////////////////////////////////////////////////////////////////////////
class transponderReleaseDlg : public wxDialog
{
	private:

	protected:
		wxStaticText* m_staticText2;
		wxStaticText* m_staticTextID;
		wxStaticBitmap* m_bitmap1;
		wxStaticText* m_staticText1;
		wxStaticText* m_staticTextStatus;
		wxButton* m_buttonRecovered;
		wxButton* m_buttonRetry;
		wxButton* m_buttonOk;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void markRecoveredClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void retryClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void okClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		transponderReleaseDlg( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Transponder Release"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );

		~transponderReleaseDlg();

};

