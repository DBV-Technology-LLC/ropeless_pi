///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-45-gfbbc9177)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif

#include "transponderReleaseDlg.h"

///////////////////////////////////////////////////////////////////////////

transponderReleaseDlg::transponderReleaseDlg( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Transponder # :"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetFont( wxFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer2->Add( m_staticText2, 0, wxALL, 10 );

	m_staticTextID = new wxStaticText( this, wxID_ANY, _("ID"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextID->Wrap( -1 );
	m_staticTextID->SetFont( wxFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer2->Add( m_staticTextID, 0, wxALL, 10 );


	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bitmap1 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmap1->Hide();

	bSizer2->Add( m_bitmap1, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Release Status:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_staticText1, 0, wxALL, 10 );

	m_staticTextStatus = new wxStaticText( this, wxID_ANY, _("Connecting"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextStatus->Wrap( -1 );
	m_staticTextStatus->SetFont( wxFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_staticTextStatus->SetMinSize( wxSize( 220,-1 ) );

	bSizer3->Add( m_staticTextStatus, 0, wxALL, 10 );


	bSizer1->Add( bSizer3, 1, wxEXPAND, 10 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_buttonRecovered = new wxButton( this, wxID_ANY, _("Mark Recovered"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRecovered->SetFont( wxFont( 18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer9->Add( m_buttonRecovered, 0, wxALIGN_BOTTOM|wxALL, 10 );


	bSizer9->Add( 0, 0, 1, wxEXPAND, 10 );

	m_buttonRetry = new wxButton( this, wxID_ANY, _("Retry"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRetry->SetFont( wxFont( 18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer9->Add( m_buttonRetry, 0, wxALIGN_BOTTOM|wxALL, 10 );


	bSizer9->Add( 0, 0, 1, wxEXPAND, 10 );

	m_buttonOk = new wxButton( this, wxID_ANY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOk->SetFont( wxFont( 18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer9->Add( m_buttonOk, 0, wxALIGN_BOTTOM|wxALL, 10 );


	bSizer1->Add( bSizer9, 1, wxEXPAND, 0 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( transponderReleaseDlg::OnClose ) );
	m_buttonRecovered->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( transponderReleaseDlg::markRecoveredClick ), NULL, this );
	m_buttonRetry->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( transponderReleaseDlg::retryClick ), NULL, this );
	m_buttonOk->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( transponderReleaseDlg::okClick ), NULL, this );
}

transponderReleaseDlg::~transponderReleaseDlg()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( transponderReleaseDlg::OnClose ) );
	m_buttonRecovered->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( transponderReleaseDlg::markRecoveredClick ), NULL, this );
	m_buttonRetry->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( transponderReleaseDlg::retryClick ), NULL, this );
	m_buttonOk->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( transponderReleaseDlg::okClick ), NULL, this );

}
