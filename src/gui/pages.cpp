//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
//
// pages.cpp
//

#include "gdef.h"
#include "qt.h"
#include "pages.h"
#include "legal.h"
#include "dir.h"
#include "boot.h"
#include "gmapfile.h"
#include "gstr.h"
#include "gfile.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gmd5.h"
#include "gxtext.h"
#include "gdebug.h"
#include <stdexcept>
#include <fstream>

namespace
{
	std::string encrypt( const std::string & pwd , const std::string & mechanism )
	{
		return mechanism == "CRAM-MD5" ? G::Md5::mask(pwd) : G::Xtext::encode(pwd) ;
	}
}

// ==

TitlePage::TitlePage( GDialog & dialog , const G::MapFile & , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) :
		GPage(dialog,name,next_1,next_2,finish,close)
{
	m_label = new QLabel( Legal::text() ) ;
	m_credit = new QLabel( Legal::credit() ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("E-MailRelay"))) ;
	layout->addWidget(m_label);
	layout->addWidget(m_credit);
	setLayout(layout);
}

std::string TitlePage::nextPage()
{
	return next1() ;
}

void TitlePage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
}

// ==

LicensePage::LicensePage( GDialog & dialog , const G::MapFile & , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool accepted ) :
		GPage(dialog,name,next_1,next_2,finish,close)
{
	m_text_edit = new QTextEdit;
	m_text_edit->setReadOnly(true) ;
	m_text_edit->setWordWrapMode(QTextOption::NoWrap) ;
	m_text_edit->setLineWrapMode(QTextEdit::NoWrap) ;
	m_text_edit->setFontFamily("courier") ;
	m_text_edit->setPlainText(Legal::license()) ;

	m_agree_checkbox = new QCheckBox(tr("I agree to the terms and conditions of the license"));
	setFocusProxy( m_agree_checkbox ) ;

	if( testMode() || accepted )
		m_agree_checkbox->setChecked(true) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("License"))) ;
	layout->addWidget(m_text_edit);
	layout->addWidget(m_agree_checkbox);
	setLayout(layout);

	connect( m_agree_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
}

std::string LicensePage::nextPage()
{
	return next1() ;
}

void LicensePage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
}

bool LicensePage::isComplete()
{
	return m_agree_checkbox->isChecked() ;
}

// ==

DirectoryPage::DirectoryPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
	bool installing ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_installing(installing)
{
	m_install_dir_label = new QLabel(tr("&Directory:")) ;
	m_install_dir_edit_box = new QLineEdit ;
	m_install_dir_label->setBuddy(m_install_dir_edit_box) ;
	m_install_dir_browse_button = new QPushButton(tr("B&rowse")) ;

	QHBoxLayout * install_layout = new QHBoxLayout ;
	install_layout->addWidget( m_install_dir_label ) ;
	install_layout->addWidget( m_install_dir_edit_box ) ;
	install_layout->addWidget( m_install_dir_browse_button ) ;

	QGroupBox * install_group = new QGroupBox(tr("Installation directory")) ;
	install_group->setLayout( install_layout ) ;

	//

	m_spool_dir_label = new QLabel(tr("D&irectory:")) ;
	m_spool_dir_edit_box = new QLineEdit ;
	tip( m_spool_dir_edit_box , "--spool-dir" ) ;
	m_spool_dir_label->setBuddy(m_spool_dir_edit_box) ;
	m_spool_dir_browse_button = new QPushButton(tr("B&rowse")) ;

	QHBoxLayout * spool_layout = new QHBoxLayout ;
	spool_layout->addWidget( m_spool_dir_label ) ;
	spool_layout->addWidget( m_spool_dir_edit_box ) ;
	spool_layout->addWidget( m_spool_dir_browse_button ) ;

	QGroupBox * spool_group = new QGroupBox(tr("Spool directory")) ;
	spool_group->setLayout( spool_layout ) ;

	//

	m_config_dir_label = new QLabel(tr("Dir&ectory:")) ;
	m_config_dir_edit_box = new QLineEdit ;
	m_config_dir_label->setBuddy(m_config_dir_edit_box) ;
	m_config_dir_browse_button = new QPushButton(tr("B&rowse")) ;

	QHBoxLayout * config_layout = new QHBoxLayout ;
	config_layout->addWidget( m_config_dir_label ) ;
	config_layout->addWidget( m_config_dir_edit_box ) ;
	config_layout->addWidget( m_config_dir_browse_button ) ;

	QGroupBox * config_group = new QGroupBox(tr("Configuration directory")) ;
	config_group->setLayout( config_layout ) ;

	//

	m_runtime_dir_label = new QLabel(tr("Dire&ctory:")) ;
	m_runtime_dir_edit_box = new QLineEdit ;
	m_runtime_dir_label->setBuddy(m_runtime_dir_edit_box) ;
	m_runtime_dir_browse_button = new QPushButton(tr("B&rowse")) ;

	QHBoxLayout * runtime_layout = new QHBoxLayout ;
	runtime_layout->addWidget( m_runtime_dir_label ) ;
	runtime_layout->addWidget( m_runtime_dir_edit_box ) ;
	runtime_layout->addWidget( m_runtime_dir_browse_button ) ;

	QGroupBox * runtime_group = new QGroupBox(tr("Run-time directory")) ;
	runtime_group->setLayout( runtime_layout ) ;

	//

	if( m_installing )
		setFocusProxy( m_install_dir_edit_box ) ;
	else
		setFocusProxy( m_spool_dir_edit_box ) ;

	m_install_dir_edit_box->setText( qstr(config.value("=dir-install")) ) ;
	m_spool_dir_edit_box->setText( qstr(config.value("spool-dir")) ) ;
	m_config_dir_edit_box->setText( qstr(config.value("=dir-config")) ) ;
	m_runtime_dir_edit_box->setText( qstr(config.value("=dir-run")) ) ;

	QVBoxLayout * layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Directories"))) ;
	layout->addWidget( install_group ) ;
	layout->addWidget( spool_group ) ;
	layout->addWidget( config_group ) ;
	layout->addWidget( runtime_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	if( ! m_installing )
	{
		// if just configuring dont allow the base directories to change
		//
		m_install_dir_browse_button->setEnabled(false) ;
		m_install_dir_edit_box->setEnabled(false) ;
		m_config_dir_browse_button->setEnabled(false) ;
		m_config_dir_edit_box->setEnabled(false) ;
	}

	connect( m_install_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseInstall()) ) ;
	connect( m_spool_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseSpool()) ) ;
	connect( m_config_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseConfig()) ) ;
	connect( m_runtime_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseRun()) ) ;

	connect( m_install_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
	connect( m_spool_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
	connect( m_config_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
	connect( m_runtime_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
}

void DirectoryPage::browseInstall()
{
	QString s = browse(m_install_dir_edit_box->text()) ;
	if( ! s.isEmpty() )
		m_install_dir_edit_box->setText( s ) ;
}

void DirectoryPage::browseSpool()
{
	QString s = browse(m_spool_dir_edit_box->text()) ;
	if( ! s.isEmpty() )
		m_spool_dir_edit_box->setText( s ) ;
}

void DirectoryPage::browseConfig()
{
	QString s = browse(m_config_dir_edit_box->text()) ;
	if( ! s.isEmpty() )
		m_config_dir_edit_box->setText( s ) ;
}

void DirectoryPage::browseRuntime()
{
	QString s = browse(m_runtime_dir_edit_box->text()) ;
	if( ! s.isEmpty() )
		m_runtime_dir_edit_box->setText( s ) ;
}

QString DirectoryPage::browse( QString dir )
{
	return QFileDialog::getExistingDirectory( this , QString() , dir ) ;
}

std::string DirectoryPage::nextPage()
{
	return next1() ;
}

G::Path DirectoryPage::normalise( const G::Path & dir ) const
{
	// make relative paths relative to the home directory
	// since gui users wont have a sense of the cwd

	G::Path result = dir ;
	if( dir.isRelative() && Dir::home() != G::Path() )
	{
		if( dir.str() == "~" )
		{
			result = Dir::home() ;
		}
		else if( dir.str().at(0U) == '~' )
		{
			result = G::Path( Dir::home() , dir.str().substr(1U) ) ;
		}
		else
		{
			result = G::Path::join( Dir::home() , dir ) ;
		}
	}
	return result ;
}

void DirectoryPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	std::string ws = " " ;
	G::Path dir_install = normalise(G::Str::trimmed(value(m_install_dir_edit_box),ws)) ;
	dumpItem( stream , for_install , "dir-install" , dir_install ) ;
	dumpItem( stream , for_install , "dir-spool" , normalise(G::Str::trimmed(value(m_spool_dir_edit_box),ws)) ) ;
	dumpItem( stream , for_install , "dir-config" , normalise(G::Str::trimmed(value(m_config_dir_edit_box),ws)) ) ;
	dumpItem( stream , for_install , "dir-run" , normalise(G::Str::trimmed(value(m_runtime_dir_edit_box),ws)) ) ;

	dumpItem( stream , for_install , "dir-boot" , Dir::boot() ) ;
	dumpItem( stream , for_install , "dir-desktop" , Dir::desktop() ) ;
	dumpItem( stream , for_install , "dir-menu" , Dir::menu() ) ;
	dumpItem( stream , for_install , "dir-login" , Dir::login() ) ;
}

bool DirectoryPage::isComplete()
{
	return
		!m_install_dir_edit_box->text().isEmpty() &&
		!m_spool_dir_edit_box->text().isEmpty() &&
		!m_config_dir_edit_box->text().isEmpty() ;
}

std::string DirectoryPage::helpName() const
{
	return name() ;
}

// ==

DoWhatPage::DoWhatPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) :
		GPage(dialog,name,next_1,next_2,finish,close)
{
	m_pop_checkbox = new QCheckBox(tr("&POP3 server"));
	tip( m_pop_checkbox , "Allow clients to see spooled messages using POP" ) ;
	m_smtp_checkbox = new QCheckBox(tr("&SMTP server"));
	tip( m_smtp_checkbox , "Allow clients to submit new messages using SMTP" ) ;

	m_smtp_checkbox->setChecked( !config.booleanValue("no-smtp",false) ) ;
	m_pop_checkbox->setChecked( config.booleanValue("pop",false) ) ;

	QVBoxLayout * server_type_box_layout = new QVBoxLayout ;
	server_type_box_layout->addWidget( m_pop_checkbox ) ;
	server_type_box_layout->addWidget( m_smtp_checkbox ) ;

	QGroupBox * server_type_group = new QGroupBox(tr("Server")) ;
	server_type_group->setLayout( server_type_box_layout ) ;

	m_immediate_checkbox = new QRadioButton(tr("&Synchronously"));
	tip( m_immediate_checkbox , "--immediate" ) ;
	m_on_disconnect_checkbox = new QRadioButton(tr("When client &disconnects"));
	tip( m_on_disconnect_checkbox , "--poll 0" ) ;
	m_periodically_checkbox = new QRadioButton(tr("&Check periodically"));
	tip( m_periodically_checkbox , "--poll" ) ;
	m_on_demand_checkbox = new QRadioButton(tr("&Only when triggered"));
	tip( m_on_demand_checkbox , "--admin" ) ;

	if( config.booleanValue("immediate",false) )
		m_immediate_checkbox->setChecked(true) ;
	else if( config.booleanValue("forward-on-disconnect",false) || config.numericValue("poll",99U) == 0U )
		m_on_disconnect_checkbox->setChecked(true) ;
	else if( config.numericValue("poll",0U) != 0U )
		m_periodically_checkbox->setChecked(true) ;
	else
		m_on_demand_checkbox->setChecked(true) ;

	QLabel * period_label = new QLabel( tr("e&very") ) ;
	m_period_combo = new QComboBox ;
	m_period_combo->addItem( tr("second") ) ;
	m_period_combo->addItem( tr("minute") ) ;
	m_period_combo->addItem( tr("hour") ) ;
	if( config.numericValue("poll",99U) == 1U )
		m_period_combo->setCurrentIndex( 0 ) ;
	else if( config.numericValue("poll",1U) >= 60U ) // some information loss here :(
		m_period_combo->setCurrentIndex( 1 ) ;
	else
		m_period_combo->setCurrentIndex( 2 ) ;
	m_period_combo->setEditable( false ) ;
	period_label->setBuddy( m_period_combo ) ;

	QVBoxLayout * forwarding_box_layout = new QVBoxLayout ;
	forwarding_box_layout->addWidget( m_immediate_checkbox ) ;
	forwarding_box_layout->addWidget( m_on_disconnect_checkbox ) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( m_periodically_checkbox ) ;
		inner->addWidget( period_label ) ;
		inner->addWidget( m_period_combo ) ;
		forwarding_box_layout->addLayout( inner ) ;
	}
	forwarding_box_layout->addWidget( m_on_demand_checkbox ) ;

	m_forwarding_group = new QGroupBox(tr("Mail forwarding")) ;
	m_forwarding_group->setLayout( forwarding_box_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Installation type"))) ;
	layout->addWidget( server_type_group ) ;
	layout->addWidget( m_forwarding_group ) ;
	layout->addStretch() ;
	setLayout(layout);

	connect( m_pop_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_smtp_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_on_disconnect_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_periodically_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_smtp_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;

	onToggle() ;
}

void DoWhatPage::onToggle()
{
	m_period_combo->setEnabled( m_smtp_checkbox->isChecked() && m_periodically_checkbox->isChecked() ) ;
	m_forwarding_group->setEnabled( m_smtp_checkbox->isChecked() ) ;
}

std::string DoWhatPage::nextPage()
{
	// sneaky feature - see PopAccountsPage::nextPage()
	if( dialog().currentPageName() != name() )
		return m_smtp_checkbox->isChecked() ? next2() : std::string() ;

	return
		m_pop_checkbox->isChecked() ?
			next1() :
			next2() ;
}

void DoWhatPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "do-pop" , value(m_pop_checkbox) ) ;
	dumpItem( stream , for_install , "do-smtp" , value(m_smtp_checkbox) ) ;
	dumpItem( stream , for_install , "forward-immediate" , value(m_immediate_checkbox) ) ;
	dumpItem( stream , for_install , "forward-on-disconnect" , value(m_on_disconnect_checkbox) ) ;
	dumpItem( stream , for_install , "forward-poll" , value(m_periodically_checkbox) ) ;
	dumpItem( stream , for_install , "forward-poll-period" , value(m_period_combo) ) ;
}

bool DoWhatPage::isComplete()
{
	return
		m_pop_checkbox->isChecked() ||
		m_smtp_checkbox->isChecked() ;
}

std::string DoWhatPage::helpName() const
{
	return name() ;
}

// ==

PopPage::PopPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) :
		GPage(dialog,name,next_1,next_2,finish,close)
{
	QLabel * port_label = new QLabel( tr("P&ort:") ) ;
	m_port_edit_box = new QLineEdit( qstr(config.value("pop-port","110")) ) ;
	tip( m_port_edit_box , "--pop-port" ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	QHBoxLayout * server_layout = new QHBoxLayout ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;

	QGroupBox * server_group = new QGroupBox(tr("Local server")) ;
	server_group->setLayout( server_layout ) ;

	m_one = new QRadioButton( tr("&One client") ) ;
	m_shared = new QRadioButton( tr("&Many clients sharing a spool directory") ) ;
	m_pop_by_name = new QRadioButton( tr("M&any clients with separate spool directories") ) ;
	tip( m_pop_by_name , "--pop-by-name" ) ;

	m_no_delete_checkbox = new QCheckBox( tr("Disable message deletion") ) ;
	tip( m_no_delete_checkbox , "--pop-no-delete" ) ;
	m_auto_copy_checkbox = new QCheckBox( tr("Copy SMTP messages to all") ) ;
	tip( m_auto_copy_checkbox , "--filter=emailrelay-filter-copy" ) ;

	QGridLayout * radio_layout = new QGridLayout ;
	radio_layout->addWidget( m_one , 0 , 0 ) ;
	radio_layout->addWidget( m_shared , 1 , 0 ) ;
	radio_layout->addWidget( m_no_delete_checkbox , 1 , 1 ) ;
	radio_layout->addWidget( m_pop_by_name , 2 , 0 ) ;
	radio_layout->addWidget( m_auto_copy_checkbox , 2 , 1 ) ;

	bool pop_by_name = config.booleanValue("pop-by-name",false) ;
	bool pop_no_delete = config.booleanValue("pop-no-delete",false) ;
	bool pop_copy = config.value("filter","").find("emailrelay-filter-copy") != std::string::npos ;
	if( pop_by_name ) // "many clients with separate spool directories"
	{
		m_pop_by_name->setChecked( true ) ;
		m_auto_copy_checkbox->setChecked( pop_copy ) ;
	}
	else if( pop_no_delete ) // "many clients sharing a spool directory"
	{
		m_shared->setChecked( true ) ;
		m_no_delete_checkbox->setChecked( pop_no_delete ) ;
	}
	else // "one client" or "many clients sharing a spool directory"-without-nodelete
	{
		m_one->setChecked( true ) ;
	}

	QGroupBox * accounts_group = new QGroupBox(tr("Client accounts")) ;
	accounts_group->setLayout( radio_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("POP server"))) ;
	layout->addWidget( server_group ) ;
	layout->addWidget( accounts_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
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

void PopPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "pop-port" , value(m_port_edit_box) ) ;
	dumpItem( stream , for_install , "pop-simple" , value(m_one) ) ;
	dumpItem( stream , for_install , "pop-shared" , value(m_shared) ) ;
	dumpItem( stream , for_install , "pop-shared-no-delete" , value(m_no_delete_checkbox) ) ;
	dumpItem( stream , for_install , "pop-by-name" , value(m_pop_by_name) ) ;
	dumpItem( stream , for_install , "pop-by-name-auto-copy" , value(m_auto_copy_checkbox) ) ;
}

bool PopPage::isComplete()
{
	return ! m_port_edit_box->text().isEmpty() ;
}

void PopPage::onToggle()
{
	m_no_delete_checkbox->setEnabled( m_shared->isChecked() ) ;
	m_auto_copy_checkbox->setEnabled( m_pop_by_name->isChecked() ) ;
}

std::string PopPage::helpName() const
{
	return name() ;
}

// ==

PopAccountsPage::PopAccountsPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool have_accounts ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_have_accounts(have_accounts)
{
	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("APOP") ) ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 1 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QGridLayout * account_layout = new QGridLayout ;
	QLabel * name_label = new QLabel(tr("Name:")) ;
	QLabel * pwd_label = new QLabel(tr("Password:")) ;
	m_name_1 = new QLineEdit ;
	tip( m_name_1 , NameTip() ) ;
	m_pwd_1 = new QLineEdit ;
	tip( m_pwd_1 , PasswordTip() ) ;
	m_pwd_1->setEchoMode( QLineEdit::Password ) ;
	m_name_2 = new QLineEdit ;
	tip( m_name_2 , NameTip() ) ;
	m_pwd_2 = new QLineEdit ;
	tip( m_pwd_2 , PasswordTip() ) ;
	m_pwd_2->setEchoMode( QLineEdit::Password ) ;
	m_name_3 = new QLineEdit ;
	tip( m_name_3 , NameTip() ) ;
	m_pwd_3 = new QLineEdit ;
	tip( m_pwd_3 , PasswordTip() ) ;
	m_pwd_3->setEchoMode( QLineEdit::Password ) ;
	account_layout->addWidget( name_label , 0 , 0 ) ;
	account_layout->addWidget( pwd_label , 0 , 1 ) ;
	account_layout->addWidget( m_name_1 , 1 , 0 ) ;
	account_layout->addWidget( m_pwd_1 , 1 , 1 ) ;
	account_layout->addWidget( m_name_2 , 2 , 0 ) ;
	account_layout->addWidget( m_pwd_2 , 2 , 1 ) ;
	account_layout->addWidget( m_name_3 , 3 , 0 ) ;
	account_layout->addWidget( m_pwd_3 , 3 , 1 ) ;

	if( testMode() )
	{
		m_name_1->setText("me") ;
		m_pwd_1->setText("secret") ;
	}

	QGroupBox * account_group =
		m_have_accounts ?
			new QGroupBox(tr("New Accounts")) :
			new QGroupBox(tr("Accounts")) ;
	account_group->setLayout( account_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("POP accounts"))) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( mechanism_label ) ;
		inner->addWidget( m_mechanism_combo ) ;
		layout->addLayout( inner ) ;
	}
	layout->addWidget( account_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_mechanism_combo , SIGNAL(currentIndexChanged(QString)), this, SLOT(mechanismUpdateSlot(QString)) ) ;
	connect( m_name_1 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_pwd_1 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_name_2 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_pwd_2 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_name_3 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_pwd_3 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
}

std::string PopAccountsPage::nextPage()
{
	// only the dowhat page knows whether we should do smtp -- a special	
	// feature of the dowhat page's nextPage() is that if it detects
	// that it is not the current page (ie. if it's called from here)
	// then it will give us an empty string if no smtp is required

	return
		dialog().previousPage(2U).nextPage().empty() ?
			next2() :
			next1() ;
}

void PopAccountsPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "pop-auth-mechanism" , value(m_mechanism_combo) ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "pop-account-1-name" , value(m_name_1) ) ;
		dumpItem( stream , for_install , "pop-account-1-password" , encrypt(value(m_pwd_1),value(m_mechanism_combo)) );
		dumpItem( stream , for_install , "pop-account-2-name" , value(m_name_2) ) ;
		dumpItem( stream , for_install , "pop-account-2-password" , encrypt(value(m_pwd_2),value(m_mechanism_combo)) );
		dumpItem( stream , for_install , "pop-account-3-name" , value(m_name_3) ) ;
		dumpItem( stream , for_install , "pop-account-3-password" , encrypt(value(m_pwd_3),value(m_mechanism_combo)) );
	}
}

bool PopAccountsPage::isComplete()
{
	return
		m_have_accounts ||
		( !m_name_1->text().isEmpty() && !m_pwd_1->text().isEmpty() ) ||
		( !m_name_2->text().isEmpty() && !m_pwd_2->text().isEmpty() ) ||
		( !m_name_3->text().isEmpty() && !m_pwd_3->text().isEmpty() ) ;
}

std::string PopAccountsPage::helpName() const
{
	return name() ;
}

// ==

PopAccountPage::PopAccountPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool have_account ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_have_account(have_account)
{
	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("APOP") ) ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 1 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QGridLayout * account_layout = new QGridLayout ;
	QLabel * name_label = new QLabel(tr("Name:")) ;
	QLabel * pwd_label = new QLabel(tr("Password:")) ;
	m_name_1 = new QLineEdit ;
	tip( m_name_1 , NameTip() ) ;
	m_pwd_1 = new QLineEdit ;
	tip( m_pwd_1 , PasswordTip() ) ;
	m_pwd_1->setEchoMode( QLineEdit::Password ) ;
	account_layout->addWidget( name_label , 0 , 0 ) ;
	account_layout->addWidget( m_name_1 , 0 , 1 ) ;
	account_layout->addWidget( pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_pwd_1 , 1 , 1 ) ;

	if( testMode() )
	{
		m_name_1->setText("me") ;
		m_pwd_1->setText("secret") ;
	}

	QGroupBox * account_group =
		m_have_account ?
			new QGroupBox(tr("New Account")) :
			new QGroupBox(tr("Account")) ;
	account_group->setLayout( account_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("POP account"))) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( mechanism_label ) ;
		inner->addWidget( m_mechanism_combo ) ;
		layout->addLayout( inner ) ;
	}
	layout->addWidget( account_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_mechanism_combo , SIGNAL(currentIndexChanged(QString)), this, SLOT(mechanismUpdateSlot(QString)) ) ;
	connect( m_name_1 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_pwd_1 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
}

std::string PopAccountPage::nextPage()
{
	// (see above)
	return
		dialog().previousPage(2U).nextPage().empty() ?
			next2() :
			next1() ;
}

void PopAccountPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "pop-auth-mechanism" , value(m_mechanism_combo) ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "pop-account-1-name" , value(m_name_1) ) ;
		dumpItem( stream , for_install , "pop-account-1-password" , encrypt(value(m_pwd_1),value(m_mechanism_combo)) ) ;
	}
}

bool PopAccountPage::isComplete()
{
	return
		m_have_account ||
		( !m_name_1->text().isEmpty() && !m_pwd_1->text().isEmpty() ) ;
}

std::string PopAccountPage::helpName() const
{
	return name() ;
}

// ==

SmtpServerPage::SmtpServerPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool have_account ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_have_account(have_account)
{
	QLabel * port_label = new QLabel(tr("P&ort:")) ;
	m_port_edit_box = new QLineEdit( qstr(config.value("port","25")) ) ;
	tip( m_port_edit_box , "--port" ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	QHBoxLayout * server_layout = new QHBoxLayout ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;

	QGroupBox * server_group = new QGroupBox(tr("Local server")) ;
	server_group->setLayout( server_layout ) ;

	//

	m_auth_checkbox = new QCheckBox( tr("&Require authentication") ) ;
	tip( m_auth_checkbox , "--server-auth" ) ;
	m_auth_checkbox->setChecked( config.booleanValue("server-auth",false) ) ;

	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 0 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QHBoxLayout * mechanism_layout = new QHBoxLayout ;
	mechanism_layout->addWidget( mechanism_label ) ;
	mechanism_layout->addWidget( m_mechanism_combo ) ;

	//

	QLabel * account_name_label = new QLabel(tr("&Name:")) ;
	m_account_name = new QLineEdit ;
	tip( m_account_name , NameTip() ) ;
	account_name_label->setBuddy( m_account_name ) ;

	QLabel * account_pwd_label = new QLabel(tr("&Password:")) ;
	m_account_pwd = new QLineEdit ;
	tip( m_account_pwd , PasswordTip() ) ;
	m_account_pwd->setEchoMode( QLineEdit::Password ) ;
	account_pwd_label->setBuddy( m_account_pwd ) ;

	if( testMode() )
	{
		m_auth_checkbox->setChecked(true) ;
		m_account_name->setText("me") ;
		m_account_pwd->setText("secret") ;
	}

	QGridLayout * account_layout = new QGridLayout ;
	account_layout->addWidget( account_name_label , 0 , 0 ) ;
	account_layout->addWidget( m_account_name , 0 , 1 ) ;
	account_layout->addWidget( account_pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_account_pwd , 1 , 1 ) ;

	m_account_group =
		m_have_account ?
			new QGroupBox(tr("New Account")) :
			new QGroupBox(tr("Account")) ;
	m_account_group->setLayout( account_layout ) ;

	//

	QLabel * trust_label = new QLabel(tr("&IP address:")) ;
	m_trust_address = new QLineEdit ;
	trust_label->setBuddy( m_trust_address ) ;
	tip( m_trust_address , "eg. 192.168.0.0/8" ) ;
	m_trust_group = new QGroupBox(tr("Exemptions")) ;
	QHBoxLayout * trust_layout = new QHBoxLayout ;
	trust_layout->addWidget( trust_label ) ;
	trust_layout->addWidget( m_trust_address ) ;
	m_trust_group->setLayout( trust_layout ) ;
	m_trust_address->setText( qstr(testMode()?"192.168.1.*":"") ) ;
	
	//

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("SMTP server"))) ;
	layout->addWidget( server_group ) ;
	layout->addWidget( m_auth_checkbox ) ;
	layout->addLayout( mechanism_layout ) ;
	layout->addWidget( m_account_group ) ;
	layout->addWidget( m_trust_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_mechanism_combo , SIGNAL(currentIndexChanged(QString)), this, SLOT(mechanismUpdateSlot(QString)) ) ;
	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_name , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_pwd , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_trust_address , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;

	onToggle() ;
}

std::string SmtpServerPage::nextPage()
{
	return next1() ;
}

void SmtpServerPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "smtp-server-port" , value(m_port_edit_box) ) ;
	dumpItem( stream , for_install , "smtp-server-auth" , value(m_auth_checkbox) ) ;
	dumpItem( stream , for_install , "smtp-server-auth-mechanism" , value(m_mechanism_combo) ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "smtp-server-account-name" , value(m_account_name) ) ;
		dumpItem( stream , for_install , "smtp-server-account-password" ,
			encrypt(value(m_account_pwd),value(m_mechanism_combo)) ) ;
	}
	dumpItem( stream , for_install , "smtp-server-trust" , value(m_trust_address) ) ;
}

void SmtpServerPage::onToggle()
{
	m_account_group->setEnabled( m_auth_checkbox->isChecked() ) ;
	m_mechanism_combo->setEnabled( m_auth_checkbox->isChecked() ) ;
	m_trust_group->setEnabled( m_auth_checkbox->isChecked() ) ;
}

bool SmtpServerPage::isComplete()
{
	return
		! m_port_edit_box->text().isEmpty() && (
		m_have_account ||
		!m_auth_checkbox->isChecked() || (
			! m_account_name->text().isEmpty() &&
			! m_account_pwd->text().isEmpty() ) ) ;
}

std::string SmtpServerPage::helpName() const
{
	return name() ;
}

// ==

SmtpClientPage::SmtpClientPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool have_account ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_have_account(have_account)
{
	QLabel * server_label = new QLabel(tr("&Hostname:")) ;
	m_server_edit_box = new QLineEdit ;
	server_label->setBuddy( m_server_edit_box ) ;

	tip( m_server_edit_box , "--forward-to" ) ;
	std::string address = config.value("forward-to") ;
	address = address.empty() ? config.value("as-client") : address ;
	address = address.empty() ? std::string("smtp.example.com:25") : address ;
	std::string net_address = G::Str::head( address , address.find_last_of(":") , std::string() ) ;
	std::string port = G::Str::tail( address , address.find_last_of(":") , std::string() ) ;
	m_server_edit_box->setText( qstr(net_address) ) ;

	QLabel * port_label = new QLabel( tr("P&ort:") ) ;
	m_port_edit_box = new QLineEdit( qstr(port) ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	QHBoxLayout * server_layout = new QHBoxLayout ;
	server_layout->addWidget( server_label ) ;
	server_layout->addWidget( m_server_edit_box ) ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;
	server_layout->setStretchFactor( m_server_edit_box , 4 ) ;

	QGroupBox * server_group = new QGroupBox(tr("Remote server")) ;
	server_group->setLayout( server_layout ) ;

	m_tls_checkbox = new QCheckBox( tr("&Allow TLS/SSL encryption") ) ;
	m_tls_checkbox->setChecked( config.booleanValue("client-tls",true) ) ;
	tip( m_tls_checkbox , "--client-tls" ) ;

	m_auth_checkbox = new QCheckBox( tr("&Supply authentication") ) ;
	m_auth_checkbox->setChecked( config.booleanValue("client-auth",false) ) ;
	tip( m_auth_checkbox , "--client-auth" ) ;

	m_mechanism_combo = new QComboBox ;
	m_mechanism_combo->addItem( tr("CRAM-MD5") ) ;
	m_mechanism_combo->addItem( tr("LOGIN") ) ;
	m_mechanism_combo->setCurrentIndex( 0 ) ;
	m_mechanism_combo->setEditable( false ) ;
	QLabel * mechanism_label = new QLabel( tr("Authentication &mechanism") ) ;
	mechanism_label->setBuddy( m_mechanism_combo ) ;

	QLabel * account_name_label = new QLabel(tr("&Name:")) ;
	m_account_name = new QLineEdit ;
	tip( m_account_name , NameTip() ) ;
	account_name_label->setBuddy( m_account_name ) ;

	QLabel * account_pwd_label = new QLabel(tr("&Password:")) ;
	m_account_pwd = new QLineEdit ;
	tip( m_account_pwd , PasswordTip() ) ;
	m_account_pwd->setEchoMode( QLineEdit::Password ) ;
	account_pwd_label->setBuddy( m_account_pwd ) ;

	if( testMode() )
	{
		m_auth_checkbox->setChecked(true) ;
		m_account_name->setText("me") ;
		m_account_pwd->setText("secret") ;
	}

	QGridLayout * account_layout = new QGridLayout ;
	account_layout->addWidget( account_name_label , 0 , 0 ) ;
	account_layout->addWidget( m_account_name , 0 , 1 ) ;
	account_layout->addWidget( account_pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_account_pwd , 1 , 1 ) ;

	m_account_group =
		m_have_account ?
			new QGroupBox(tr("New Account")) :
			new QGroupBox(tr("Account")) ;
	m_account_group->setLayout( account_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("SMTP client"))) ;
	layout->addWidget( server_group ) ;
	layout->addWidget( m_tls_checkbox ) ;
	layout->addWidget( m_auth_checkbox ) ;
	{
		QHBoxLayout * inner = new QHBoxLayout ;
		inner->addWidget( mechanism_label ) ;
		inner->addWidget( m_mechanism_combo ) ;
		layout->addLayout( inner ) ;
	}
	layout->addWidget( m_account_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_mechanism_combo , SIGNAL(currentIndexChanged(QString)), this, SLOT(mechanismUpdateSlot(QString)) ) ;
	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_server_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_name , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_pwd , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_tls_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;

	onToggle() ;
}

void SmtpClientPage::onToggle()
{
	m_account_group->setEnabled( m_auth_checkbox->isChecked() ) ;
	m_mechanism_combo->setEnabled( m_auth_checkbox->isChecked() ) ;
}

std::string SmtpClientPage::nextPage()
{
	return next1() ;
}

void SmtpClientPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "smtp-client-host" , value(m_server_edit_box) ) ;
	dumpItem( stream , for_install , "smtp-client-port" , value(m_port_edit_box) ) ;
	dumpItem( stream , for_install , "smtp-client-tls" , value(m_tls_checkbox) ) ;
	dumpItem( stream , for_install , "smtp-client-auth" , value(m_auth_checkbox) ) ;
	dumpItem( stream , for_install , "smtp-client-auth-mechanism" , value(m_mechanism_combo) ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "smtp-client-account-name" , value(m_account_name) ) ;
		dumpItem( stream , for_install , "smtp-client-account-password" ,
			encrypt(value(m_account_pwd),value(m_mechanism_combo)) ) ;
	}
}

bool SmtpClientPage::isComplete()
{
	return
		! m_port_edit_box->text().isEmpty() &&
		! m_server_edit_box->text().isEmpty() && (
		m_have_account ||
		! m_auth_checkbox->isChecked() || (
			! m_account_name->text().isEmpty() &&
			! m_account_pwd->text().isEmpty() ) ) ;
}

std::string SmtpClientPage::helpName() const
{
	return name() ;
}

// ==

LoggingPage::LoggingPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) :
		GPage(dialog,name,next_1,next_2,finish,close)
{
	m_debug_checkbox = new QCheckBox( tr("&Debug messages") ) ;
	tip( m_debug_checkbox , "--debug" ) ;
	m_verbose_checkbox = new QCheckBox( tr("&Verbose logging") ) ;
	tip( m_verbose_checkbox , "--verbose" ) ;
	m_syslog_checkbox = new QCheckBox( tr("&Write to the system log") ) ;
	tip( m_syslog_checkbox , "--syslog" ) ;

	QVBoxLayout * logging_layout = new QVBoxLayout ;
	logging_layout->addWidget( m_verbose_checkbox ) ;
	logging_layout->addWidget( m_syslog_checkbox ) ;
	logging_layout->addWidget( m_debug_checkbox ) ;

	bool syslog_override = config.booleanValue("syslog",false) ;
	bool as_client = config.booleanValue("as-client",false) ;
	bool no_syslog = config.booleanValue("no-syslog",false) ;
	bool syslog = syslog_override || !(as_client||no_syslog) ; // true by default

	m_syslog_checkbox->setChecked( syslog ) ;
	m_verbose_checkbox->setChecked( config.booleanValue("verbose",false) ) ;
	m_debug_checkbox->setEnabled( config.booleanValue("debug",false) ) ;

	QGroupBox * logging_group = new QGroupBox(tr("Logging")) ;
	logging_group->setLayout( logging_layout ) ;

	//

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Logging"))) ;
	layout->addWidget(logging_group) ;
	layout->addStretch() ;
	setLayout( layout ) ;
}

std::string LoggingPage::nextPage()
{
	return next1() ;
}

void LoggingPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "logging-verbose" , value(m_verbose_checkbox) ) ;
	dumpItem( stream , for_install , "logging-debug" , value(m_debug_checkbox) ) ;
	dumpItem( stream , for_install , "logging-syslog" , value(m_syslog_checkbox) ) ;
}

std::string LoggingPage::helpName() const
{
	return name() ;
}

// ==

ListeningPage::ListeningPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) :
		GPage(dialog,name,next_1,next_2,finish,close)
{
	m_listening_interface = new QLineEdit ;
	tip( m_listening_interface , "eg. 127.0.0.1,192.168.1.1" ) ;
	m_all_radio = new QRadioButton(tr("&All interfaces")) ;
	m_one_radio = new QRadioButton(tr("&One")) ;
	tip( m_one_radio , "--interface" ) ;
	QLabel * listening_interface_label = new QLabel(tr("&Interface:")) ;
	listening_interface_label->setBuddy( m_listening_interface ) ;

	if( config.contains("interface") )
	{
		m_one_radio->setChecked( true ) ;
		std::string interfaces = config.value("interface") ;
		m_listening_interface->setEnabled( true ) ;
		m_listening_interface->setText( qstr(interfaces) ) ;
	}
	else
	{
		m_all_radio->setChecked( true ) ;
		m_listening_interface->setEnabled( false ) ;
	}

	QGridLayout * listening_layout = new QGridLayout ;
	listening_layout->addWidget( m_all_radio , 0 , 0 ) ;
	listening_layout->addWidget( m_one_radio , 1 , 0 ) ;
	listening_layout->addWidget( listening_interface_label , 1 , 1 ) ;
	listening_layout->addWidget( m_listening_interface , 1 , 2 ) ;

	QGroupBox * listening_group = new QGroupBox(tr("Listen on")) ;
	listening_group->setLayout( listening_layout ) ;

	//

	m_remote_checkbox = new QCheckBox(tr("&Allow remote clients")) ;
	tip( m_remote_checkbox , "--remote-clients" ) ;
	m_remote_checkbox->setChecked( config.booleanValue("remote-clients",false) ) ;

	QHBoxLayout * connections_layout = new QHBoxLayout ;
	connections_layout->addWidget( m_remote_checkbox ) ;

	QGroupBox * connections_group = new QGroupBox(tr("Clients")) ;
	connections_group->setLayout( connections_layout ) ;

	//

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Connections"))) ;
	layout->addWidget(listening_group) ;
	layout->addWidget(connections_group) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_all_radio , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_all_radio , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_one_radio , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_one_radio , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_listening_interface , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;

	onToggle() ;
}

std::string ListeningPage::nextPage()
{
	return next1() ;
}

void ListeningPage::onToggle()
{
	m_listening_interface->setEnabled( ! m_all_radio->isChecked() ) ;
}

bool ListeningPage::isComplete()
{
	G_DEBUG( "ListeningPage::isComplete" ) ;
	return
		m_all_radio->isChecked() ||
		!m_listening_interface->text().isEmpty() ;
}

void ListeningPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "listening-all" , value(m_all_radio) ) ;
	dumpItem( stream , for_install , "listening-interface" , value(m_listening_interface) ) ;
	dumpItem( stream , for_install , "listening-remote" , value(m_remote_checkbox) ) ;
}

std::string ListeningPage::helpName() const
{
	return name() ;
}

// ==

StartupPage::StartupPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
	bool start_on_boot_able , bool is_mac ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_is_mac(is_mac)
{
	m_on_boot_checkbox = new QCheckBox( tr("At &system startup") ) ;
	m_at_login_checkbox = new QCheckBox( tr("&When logging in") ) ;
	QVBoxLayout * auto_layout = new QVBoxLayout ;
	auto_layout->addWidget( m_on_boot_checkbox ) ;
	auto_layout->addWidget( m_at_login_checkbox ) ;

	m_add_menu_item_checkbox = new QCheckBox( tr("Add to start menu") ) ;
	m_add_desktop_item_checkbox = new QCheckBox( tr("Add to desktop") ) ;

	QVBoxLayout * manual_layout = new QVBoxLayout ;
	manual_layout->addWidget( m_add_menu_item_checkbox ) ;
	manual_layout->addWidget( m_add_desktop_item_checkbox ) ;

	if( m_is_mac )
	{
		m_add_menu_item_checkbox->setEnabled( false ) ;
		m_add_desktop_item_checkbox->setEnabled( false ) ;
	}
	m_at_login_checkbox->setEnabled( !Dir::login().str().empty() ) ;
	m_on_boot_checkbox->setEnabled( start_on_boot_able ) ;

	m_at_login_checkbox->setChecked( !Dir::login().str().empty() && config.booleanValue("start-at-login",false) ) ;
	m_add_menu_item_checkbox->setChecked( !m_is_mac && config.booleanValue("start-link-menu",true) ) ;
	m_add_desktop_item_checkbox->setChecked( !m_is_mac && config.booleanValue("start-link-desktop",false) ) ;
	m_on_boot_checkbox->setChecked( start_on_boot_able && config.booleanValue("start-on-boot",false) ) ;

	QGroupBox * auto_group = new QGroupBox(tr("Automatic")) ;
	auto_group->setLayout( auto_layout ) ;

	QGroupBox * manual_group = new QGroupBox(tr("Manual")) ;
	manual_group->setLayout( manual_layout ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Server startup"))) ;
	layout->addWidget(auto_group) ;
	layout->addWidget(manual_group) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_on_boot_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
	connect( m_add_desktop_item_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
}

bool StartupPage::isComplete()
{
	return true ;
}

std::string StartupPage::nextPage()
{
	return next1() ;
}

void StartupPage::dump( std::ostream & stream , bool for_install ) const
{
	GPage::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "start-on-boot" , value(m_on_boot_checkbox) ) ;
	dumpItem( stream , for_install , "start-at-login" , value(m_at_login_checkbox) ) ;
	dumpItem( stream , for_install , "start-link-menu" , value(m_add_menu_item_checkbox) ) ;
	dumpItem( stream , for_install , "start-link-desktop" , value(m_add_desktop_item_checkbox) ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "start-is-mac" , value(m_is_mac) ) ;
	}
}

std::string StartupPage::helpName() const
{
	return name() ;
}

// ==

ReadyPage::ReadyPage( GDialog & dialog , const G::MapFile & , const std::string & name , const std::string & next_1 ,
	const std::string & next_2 , bool finish , bool close , bool installing ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_installing(installing)
{
	m_label = new QLabel( text() ) ;

	QVBoxLayout *layout = new QVBoxLayout;
	std::string message = std::string() + "Ready to " + verb(false) ;
	layout->addWidget(newTitle(tr(message.c_str()))) ;
	layout->addWidget(m_label);
	setLayout(layout);
}

void ReadyPage::onShow( bool )
{
}

std::string ReadyPage::verb( bool pp ) const
{
	return std::string( m_installing ? (pp?"installed":"install") : (pp?"configured":"configure") ) ;
}

QString ReadyPage::text() const
{
	std::string html = std::string() +
		"<center>" +
		"<p>E-MailRelay will now be " + verb(true) + ".</p>" +
		"</center>" ;
	return GPage::tr(html.c_str()) ;
}

std::string ReadyPage::nextPage()
{
	return next1() ;
}

void ReadyPage::dump( std::ostream & s , bool for_install ) const
{
	GPage::dump( s , for_install ) ;
}

std::string ReadyPage::helpName() const
{
	return name() ;
}

// ==

ProgressPage::ProgressPage( GDialog & dialog , const G::MapFile & , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
	Installer & installer ) :
		GPage(dialog,name,next_1,next_2,finish,close) ,
		m_installer(installer)
{
	m_text_edit = new QTextEdit;
	m_text_edit->setReadOnly(true) ;
	m_text_edit->setWordWrapMode(QTextOption::NoWrap) ;
	m_text_edit->setLineWrapMode(QTextEdit::NoWrap) ;
	m_text_edit->setFontFamily("courier") ;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Installing"))) ;
	layout->addWidget(m_text_edit) ;
	setLayout( layout ) ;
}

void ProgressPage::onShow( bool back )
{
	if( ! back )
	{
		// dump install variables into a stringstream
		std::stringstream ss ;
		dialog().dumpInstallVariables( ss ) ;
		if( testMode() )
		{
			std::ofstream f( "installer.txt" ) ;
			f << ss.str() ;
		}

		// start running the installer
		m_installer.start( ss ) ; // reads from istream
		dialog().wait( true ) ; // disable buttons

		// run a continuous zero-length timer that calls poke()
		m_text = QString() ;
		m_text_edit->setPlainText( m_text ) ;
		m_timer = new QTimer( this ) ;
		connect( m_timer , SIGNAL(timeout()) , this , SLOT(poke()) ) ;
		m_timer->start() ;
	}
}

void ProgressPage::poke()
{
	try
	{
		if( m_timer == NULL )
			throw std::runtime_error("internal error: no timer") ;

		bool more = m_installer.next() ;
		if( more )
		{
			addLine( m_installer.beforeText() + "... " ) ;
			m_installer.run() ;
			addLine( m_installer.afterText() + "\n" ) ;
		}
		else
		{
			dialog().wait( false ) ;
			m_timer->stop() ;
			{ QTimer * p = m_timer ; m_timer = NULL ; delete p ; }
			if( m_installer.failed() )
				addLine( "** failed **" ) ;
			else
				addLine( "== finished ==" ) ;
		}
		emit pageUpdateSignal() ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception in timer callback: " << e.what() << std::endl ;
		throw ;
	}
	catch(...)
	{
		std::cerr << "exception in timer callback" << std::endl ;
		throw ;
	}
}

void ProgressPage::addLine( const std::string & line )
{
	G_DEBUG( "ProgressPage::addLine: [" << G::Str::printable(line) << "]" ) ;
	m_text.append( qstr(line) ) ;
	m_text_edit->setFontFamily("courier") ;
	m_text_edit->setPlainText( m_text ) ;
}

std::string ProgressPage::nextPage()
{
	return next1() ;
}

void ProgressPage::dump( std::ostream & s , bool for_install ) const
{
	GPage::dump( s , for_install ) ;
}

bool ProgressPage::closeButton() const
{
	bool close_button = GPage::closeButton() ;
	if( m_installer.done() && m_installer.failed() )
		close_button = false ;

	G_DEBUG( "ProgressPage::closeButton: " << close_button ) ;
	return close_button ;
}

bool ProgressPage::isComplete()
{
	return m_installer.done() && !m_installer.failed() ;
}

std::string ProgressPage::helpName() const
{
	return name() ;
}

// ==

EndPage_::EndPage_( GDialog & dialog , const G::MapFile & , const std::string & name ) :
	GPage(dialog,name,"","",true,true)
{
	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(newTitle(tr("Finish"))) ;
	layout->addStretch() ;
	setLayout( layout ) ;
}

std::string EndPage_::nextPage()
{
	return std::string() ;
}

void EndPage_::dump( std::ostream & s , bool for_install ) const
{
	GPage::dump( s , for_install ) ;
}

/// \file pages.cpp
