//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// pages.cpp
//

#include "pages.h"
#include "legal.h"
#include "gsystem.h"
#include "gdebug.h"
#include <QtGui>
#include <stdexcept>

// ==

TitlePage::TitlePage( GDialog & dialog , const std::string & name , 
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	m_label = new QLabel( Legal::text() ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("E-MailRelay"))) ;
	layout->addWidget(m_label);
	setLayout(layout);
}

std::string TitlePage::nextPage()
{
	return next1() ;
}

// ==

LicensePage::LicensePage( GDialog & dialog , const std::string & name , 
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	m_text_edit = new QTextEdit;
	m_text_edit->setReadOnly(true) ;
	m_text_edit->setWordWrapMode(QTextOption::NoWrap) ;
	m_text_edit->setLineWrapMode(QTextEdit::NoWrap) ;
	m_text_edit->setFontFamily("courier") ;
	m_text_edit->setPlainText(Legal::license()) ;

	m_agree_check_box = new QCheckBox(tr("I agree to the terms and conditions of the license"));
	setFocusProxy( m_agree_check_box ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("License"))) ;
	layout->addWidget(m_text_edit);
	layout->addWidget(m_agree_check_box);
	setLayout(layout);

	connect( m_agree_check_box , SIGNAL(toggled(bool)) , this , SIGNAL(onUpdate()) ) ;
}

std::string LicensePage::nextPage()
{
	return next1() ;
}

bool LicensePage::isComplete()
{
	return m_agree_check_box->isChecked() ;
}

// ==

DirectoryPage::DirectoryPage( GDialog & dialog , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	m_install_dir_label = new QLabel(tr("&Directory:")) ;
	m_install_dir_edit_box = new QLineEdit ;
	m_install_dir_label->setBuddy(m_install_dir_edit_box) ;
	m_install_dir_browse_button = new QPushButton(tr("B&rowse")) ;

	QHBoxLayout * install_layout = new QHBoxLayout ;
	install_layout->addWidget( m_install_dir_label ) ;
	install_layout->addWidget( m_install_dir_edit_box ) ;
	install_layout->addWidget( m_install_dir_browse_button ) ;

	QGroupBox * install_box = new QGroupBox(tr("Installation directory")) ;
	install_box->setLayout( install_layout ) ;

	m_spool_dir_label = new QLabel(tr("D&irectory:")) ;
	m_spool_dir_edit_box = new QLineEdit ;
	m_spool_dir_label->setBuddy(m_spool_dir_edit_box) ;
	m_spool_dir_browse_button = new QPushButton(tr("B&rowse")) ;

	QHBoxLayout * spool_layout = new QHBoxLayout ;
	spool_layout->addWidget( m_spool_dir_label ) ;
	spool_layout->addWidget( m_spool_dir_edit_box ) ;
	spool_layout->addWidget( m_spool_dir_browse_button ) ;

	QGroupBox * spool_box = new QGroupBox(tr("Spool directory")) ;
	spool_box->setLayout( spool_layout ) ;

	setFocusProxy( m_install_dir_edit_box ) ;

	m_install_dir_edit_box->setText( QString(GSystem::install().str().c_str()) ) ;
	m_spool_dir_edit_box->setText( QString(GSystem::spool().str().c_str()) ) ;

	QVBoxLayout * layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Directories"))) ;
	layout->addWidget( install_box ) ;
	layout->addWidget( spool_box ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_install_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseInstall()) ) ;
	connect( m_spool_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseSpool()) ) ;
	connect( m_install_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()));
	connect( m_spool_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()));
}

void DirectoryPage::browseInstall()
{
	G_DEBUG( "DirectoryPage::browseInstall" ) ;
	QString s = browse(m_install_dir_edit_box->text()) ;
	if( ! s.isEmpty() )
		m_install_dir_edit_box->setText( s ) ;
}

void DirectoryPage::browseSpool()
{
	G_DEBUG( "DirectoryPage::browseSpool" ) ;
	
	QString s = browse(m_spool_dir_edit_box->text()) ;
	if( ! s.isEmpty() )
		m_spool_dir_edit_box->setText( s ) ;
}

QString DirectoryPage::browse( QString dir )
{
	return QFileDialog::getExistingDirectory( this , QString() , dir ) ;
}

void DirectoryPage::reset()
{
	m_install_dir_edit_box->clear();
	m_spool_dir_edit_box->clear();
}

std::string DirectoryPage::nextPage()
{
	return next1() ;
}

bool DirectoryPage::isComplete()
{
	return 
		!m_install_dir_edit_box->text().isEmpty() &&
		!m_spool_dir_edit_box->text().isEmpty() ;
}

// ==

DoWhatPage::DoWhatPage( GDialog & dialog, const std::string & name , 
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	m_pop_check_box = new QCheckBox(tr("&POP3 server"));
	m_smtp_check_box = new QCheckBox(tr("&SMTP server"));
	m_smtp_check_box->setChecked(true) ;

	QVBoxLayout * server_type_box_layout = new QVBoxLayout ;
	server_type_box_layout->addWidget( m_pop_check_box ) ;
	server_type_box_layout->addWidget( m_smtp_check_box ) ;

	QGroupBox * server_type_box = new QGroupBox(tr("Server")) ;
	server_type_box->setLayout( server_type_box_layout ) ;

	m_immediate_check_box = new QRadioButton(tr("&After a message is received"));
	m_periodically_check_box = new QRadioButton(tr("&Check periodically"));
	m_on_demand_check_box = new QRadioButton(tr("&Only when triggered"));
	m_immediate_check_box->setChecked(true) ;

	QLabel * period_label = new QLabel( tr("e&very") ) ;
	m_period_combo_box = new QComboBox ;
	m_period_combo_box->addItem( tr("second") ) ;
	m_period_combo_box->addItem( tr("minute") ) ;
	m_period_combo_box->addItem( tr("hour") ) ;
	m_period_combo_box->setCurrentIndex( 1 ) ;
	m_period_combo_box->setEditable( false ) ;
	period_label->setBuddy( m_period_combo_box ) ;

	QVBoxLayout * forwarding_box_layout = new QVBoxLayout ;
	forwarding_box_layout->addWidget( m_immediate_check_box ) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( m_periodically_check_box ) ;
		inner->addWidget( period_label ) ;
		inner->addWidget( m_period_combo_box ) ;
		forwarding_box_layout->addLayout( inner ) ;
	}
	forwarding_box_layout->addWidget( m_on_demand_check_box ) ;

	m_forwarding_box = new QGroupBox(tr("SMTP forwarding")) ;
	m_forwarding_box->setLayout( forwarding_box_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Installation type"))) ;
	layout->addWidget( server_type_box ) ;
	layout->addWidget( m_forwarding_box ) ;
	layout->addStretch() ;
	setLayout(layout);

	connect( m_pop_check_box , SIGNAL(toggled(bool)) , this , SIGNAL(onUpdate()) ) ;
	connect( m_smtp_check_box , SIGNAL(toggled(bool)) , this , SIGNAL(onUpdate()) ) ;
	connect( m_periodically_check_box , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_smtp_check_box , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;

	onToggle() ;
}

void DoWhatPage::onToggle()
{
	m_period_combo_box->setEnabled( m_smtp_check_box->isChecked() && m_periodically_check_box->isChecked() ) ;
	m_forwarding_box->setEnabled( m_smtp_check_box->isChecked() ) ;
}

std::string DoWhatPage::nextPage()
{
	// sneaky feature...
	if( dialog().currentPageName() != name() )
	{
		return m_smtp_check_box->isChecked() ? next2() : std::string() ;
	}

	return 
		m_pop_check_box->isChecked() ?
			next1() :
			next2() ;
}

bool DoWhatPage::isComplete()
{
	return 
		m_pop_check_box->isChecked() ||
		m_smtp_check_box->isChecked() ;
}

// ==

PopPage::PopPage( GDialog & dialog , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	QLabel * port_label = new QLabel( tr("P&ort") ) ;
	m_port_edit_box = new QLineEdit( tr("110") ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	QHBoxLayout * server_layout = new QHBoxLayout ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;

	QGroupBox * server_box = new QGroupBox(tr("Local server")) ;
	server_box->setLayout( server_layout ) ;

	m_one = new QRadioButton( tr("&One client") ) ;
	m_shared = new QRadioButton( tr("&Many clients sharing a spool directory") ) ;
	m_pop_by_name = new QRadioButton( tr("M&any clients with separate spool directories") ) ;
	m_one->setChecked(true) ;

	m_no_delete_check_box = new QCheckBox( tr("Disable message deletion") ) ;
	m_no_delete_check_box->setChecked(true) ;

	m_auto_copy_check_box = new QCheckBox( tr("Copy SMTP messages to all") ) ;
	m_auto_copy_check_box->setChecked(false) ;

	QGridLayout * radio_layout = new QGridLayout ;
	radio_layout->addWidget( m_one , 0 , 0 ) ;
	radio_layout->addWidget( m_shared , 1 , 0 ) ;
	radio_layout->addWidget( m_no_delete_check_box , 1 , 1 ) ;
	radio_layout->addWidget( m_pop_by_name , 2 , 0 ) ;
	radio_layout->addWidget( m_auto_copy_check_box , 2 , 1 ) ;

	QGroupBox * box = new QGroupBox(tr("Client accounts")) ;
	box->setLayout( radio_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("POP server"))) ;
	layout->addWidget( server_box ) ;
	layout->addWidget( box ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_one , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_shared , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_pop_by_name , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;

	onToggle() ;
}

std::string PopPage::nextPage()
{
	return 
		m_one->isChecked() ?
			next1() :
			next2() ;
}

bool PopPage::isComplete()
{
	return ! m_port_edit_box->text().isEmpty() ;
}

void PopPage::onToggle()
{
	m_no_delete_check_box->setEnabled( m_shared->isChecked() ) ;
	m_auto_copy_check_box->setEnabled( m_pop_by_name->isChecked() ) ;
}

// ==

PopAccountsPage::PopAccountsPage( GDialog & dialog , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("APOP") ) ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 0 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QGridLayout * account_layout = new QGridLayout ;
	QLabel * name_label = new QLabel( tr("Name") ) ;
	QLabel * pwd_label = new QLabel( tr("Password") ) ;
	m_name_1 = new QLineEdit ;
	m_pwd_1 = new QLineEdit ;
	m_pwd_1->setEchoMode( QLineEdit::Password ) ;
	m_name_2 = new QLineEdit ;
	m_pwd_2 = new QLineEdit ;
	m_pwd_2->setEchoMode( QLineEdit::Password ) ;
	m_name_3 = new QLineEdit ;
	m_pwd_3 = new QLineEdit ;
	m_pwd_3->setEchoMode( QLineEdit::Password ) ;
	account_layout->addWidget( name_label , 0 , 0 ) ;
	account_layout->addWidget( pwd_label , 0 , 1 ) ;
	account_layout->addWidget( m_name_1 , 1 , 0 ) ;
	account_layout->addWidget( m_pwd_1 , 1 , 1 ) ;
	account_layout->addWidget( m_name_2 , 2 , 0 ) ;
	account_layout->addWidget( m_pwd_2 , 2 , 1 ) ;
	account_layout->addWidget( m_name_3 , 3 , 0 ) ;
	account_layout->addWidget( m_pwd_3 , 3 , 1 ) ;

	QGroupBox * account_box = new QGroupBox(tr("Accounts")) ;
	account_box->setLayout( account_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("POP accounts"))) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( mechanism_label ) ;
		inner->addWidget( m_mechanism_combo ) ;
		layout->addLayout( inner ) ;
	}
	layout->addWidget( account_box ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_name_1 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_pwd_1 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_name_2 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_pwd_2 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_name_3 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_pwd_3 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
}

std::string PopAccountsPage::nextPage()
{
	// only the dowhat page knows whether we should do smtp -- a special	
	// feature of the dowhat page's nextPage() is that if it detects
	// that it is not the current page then it will give us an empty string 
	// if no smtp is required

	return
		dialog().previousPage(2U).nextPage().empty() ?
			next2() :
			next1() ;
}

bool PopAccountsPage::isComplete()
{
	return
		( !m_name_1->text().isEmpty() && !m_pwd_1->text().isEmpty() ) ||
		( !m_name_2->text().isEmpty() && !m_pwd_2->text().isEmpty() ) ||
		( !m_name_3->text().isEmpty() && !m_pwd_3->text().isEmpty() ) ;
}

// ==

PopAccountPage::PopAccountPage( GDialog & dialog , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("APOP") ) ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 0 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QGridLayout * account_layout = new QGridLayout ;
	QLabel * name_label = new QLabel( tr("Name") ) ;
	QLabel * pwd_label = new QLabel( tr("Password") ) ;
	m_name_1 = new QLineEdit ;
	m_pwd_1 = new QLineEdit ;
	m_pwd_1->setEchoMode( QLineEdit::Password ) ;
	account_layout->addWidget( name_label , 0 , 0 ) ;
	account_layout->addWidget( m_name_1 , 0 , 1 ) ;
	account_layout->addWidget( pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_pwd_1 , 1 , 1 ) ;

	QGroupBox * account_box = new QGroupBox(tr("Account")) ;
	account_box->setLayout( account_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("POP account"))) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( mechanism_label ) ;
		inner->addWidget( m_mechanism_combo ) ;
		layout->addLayout( inner ) ;
	}
	layout->addWidget( account_box ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_name_1 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_pwd_1 , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
}

std::string PopAccountPage::nextPage()
{
	// (see above)
	return
		dialog().previousPage(2U).nextPage().empty() ?
			next2() :
			next1() ;
}

bool PopAccountPage::isComplete()
{
	return
		!m_name_1->text().isEmpty() && !m_pwd_1->text().isEmpty() ;
}

// ==

SmtpServerPage::SmtpServerPage( GDialog & dialog , const std::string & name , 
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	QLabel * port_label = new QLabel( tr("P&ort") ) ;
	m_port_edit_box = new QLineEdit( tr("25") ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	QHBoxLayout * server_layout = new QHBoxLayout ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;

	QGroupBox * server_box = new QGroupBox(tr("Local server")) ;
	server_box->setLayout( server_layout ) ;

	m_auth_check_box = new QCheckBox( tr("&Require authentication") ) ;

	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 0 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QLabel * account_name_label = new QLabel( tr("&Name") ) ;
	m_account_name = new QLineEdit ;
	account_name_label->setBuddy( m_account_name ) ;

	QLabel * account_pwd_label = new QLabel( tr("&Password") ) ;
	m_account_pwd = new QLineEdit ;
	m_account_pwd->setEchoMode( QLineEdit::Password ) ;
	account_pwd_label->setBuddy( m_account_pwd ) ;

	QGridLayout * account_layout = new QGridLayout ;
	account_layout->addWidget( account_name_label , 0 , 0 ) ;
	account_layout->addWidget( m_account_name , 0 , 1 ) ;
	account_layout->addWidget( account_pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_account_pwd , 1 , 1 ) ;

	m_account_box = new QGroupBox(tr("Account")) ;
	m_account_box->setLayout( account_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("SMTP server"))) ;
	layout->addWidget( server_box ) ;
	layout->addWidget( m_auth_check_box ) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( mechanism_label ) ;
		inner->addWidget( m_mechanism_combo ) ;
		layout->addLayout( inner ) ;
	}
	layout->addWidget( m_account_box ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_account_name , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_account_pwd , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_auth_check_box , SIGNAL(toggled(bool)), this, SIGNAL(onUpdate()) ) ;
	connect( m_auth_check_box , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;

	onToggle() ;
}

std::string SmtpServerPage::nextPage()
{
	return next1() ;
}

void SmtpServerPage::onToggle()
{
	m_account_box->setEnabled( m_auth_check_box->isChecked() ) ;
	m_mechanism_combo->setEnabled( m_auth_check_box->isChecked() ) ;
}

bool SmtpServerPage::isComplete()
{
	return
		! m_port_edit_box->text().isEmpty() &&
		( ! m_auth_check_box->isChecked() || (
			! m_account_name->text().isEmpty() &&
			! m_account_pwd->text().isEmpty() ) ) ;
}

// ==

SmtpClientPage::SmtpClientPage( GDialog & dialog , const std::string & name , 
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	QLabel * server_label = new QLabel( tr("&Hostname") ) ;
	m_server_edit_box = new QLineEdit ;
	server_label->setBuddy( m_server_edit_box ) ;

	QLabel * port_label = new QLabel( tr("P&ort") ) ;
	m_port_edit_box = new QLineEdit( tr("25") ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	QHBoxLayout * server_layout = new QHBoxLayout ;
	server_layout->addWidget( server_label ) ;
	server_layout->addWidget( m_server_edit_box ) ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;
	server_layout->setStretchFactor( m_server_edit_box , 4 ) ;

	QGroupBox * server_box = new QGroupBox(tr("Remote server")) ;
	server_box->setLayout( server_layout ) ;

	m_auth_check_box = new QCheckBox( tr("&Supply authentication") ) ;

	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 0 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QLabel * account_name_label = new QLabel( tr("&Name") ) ;
	m_account_name = new QLineEdit ;
	account_name_label->setBuddy( m_account_name ) ;

	QLabel * account_pwd_label = new QLabel( tr("&Password") ) ;
	m_account_pwd = new QLineEdit ;
	m_account_pwd->setEchoMode( QLineEdit::Password ) ;
	account_pwd_label->setBuddy( m_account_pwd ) ;

	QGridLayout * account_layout = new QGridLayout ;
	account_layout->addWidget( account_name_label , 0 , 0 ) ;
	account_layout->addWidget( m_account_name , 0 , 1 ) ;
	account_layout->addWidget( account_pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_account_pwd , 1 , 1 ) ;

	m_account_box = new QGroupBox(tr("Account")) ;
	m_account_box->setLayout( account_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("SMTP client"))) ;
	layout->addWidget( server_box ) ;
	layout->addWidget( m_auth_check_box ) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( mechanism_label ) ;
		inner->addWidget( m_mechanism_combo ) ;
		layout->addLayout( inner ) ;
	}
	layout->addWidget( m_account_box ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_server_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_account_name , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_account_pwd , SIGNAL(textChanged(QString)), this, SIGNAL(onUpdate()) ) ;
	connect( m_auth_check_box , SIGNAL(toggled(bool)), this, SIGNAL(onUpdate()) ) ;
	connect( m_auth_check_box , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;

	onToggle() ;
}

void SmtpClientPage::onToggle()
{
	m_account_box->setEnabled( m_auth_check_box->isChecked() ) ;
	m_mechanism_combo->setEnabled( m_auth_check_box->isChecked() ) ;
}

std::string SmtpClientPage::nextPage()
{
	return next1() ;
}

bool SmtpClientPage::isComplete()
{
	return
		! m_port_edit_box->text().isEmpty() &&
		! m_server_edit_box->text().isEmpty() &&
		( ! m_auth_check_box->isChecked() || (
			! m_account_name->text().isEmpty() &&
			! m_account_pwd->text().isEmpty() ) ) ;
}

// ==

StartupPage::StartupPage( GDialog & dialog , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	m_on_boot_check_box = new QCheckBox( tr("At &system startup") ) ;
	m_at_login_check_box = new QCheckBox( tr("&When logging in") ) ;
	QVBoxLayout * auto_layout = new QVBoxLayout ;
	auto_layout->addWidget( m_on_boot_check_box ) ;
	auto_layout->addWidget( m_at_login_check_box ) ;

	m_add_menu_item_check_box = new QCheckBox( tr("Add to start menu") ) ;
	m_add_desktop_item_check_box = new QCheckBox( tr("Add to desktop") ) ;
	QVBoxLayout * manual_layout = new QVBoxLayout ;
	manual_layout->addWidget( m_add_menu_item_check_box ) ;
	manual_layout->addWidget( m_add_desktop_item_check_box ) ;
	m_add_menu_item_check_box->setChecked(true) ;

	m_verbose_check_box = new QCheckBox( tr("&Verbose") ) ;
	QVBoxLayout * logging_layout = new QVBoxLayout ;
	logging_layout->addWidget( m_verbose_check_box ) ;

	QGroupBox * auto_box = new QGroupBox(tr("Automatic")) ;
	auto_box->setLayout( auto_layout ) ;

	QGroupBox * manual_box = new QGroupBox(tr("Manual")) ;
	manual_box->setLayout( manual_layout ) ;

	QGroupBox * logging_box = new QGroupBox(tr("Logging")) ;
	logging_box->setLayout( logging_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Server startup"))) ;
	layout->addWidget(auto_box) ;
	layout->addWidget(manual_box) ;
	layout->addWidget(logging_box) ;
	layout->addStretch() ;
	setLayout( layout ) ;
}

std::string StartupPage::nextPage()
{
	return next1() ;
}

// ==

FinalPage::FinalPage( GDialog & dialog , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) : 
		GPage(dialog,name,next_1,next_2)
{
	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Finish"))) ;
	layout->addStretch() ;
	setLayout( layout ) ;
}

std::string FinalPage::nextPage()
{
	return std::string() ;
}

