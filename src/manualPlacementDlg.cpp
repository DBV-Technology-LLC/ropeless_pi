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

#include "manualPlacementDlg.h"

///////////////////////////////////////////////////////////////////////////

manualPlacementDlg::manualPlacementDlg( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Lat: "), wxDefaultPosition, wxSize( 30,-1 ), 0 );
	m_staticText2->Wrap( -1 );
	bSizer2->Add( m_staticText2, 0, wxALL, 5 );

	m_staticText7 = new wxStaticText( this, wxID_ANY, _("41°33'52.1\"N"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bSizer2->Add( m_staticText7, 0, wxALL, 5 );


	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Lon:"), wxDefaultPosition, wxSize( 30,-1 ), 0 );
	m_staticText21->Wrap( -1 );
	bSizer2->Add( m_staticText21, 0, wxALL, 5 );

	m_staticText8 = new wxStaticText( this, wxID_ANY, _("71°25'26.4\"W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bSizer2->Add( m_staticText8, 0, wxALL, 5 );


	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("ID: "), wxDefaultPosition, wxSize( 30,-1 ), 0 );
	m_staticText1->Wrap( -1 );
	bSizer3->Add( m_staticText1, 0, wxALL, 5 );

	m_textCtrl12 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), 0 );
	bSizer3->Add( m_textCtrl12, 0, wxALL, 5 );

	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Pair:"), wxDefaultPosition, wxSize( 30,-1 ), 0 );
	m_staticText11->Wrap( -1 );
	bSizer3->Add( m_staticText11, 0, wxALL, 5 );

	m_textCtrl121 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 75,-1 ), 0 );
	bSizer3->Add( m_textCtrl121, 0, wxALL, 5 );

	m_checkBox1 = new wxCheckBox( this, wxID_ANY, _("Owned?"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox1->SetValue(true);
	bSizer3->Add( m_checkBox1, 0, wxALL, 5 );


	bSizer1->Add( bSizer3, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Time: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	bSizer6->Add( m_staticText10, 0, wxALL, 5 );

	m_staticText111 = new wxStaticText( this, wxID_ANY, _("YYYY-MM-DDThh:mm:ssZ"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText111->Wrap( -1 );
	bSizer6->Add( m_staticText111, 0, wxALL, 5 );


	bSizer1->Add( bSizer6, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );


	bSizer1->Add( bSizer9, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizer1->Add( m_sdbSizer1, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_textCtrl12->Connect( wxEVT_CHAR, wxKeyEventHandler( manualPlacementDlg::idOnChar ), NULL, this );
	m_textCtrl121->Connect( wxEVT_CHAR, wxKeyEventHandler( manualPlacementDlg::pairOnChar ), NULL, this );
	m_checkBox1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( manualPlacementDlg::ownedChecked ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( manualPlacementDlg::cancelPlaceTransponder ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( manualPlacementDlg::okPlaceTransponder ), NULL, this );
}

manualPlacementDlg::~manualPlacementDlg()
{
	// Disconnect Events
	m_textCtrl12->Disconnect( wxEVT_CHAR, wxKeyEventHandler( manualPlacementDlg::idOnChar ), NULL, this );
	m_textCtrl121->Disconnect( wxEVT_CHAR, wxKeyEventHandler( manualPlacementDlg::pairOnChar ), NULL, this );
	m_checkBox1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( manualPlacementDlg::ownedChecked ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( manualPlacementDlg::cancelPlaceTransponder ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( manualPlacementDlg::okPlaceTransponder ), NULL, this );

}
