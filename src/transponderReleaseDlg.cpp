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

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("ID"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticText21->Wrap( -1 );
	m_staticText21->SetFont( wxFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer2->Add( m_staticText21, 0, wxALL, 10 );


	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Release Status:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_staticText1, 0, wxALL, 10 );

	m_staticText11 = new wxStaticText( this, wxID_ANY, _("VALIDATED"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticText11->Wrap( -1 );
	m_staticText11->SetFont( wxFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_staticText11, 0, wxALL, 10 );


	bSizer1->Add( bSizer3, 1, wxEXPAND, 10 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_button2 = new wxButton( this, wxID_ANY, _("Mark Recovered"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button2->SetFont( wxFont( 18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer9->Add( m_button2, 0, wxALIGN_BOTTOM|wxALL, 10 );


	bSizer9->Add( 0, 0, 1, wxEXPAND, 10 );

	m_button1 = new wxButton( this, wxID_ANY, _("Retry"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button1->SetFont( wxFont( 18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer9->Add( m_button1, 0, wxALIGN_BOTTOM|wxALL, 10 );


	bSizer9->Add( 0, 0, 1, wxEXPAND, 10 );

	m_button11 = new wxButton( this, wxID_ANY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button11->SetFont( wxFont( 18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer9->Add( m_button11, 0, wxALIGN_BOTTOM|wxALL, 10 );


	bSizer1->Add( bSizer9, 1, wxEXPAND, 0 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	this->Centre( wxBOTH );
}

transponderReleaseDlg::~transponderReleaseDlg()
{
}
