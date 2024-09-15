//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file guipages.cpp
///

#include "gdef.h"
#include "gqt.h"
#include "guipages.h"
#include "guilegal.h"
#include "guidialog.h"
#include "guidir.h"
#include "guiboot.h"
#include "gcodepage.h"
#include "gmapfile.h"
#include "gstr.h"
#include "gfile.h"
#include "gprocess.h"
#include "gidentity.h"
#include "gmd5.h"
#include "ghash.h"
#include "gbase64.h"
#include "gxtext.h"
#include "gstringview.h"
#include "genvironment.h"
#include "glog.h"
#include <stdexcept>
#include <fstream>
#include <cstring>

#ifndef G_NO_MOC_INCLUDE
#include "moc_guipages.cpp"
#endif

// ==

TitlePage::TitlePage( Gui::Dialog & dialog , const G::MapFile & , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) :
		Gui::Page(dialog,name,next_1,next_2)
{
	m_label = new QLabel( QString(Gui::Legal::text()) ) ;
	m_label->setAlignment( Qt::AlignHCenter ) ;

	{
		std::string credit = "<small><font color=\"#888\">" ;
		credit.append( G::Str::join( "\n\n" , Gui::Legal::credits() ) ) ;
		credit.append( "</font></small>" ) ;
		m_credit = new QLabel( QString(credit.c_str()) ) ;
		m_credit->setAlignment( Qt::AlignHCenter ) ;
		m_credit->setWordWrap( true ) ;
	}

	auto * layout = new QVBoxLayout ;
	//: page title of opening page
	layout->addWidget( newTitle(tr("E-MailRelay")) ) ;
	layout->addWidget( m_label ) ;
	layout->addStretch() ;
	layout->addWidget( m_credit ) ;
	setLayout(layout);
}

std::string TitlePage::nextPage()
{
	return next1() ;
}

void TitlePage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
}

// ==

LicensePage::LicensePage( Gui::Dialog & dialog , const G::MapFile & , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool accepted ) :
		Gui::Page(dialog,name,next_1,next_2)
{
	m_text_edit = new QTextEdit ;
	m_text_edit->setReadOnly( true ) ;
	m_text_edit->setWordWrapMode( QTextOption::NoWrap ) ;
	m_text_edit->setLineWrapMode( QTextEdit::NoWrap ) ;
	m_text_edit->setFontFamily( "courier" ) ;
	m_text_edit->setPlainText( QString(Gui::Legal::license()) ) ;

	m_agree_checkbox = new QCheckBox( tr("I agree to the terms and conditions of the license") );
	setFocusProxy( m_agree_checkbox ) ;

	if( testMode() || accepted )
		m_agree_checkbox->setChecked( true ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of license page
	layout->addWidget( newTitle(tr("License")) ) ;
	layout->addWidget( m_text_edit ) ;
	layout->addWidget( m_agree_checkbox ) ;
	setLayout( layout ) ;

	connect( m_agree_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
}

std::string LicensePage::nextPage()
{
	return next1() ;
}

void LicensePage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
}

bool LicensePage::isComplete()
{
	return m_agree_checkbox->isChecked() ;
}

std::string LicensePage::helpUrl( const std::string & language ) const
{
	return "https://www.gnu.org/licenses/gpl-3.0." + language + ".html" ;
}

// ==

DirectoryPage::DirectoryPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ,
	bool installing , bool is_windows , bool is_mac ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_installing(installing) ,
		m_is_mac(is_mac) ,
		m_other_dir_changed(false)
{
	//: install directory, text-edit label
	m_install_dir_label = new QLabel( tr("Directory:") ) ;
	m_install_dir_edit_box = new QLineEdit ;
	m_install_dir_label->setBuddy( m_install_dir_edit_box ) ;
	//: activate a file-open dialog box to select a directory
	m_install_dir_browse_button = new QPushButton( tr("Browse") ) ;

	auto * install_layout = new QHBoxLayout ;
	install_layout->addWidget( m_install_dir_label ) ;
	install_layout->addWidget( m_install_dir_edit_box ) ;
	install_layout->addWidget( m_install_dir_browse_button ) ;

	//: install directory, group label
	QGroupBox * install_group = new QGroupBox( tr("Installation directory") ) ;
	install_group->setLayout( install_layout ) ;

	//

	//: spool directory, text-edit label
	m_spool_dir_label = new QLabel( tr("Directory:") ) ;
	m_spool_dir_edit_box = new QLineEdit ;
	tip( m_spool_dir_edit_box , tr("--spool-dir") ) ;
	m_spool_dir_label->setBuddy( m_spool_dir_edit_box ) ;
	//: activate a file-open dialog box to select a directory
	m_spool_dir_browse_button = new QPushButton( tr("Browse") ) ;

	auto * spool_layout = new QHBoxLayout ;
	spool_layout->addWidget( m_spool_dir_label ) ;
	spool_layout->addWidget( m_spool_dir_edit_box ) ;
	spool_layout->addWidget( m_spool_dir_browse_button ) ;

	//: spool directory, group label
	QGroupBox * spool_group = new QGroupBox( tr("Spool directory") ) ;
	spool_group->setLayout( spool_layout ) ;

	//

	m_config_dir_label = new QLabel( tr("Directory:") ) ;
	m_config_dir_edit_box = new QLineEdit ;
	m_config_dir_label->setBuddy( m_config_dir_edit_box ) ;
	//: activate a file-open dialog box to select a directory
	m_config_dir_browse_button = new QPushButton( tr("Browse") ) ;

	auto * config_layout = new QHBoxLayout ;
	config_layout->addWidget( m_config_dir_label ) ;
	config_layout->addWidget( m_config_dir_edit_box ) ;
	config_layout->addWidget( m_config_dir_browse_button ) ;

	QGroupBox * config_group = new QGroupBox( tr("Configuration directory") ) ;
	config_group->setLayout( config_layout ) ;

	//

	//: run-time directory, text-edit label
	m_runtime_dir_label = new QLabel( tr("Directory:") ) ;
	m_runtime_dir_edit_box = new QLineEdit ;
	tip( m_runtime_dir_edit_box , tr("--pid-file, --log-file") ) ;
	m_runtime_dir_label->setBuddy( m_runtime_dir_edit_box ) ;
	//: activate a file-open dialog box to select a directory
	m_runtime_dir_browse_button = new QPushButton( tr("Browse") ) ;

	auto * runtime_layout = new QHBoxLayout ;
	runtime_layout->addWidget( m_runtime_dir_label ) ;
	runtime_layout->addWidget( m_runtime_dir_edit_box ) ;
	runtime_layout->addWidget( m_runtime_dir_browse_button ) ;

	//: run-time directory, group label
	QGroupBox * runtime_group = new QGroupBox( tr("Run-time directory") ) ;
	runtime_group->setLayout( runtime_layout ) ;

	//

	m_notice_label = new QLabel ;
	m_notice_label->setEnabled( false ) ;
	auto * notice_layout = new QHBoxLayout ;
	notice_layout->addStretch() ;
	notice_layout->addWidget( m_notice_label ) ;
	notice_layout->addStretch() ;

	//

	if( m_installing )
		setFocusProxy( m_install_dir_edit_box ) ;
	else
		setFocusProxy( m_spool_dir_edit_box ) ;

	m_install_dir_start = GQt::qstring_from_path( G::Path(config.value("=dir-install")) ) ;
	m_install_dir_edit_box->setText( m_install_dir_start ) ;
	m_spool_dir_start = GQt::qstring_from_path( G::Path(config.value("spool-dir")) ) ;
	m_spool_dir_edit_box->setText( m_spool_dir_start ) ;
	m_config_dir_start = GQt::qstring_from_path( G::Path(config.value("=dir-config")) ) ;
	m_config_dir_edit_box->setText( m_config_dir_start ) ;
	m_runtime_dir_start = GQt::qstring_from_path( G::Path(config.value("=dir-run")) ) ;
	m_runtime_dir_edit_box->setText( m_runtime_dir_start ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of directories page
	layout->addWidget( newTitle(tr("Directories")) ) ;
	layout->addWidget( install_group ) ;
	layout->addWidget( spool_group ) ;
	layout->addWidget( config_group ) ;
	layout->addWidget( runtime_group ) ;
	layout->addStretch() ;
	layout->addLayout( notice_layout ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	if( ! m_installing )
	{
		// if just configuring dont allow the base directories to change
		//
		m_install_dir_browse_button->setEnabled( false ) ;
		m_install_dir_edit_box->setEnabled( false ) ;
		m_config_dir_browse_button->setEnabled( false ) ;
		m_config_dir_edit_box->setEnabled( false ) ;
	}

	connect( m_install_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseInstall()) ) ;
	connect( m_spool_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseSpool()) ) ;
	connect( m_config_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseConfig()) ) ;
	connect( m_runtime_dir_browse_button , SIGNAL(clicked()) , this , SLOT(browseRuntime()) ) ;

	if( m_installing )
	{
		// automagic prefixing
		connect( m_install_dir_edit_box , SIGNAL(textChanged(QString)), this, SLOT(onInstallDirChange()) );
		connect( m_spool_dir_edit_box , SIGNAL(textChanged(QString)), this, SLOT(onOtherDirChange()) );
		connect( m_config_dir_edit_box , SIGNAL(textChanged(QString)), this, SLOT(onOtherDirChange()) );
		connect( m_runtime_dir_edit_box , SIGNAL(textChanged(QString)), this, SLOT(onOtherDirChange()) );
		if( testMode() )
		{
			const char * emailrelay = reinterpret_cast<const char*>(
				G::is_windows() ?
					u8"\u00C9-\u00B5\u00E4\u00EF\u2502\u0052\u00EB\u2514\u00E4\u00FF" : // cp437 compatible
					u8"\u4E18\u070B\u4ECE\u03B1\u0269\u013A\u16B1\u0115\u013A\u0103\u0423" ) ;
			G::Path tmp_base = is_windows ? G::Environment::getPath("TEMP","c:/temp") : G::Path("/tmp" ) ;
			G::Path tmp_dir = tmp_base / std::string(emailrelay).append(1U,'.').append(G::Process::Id().str()) ;
			QString old_value = m_install_dir_edit_box->text() ;
			G::Path old_path = GQt::path_from_qstring( old_value ) ;
			G::Path new_path = G::Path::join( tmp_dir , old_path.withoutRoot() ) ;
			QString new_value = GQt::qstring_from_path( new_path ) ;
			m_install_dir_edit_box->setText( new_value ) ;
		}
	}

	connect( m_install_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
	connect( m_spool_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
	connect( m_config_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
	connect( m_runtime_dir_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()));
}

void DirectoryPage::checkCharacterSets()
{
	bool ok =
		checkCharacterSet( m_install_dir_edit_box->text() ) &&
		checkCharacterSet( m_spool_dir_edit_box->text() ) &&
		checkCharacterSet( m_config_dir_edit_box->text() ) &&
		checkCharacterSet( m_runtime_dir_edit_box->text() ) ;
	if( ok )
	{
		m_notice_label->setEnabled( false ) ;
		m_notice_label->setText( "" ) ;
	}
	else if( !m_notice_label->isEnabled() )
	{
		m_notice_label->setEnabled( true ) ;
		m_notice_label->setTextFormat( Qt::RichText ) ;

		//: one or more invalid characters in an installation directory
		QString message = tr("warning: invalid characters") ;
		const QChar triangle( L'\u26A0' ) ;
		QString text = QString("<font color=\"#cc0\">").append(triangle).append(" ").append(message).append("</font>") ;
		m_notice_label->setText( text ) ;
	}
}

bool DirectoryPage::checkCharacterSet( QString s )
{
	if( s.isEmpty() )
	{
		return true ;
	}
	else if( G::is_windows() )
	{
		return std::string::npos ==
			G::CodePage::toCodePageOem(GQt::u8string_from_qstring(s)).find(G::CodePage::oem_error) ;
	}
	else
	{
		return true ;
	}
}

void DirectoryPage::onOtherDirChange()
{
	checkCharacterSets() ;
	m_other_dir_changed = true ;
}

void DirectoryPage::onInstallDirChange()
{
	checkCharacterSets() ;
	if( !m_other_dir_changed )
	{
		QString orig = m_install_dir_start ;
		QString s = m_install_dir_edit_box->text() ;
		if( s.endsWith(orig) )
		{
			QString prefix = s.mid( 0 , s.length() - m_install_dir_start.length() ) ;
			m_spool_dir_edit_box->setText( prefix + m_spool_dir_start ) ;
			m_config_dir_edit_box->setText( prefix + m_config_dir_start ) ;
			m_runtime_dir_edit_box->setText( prefix + m_runtime_dir_start ) ;
			m_other_dir_changed = false ;
		}
		else if( s.length() > 2 && s.at(1U) == ':' &&
			orig.length() > 2 && orig.at(1U) == ':' &&
			s.mid(2).endsWith(orig.mid(2)) )
		{
			QString prefix = s.mid( 0 , s.length() - m_install_dir_start.length() + 2 ) ;
			m_spool_dir_edit_box->setText( prefix + m_spool_dir_start.mid(2) ) ;
			m_config_dir_edit_box->setText( prefix + m_config_dir_start.mid(2) ) ;
			m_runtime_dir_edit_box->setText( prefix + m_runtime_dir_start.mid(2) ) ;
			m_other_dir_changed = false ;
		}
		// moot...
		else if( s.length() > 3 && s.at(1U) == ':' && s.at(2U) == '\\' &&
			orig.length() > 3 && orig.at(1U) == ':' && s.at(2U) == '\\' &&
			s.mid(3).endsWith(orig.mid(3)) )
		{
			QString prefix = s.mid( 0 , s.length() - m_install_dir_start.length() + 3 ) ;
			m_spool_dir_edit_box->setText( prefix + m_spool_dir_start.mid(3) ) ;
			m_config_dir_edit_box->setText( prefix + m_config_dir_start.mid(3) ) ;
			m_runtime_dir_edit_box->setText( prefix + m_runtime_dir_start.mid(3) ) ;
			m_other_dir_changed = false ;
		}
	}
}

void DirectoryPage::browseInstall()
{
	QString s = browse(m_install_dir_edit_box->text()) ;
	if( ! s.trimmed().isEmpty() )
		m_install_dir_edit_box->setText( s ) ;
}

void DirectoryPage::browseSpool()
{
	QString s = browse(m_spool_dir_edit_box->text()) ;
	if( ! s.trimmed().isEmpty() )
		m_spool_dir_edit_box->setText( s ) ;
}

void DirectoryPage::browseConfig()
{
	QString s = browse(m_config_dir_edit_box->text()) ;
	if( ! s.trimmed().isEmpty() )
		m_config_dir_edit_box->setText( s ) ;
}

void DirectoryPage::browseRuntime()
{
	QString s = browse(m_runtime_dir_edit_box->text()) ;
	if( ! s.trimmed().isEmpty() )
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
	// make relative paths relative to the home directory, or
	// leave them as relative to the bundle contents on mac

	G::Path result = dir ;
	if( dir.isRelative() && !Gui::Dir::home().empty() )
	{
		if( dir.str() == "~" || dir.str() == "~/" || dir.str() == "$HOME" || dir.str() == "$HOME/" )
		{
			result = Gui::Dir::home() ;
		}
		else if( dir.str().find("~/") == 0U )
		{
			result = G::Path( Gui::Dir::home() , dir.str().substr(2U) ) ;
		}
		else if( dir.str().find("$HOME/") == 0U )
		{
			result = G::Path( Gui::Dir::home() , dir.str().substr(6U) ) ;
		}
		else if( !m_is_mac )
		{
			result = G::Path::join( Gui::Dir::home() , dir ) ;
		}
	}
	return result ;
}

void DirectoryPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "dir-install" , installDir() ) ;
	dumpItem( stream , for_install , "dir-spool" , spoolDir() ) ;
	dumpItem( stream , for_install , "dir-config" , configDir() ) ;
	dumpItem( stream , for_install , "dir-run" , runtimeDir() ) ;

	dumpItem( stream , for_install , "dir-desktop" , Gui::Dir::desktop() ) ;
	dumpItem( stream , for_install , "dir-menu" , Gui::Dir::menu() ) ;
	dumpItem( stream , for_install , "dir-login" , Gui::Dir::autostart() ) ;
}

bool DirectoryPage::isComplete()
{
	return
		!m_install_dir_edit_box->text().trimmed().isEmpty() &&
		!m_spool_dir_edit_box->text().trimmed().isEmpty() &&
		!m_config_dir_edit_box->text().trimmed().isEmpty() ;
}

G::Path DirectoryPage::installDir() const
{
	return normalise( value_path(m_install_dir_edit_box) ) ;
}

G::Path DirectoryPage::spoolDir() const
{
	return normalise( value_path(m_spool_dir_edit_box) ) ;
}

G::Path DirectoryPage::runtimeDir() const
{
	return normalise( value_path(m_runtime_dir_edit_box) ) ;
}

G::Path DirectoryPage::configDir() const
{
	return normalise( value_path(m_config_dir_edit_box) ) ;
}

// ==

DoWhatPage::DoWhatPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) :
		Gui::Page(dialog,name,next_1,next_2)
{
	m_pop_checkbox = new QCheckBox( tr("POP3 server") ) ;
	m_smtp_checkbox = new QCheckBox( tr("SMTP server") ) ;

	m_smtp_checkbox->setChecked( !config.booleanValue("no-smtp",false) ) ;
	m_pop_checkbox->setChecked( config.booleanValue("pop",false) ) ;

	auto * server_type_box_layout = new QVBoxLayout ;
	server_type_box_layout->addWidget( m_pop_checkbox ) ;
	server_type_box_layout->addWidget( m_smtp_checkbox ) ;

	//: group label for pop3/smtp check boxes
	QGroupBox * server_type_group = new QGroupBox( tr("Server") ) ;
	server_type_group->setLayout( server_type_box_layout ) ;

	//: forwarding checkbox: forward emails as they are received
	m_immediate_checkbox = new QRadioButton( tr("Synchronously") ) ;
	tip( m_immediate_checkbox , tr("--immediate") ) ;
	//: forwarding checkbox: forward emails when the client disconnects
	m_on_disconnect_checkbox = new QRadioButton( tr("When client disconnects") ) ;
	tip( m_on_disconnect_checkbox , tr("--forward-on-disconnect") ) ;
	//: forwarding checkbox: forward emails from time to time
	m_periodically_checkbox = new QRadioButton( tr("Check periodically") ) ;
	tip( m_periodically_checkbox , tr("--poll") ) ;
	//: forwarding checkbox: forward emails when requested via the admin interface
	m_on_demand_checkbox = new QRadioButton( tr("Only on demand") ) ;
	tip( m_on_demand_checkbox , tr("--admin") ) ;

	if( config.booleanValue("immediate",false) )
		m_immediate_checkbox->setChecked( true ) ;
	else if( config.booleanValue("forward-on-disconnect",false) || config.numericValue("poll",99U) == 0U )
		m_on_disconnect_checkbox->setChecked( true ) ;
	else if( config.numericValue("poll",0U) != 0U )
		m_periodically_checkbox->setChecked( true ) ;
	else
		m_on_demand_checkbox->setChecked( true ) ;

	//: periodic forwarding: 'check periodically' (above) 'every' 'second/minute/hour' (below)
	QLabel * period_label = new QLabel( tr("every") ) ;
	m_period_combo = new QComboBox ;
	m_period_combo->addItem( tr("second") ) ;
	m_period_combo->addItem( tr("minute") ) ;
	m_period_combo->addItem( tr("hour") ) ;
	if( config.numericValue("poll",3600U) < 10U )
		m_period_combo->setCurrentIndex( 0 ) ; // 1s
	else if( config.numericValue("poll",3600U) < 300U )
		m_period_combo->setCurrentIndex( 1 ) ; // 1min
	else
		m_period_combo->setCurrentIndex( 2 ) ; // 1hr
	m_period_combo->setEditable( false ) ;
	period_label->setBuddy( m_period_combo ) ;

	auto * forwarding_box_layout = new QVBoxLayout ;
	forwarding_box_layout->addWidget( m_immediate_checkbox ) ;
	forwarding_box_layout->addWidget( m_on_disconnect_checkbox ) ;
	{
		auto * inner = new QHBoxLayout ;
		inner->addWidget( m_periodically_checkbox ) ;
		inner->addWidget( period_label ) ;
		inner->addWidget( m_period_combo ) ;
		forwarding_box_layout->addLayout( inner ) ;
	}
	forwarding_box_layout->addWidget( m_on_demand_checkbox ) ;

	m_forwarding_group = new QGroupBox( tr("Mail forwarding") ) ;
	m_forwarding_group->setLayout( forwarding_box_layout ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of install-type page
	layout->addWidget( newTitle(tr("Installation type")) ) ;
	layout->addWidget( server_type_group ) ;
	layout->addWidget( m_forwarding_group ) ;
	layout->addStretch() ;
	setLayout(layout);

	connect( m_pop_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_smtp_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_smtp_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_on_disconnect_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_periodically_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;

	onToggle() ;
}

void DoWhatPage::onToggle()
{
	m_period_combo->setEnabled( m_smtp_checkbox->isChecked() && m_periodically_checkbox->isChecked() ) ;
	m_forwarding_group->setEnabled( m_smtp_checkbox->isChecked() ) ;
}

std::string DoWhatPage::nextPage()
{
	// sneaky feature - see PopPage::nextPage()
	if( dialog().currentPageName() != name() )
		return m_smtp_checkbox->isChecked() ? next2() : std::string() ;

	return
		m_pop_checkbox->isChecked() ?
			next1() :
			next2() ;
}

bool DoWhatPage::pop() const
{
	return m_pop_checkbox->isChecked() ;
}

void DoWhatPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "do-pop" , value_yn(m_pop_checkbox) ) ;
	dumpItem( stream , for_install , "do-smtp" , value_yn(m_smtp_checkbox) ) ;
	dumpItem( stream , for_install , "forward-immediate" , value_yn(m_immediate_checkbox) ) ;
	dumpItem( stream , for_install , "forward-on-disconnect" , value_yn(m_on_disconnect_checkbox) ) ;
	dumpItem( stream , for_install , "forward-poll" , value_yn(m_periodically_checkbox) ) ;
	auto index = m_period_combo->currentIndex() ;
	unsigned int period = index == 0 ? 1U : ( index == 1 ? 60U : 3600U ) ;
	dumpItem( stream , for_install , "forward-poll-period" , std::to_string(period) ) ;
}

bool DoWhatPage::isComplete()
{
	return
		m_pop_checkbox->isChecked() ||
		m_smtp_checkbox->isChecked() ;
}

// ==

PopPage::PopPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ,
	bool have_accounts ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_have_accounts(have_accounts)
{
	//: internet address, port number
	QLabel * port_label = new QLabel( tr("Port:") ) ;
	std::string port_value = testMode() ? std::string("10110") : config.value( "pop-port" , "110" ) ;
	m_port_edit_box = new QLineEdit( qstr(port_value) ) ;
	tip( m_port_edit_box , tr("--pop-port") ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	auto * server_layout = new QHBoxLayout ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;

	//: group label for port number edit box
	QGroupBox * server_group = new QGroupBox( tr("Local server") ) ;
	server_group->setLayout( server_layout ) ;

	//: how pop clients will access spooled emails...
	m_one = new QRadioButton( tr("One client") ) ;
	m_shared = new QRadioButton( tr("Many clients sharing a spool directory") ) ;
	m_pop_by_name = new QRadioButton( tr("Many clients with separate spool directories") ) ;
	tip( m_pop_by_name , tr("--pop-by-name") ) ;

	m_no_delete_checkbox = new QCheckBox( tr("Disable message deletion") ) ;
	tip( m_no_delete_checkbox , tr("--pop-no-delete") ) ;
	//: copy incoming email messages to all pop clients
	m_pop_filter_copy_checkbox = new QCheckBox( tr("Copy SMTP messages to all") ) ;
	tip( m_pop_filter_copy_checkbox , tr("--filter=copy:pop") ) ;

	auto * type_layout = new QGridLayout ;
	type_layout->addWidget( m_one , 0 , 0 ) ;
	type_layout->addWidget( m_shared , 1 , 0 ) ;
	type_layout->addWidget( m_no_delete_checkbox , 1 , 1 ) ;
	type_layout->addWidget( m_pop_by_name , 2 , 0 ) ;
	type_layout->addWidget( m_pop_filter_copy_checkbox , 2 , 1 ) ;

	bool pop_by_name = config.booleanValue("pop-by-name",false) ;
	bool pop_no_delete = config.booleanValue("pop-no-delete",false) ;
	bool pop_filter_copy =
		config.value("filter").find("emailrelay-filter-copy") != std::string::npos ||
		config.value("filter").find("copy:") != std::string::npos ;
	if( pop_by_name ) // "many clients with separate spool directories"
	{
		m_pop_by_name->setChecked( true ) ;
		m_pop_filter_copy_checkbox->setChecked( pop_filter_copy ) ;
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

	//: group label for username/password edit-box pairs
	QGroupBox * type_group = new QGroupBox( tr("Client accounts") ) ;
	type_group->setLayout( type_layout ) ;

	auto * accounts_layout = new QGridLayout ;
	//: pop account, username
	QLabel * name_label = new QLabel( tr("Name:") ) ;
	//: pop account, password
	QLabel * pwd_label = new QLabel( tr("Password:") ) ;
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
	accounts_layout->addWidget( name_label , 0 , 0 ) ;
	accounts_layout->addWidget( pwd_label , 0 , 1 ) ;
	accounts_layout->addWidget( m_name_1 , 1 , 0 ) ;
	accounts_layout->addWidget( m_pwd_1 , 1 , 1 ) ;
	accounts_layout->addWidget( m_name_2 , 2 , 0 ) ;
	accounts_layout->addWidget( m_pwd_2 , 2 , 1 ) ;
	accounts_layout->addWidget( m_name_3 , 3 , 0 ) ;
	accounts_layout->addWidget( m_pwd_3 , 3 , 1 ) ;

	if( testMode() && !have_accounts )
	{
		m_name_1->setText( "me" ) ;
		m_pwd_1->setText( "secret" ) ;
	}

	QGroupBox * accounts_group =
		m_have_accounts ?
			//: group label for username/password edit boxes when installing
			new QGroupBox( tr("New Accounts") ) :
			//: group label for username/password edit boxes when reconfiguring
			new QGroupBox( tr("Accounts") ) ;
	accounts_group->setLayout( accounts_layout ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of pop-server page
	layout->addWidget( newTitle(tr("POP server")) ) ;
	layout->addWidget( server_group ) ;
	layout->addWidget( type_group ) ;
	layout->addWidget( accounts_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_one , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_shared , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_pop_by_name , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;

	connect( m_name_1 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_pwd_1 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_name_2 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_pwd_2 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_name_3 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_pwd_3 , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;

	onToggle() ;
}

std::string PopPage::nextPage()
{
	// the next page is normally the smtp page but only the dowhat page
	// knows whether we should do smtp -- a special	feature of the dowhat
	// page's nextPage() is that if it detects that it is not the current
	// page (ie. if it's called from here) then it will give us an empty
	// string if no smtp page is required

	return
		dialog().previousPage(1U).nextPage().empty() ?
			next2() :
			next1() ;
}

void PopPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "pop-port" , value_number(m_port_edit_box) ) ;
	dumpItem( stream , for_install , "pop-simple" , value_yn(m_one) ) ;
	dumpItem( stream , for_install , "pop-shared" , value_yn(m_shared) ) ;
	dumpItem( stream , for_install , "pop-shared-no-delete" , value_yn(m_no_delete_checkbox) ) ;
	dumpItem( stream , for_install , "pop-by-name" , value_yn(m_pop_by_name) ) ;

	dumpItem( stream , for_install , "pop-auth-mechanism" , std::string("plain") ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "pop-account-1-name" , G::Base64::encode(value_utf8(m_name_1)) ) ;
		dumpItem( stream , for_install , "pop-account-1-password" , G::Base64::encode(value_utf8(m_pwd_1)) ) ;
		dumpItem( stream , for_install , "pop-account-2-name" , G::Base64::encode(value_utf8(m_name_2)) ) ;
		dumpItem( stream , for_install , "pop-account-2-password" , G::Base64::encode(value_utf8(m_pwd_2)) ) ;
		dumpItem( stream , for_install , "pop-account-3-name" , G::Base64::encode(value_utf8(m_name_3)) ) ;
		dumpItem( stream , for_install , "pop-account-3-password" , G::Base64::encode(value_utf8(m_pwd_3)) ) ;
	}
}

bool PopPage::isComplete()
{
	return
		! m_port_edit_box->text().trimmed().isEmpty() && (
			m_have_accounts ||
			( !m_name_1->text().trimmed().isEmpty() && !m_pwd_1->text().trimmed().isEmpty() ) ||
			( !m_name_2->text().trimmed().isEmpty() && !m_pwd_2->text().trimmed().isEmpty() ) ||
			( !m_name_3->text().trimmed().isEmpty() && !m_pwd_3->text().trimmed().isEmpty() ) ) ;
}

void PopPage::onToggle()
{
	m_no_delete_checkbox->setEnabled( m_shared->isChecked() ) ;
	m_pop_filter_copy_checkbox->setEnabled( m_pop_by_name->isChecked() ) ;
}

bool PopPage::withFilterCopy() const
{
	return m_pop_filter_copy_checkbox->isChecked() ;
}

// ==

SmtpServerPage::SmtpServerPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ,
	bool have_account , bool can_generate ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_have_account(have_account) ,
		m_can_generate(can_generate)
{
	//: internet address, port number
	QLabel * port_label = new QLabel( tr("Port:") ) ;
	std::string port_value = testMode() ? std::string("10025") : config.value( "port" , "25" ) ;
	m_port_edit_box = new QLineEdit( qstr(port_value) ) ;
	tip( m_port_edit_box , tr("--port") ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	auto * server_layout = new QHBoxLayout ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;

	//: group label for port number edit box
	QGroupBox * server_group = new QGroupBox( tr("Local server") ) ;
	server_group->setLayout( server_layout ) ;

	//

	//: smtp server requires authentication
	m_auth_checkbox = new QCheckBox( tr("Require authentication") ) ;
	tip( m_auth_checkbox , tr("--server-auth") ) ;
	m_auth_checkbox->setChecked( config.contains("server-auth") ) ;

	auto * auth_layout = new QVBoxLayout ;
	auth_layout->addWidget( m_auth_checkbox ) ;

	//: group label for 'require authentication' check box
	QGroupBox * auth_group = new QGroupBox( tr("Authentication") ) ;
	auth_group->setLayout( auth_layout ) ;

	//

	//: smtp server account, username
	QLabel * account_name_label = new QLabel( tr("Name:") ) ;
	m_account_name = new QLineEdit ;
	tip( m_account_name , NameTip() ) ;
	account_name_label->setBuddy( m_account_name ) ;

	//: smtp server account, password
	QLabel * account_pwd_label = new QLabel( tr("Password:") ) ;
	m_account_pwd = new QLineEdit ;
	tip( m_account_pwd , PasswordTip() ) ;
	m_account_pwd->setEchoMode( QLineEdit::Password ) ;
	account_pwd_label->setBuddy( m_account_pwd ) ;

	if( testMode() && !have_account )
	{
		m_auth_checkbox->setChecked( true ) ;
		m_account_name->setText( "me" ) ;
		m_account_pwd->setText( "secret" ) ;
	}

	auto * account_layout = new QGridLayout ;
	account_layout->addWidget( account_name_label , 0 , 0 ) ;
	account_layout->addWidget( m_account_name , 0 , 1 ) ;
	account_layout->addWidget( account_pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_account_pwd , 1 , 1 ) ;

	m_account_group =
		m_have_account ?
			//: group label for username/password edit box when installing
			new QGroupBox( tr("New Account") ) :
			//: group label for username/password edit box when reconfiguring
			new QGroupBox( tr("Account") ) ;
	m_account_group->setLayout( account_layout ) ;

	//

	bool with_trust = false ; // too many widgets for a small screen
	if( with_trust )
	{
		QLabel * trust_label = new QLabel( tr("IP address:") ) ;
		m_trust_address = new QLineEdit ;
		trust_label->setBuddy( m_trust_address ) ;
		tip( m_trust_address , tr("eg. 192.168.0.0/8") ) ;
		m_trust_group = new QGroupBox( tr("Exemptions") ) ;
		auto * trust_layout = new QHBoxLayout ;
		trust_layout->addWidget( trust_label ) ;
		trust_layout->addWidget( m_trust_address ) ;
		m_trust_group->setLayout( trust_layout ) ;
	}
	else
	{
		m_trust_address = nullptr ;
		m_trust_group = nullptr ;
	}

	//

	//: group box label for encryption options
	QGroupBox * tls_group = new QGroupBox( tr("TLS encryption") ) ;

	m_tls_checkbox = new QCheckBox( tr("Enable TLS encryption") ) ;
	tip( m_tls_checkbox , tr("--server-tls, --server-tls-connection") ) ;
	//: not translatable, see RFC-2487
	m_tls_starttls = new QRadioButton( tr("STARTTLS")) ;
	tip( m_tls_starttls , tr("--server-tls") ) ;
	//: 'implicit' because encryption is assumed to be always active, see RFC-8314 3.
	m_tls_tunnel = new QRadioButton( tr("Implicit TLS (smtps)") ) ;
	tip( m_tls_tunnel , tr("--server-tls-connection") ) ;
	auto * tls_innermost_layout = new QHBoxLayout ;
	tls_innermost_layout->addWidget( m_tls_checkbox ) ;
	tls_innermost_layout->addWidget( m_tls_starttls ) ;
	tls_innermost_layout->addWidget( m_tls_tunnel ) ;

	//: X.509 certificate
	m_tls_certificate_label = new QLabel( tr("Certificate:") ) ;
	m_tls_certificate_edit_box = new QLineEdit ;
	tip( m_tls_certificate_edit_box , tr("--server-tls-certificate") ) ;
	m_tls_certificate_label->setBuddy( m_tls_certificate_edit_box ) ;
	//: activate a file-open dialog box to select a file
	m_tls_browse_button = new QPushButton( tr("Browse") ) ;
	auto * tls_layout = new QVBoxLayout ;
	auto * tls_inner_layout = new QHBoxLayout ;
	tls_inner_layout->addWidget( m_tls_certificate_label ) ;
	tls_inner_layout->addWidget( m_tls_certificate_edit_box ) ;
	tls_inner_layout->addWidget( m_tls_browse_button ) ;
	tls_layout->addLayout( tls_innermost_layout ) ;
	tls_layout->addLayout( tls_inner_layout ) ;
	tls_group->setLayout( tls_layout ) ;

	m_tls_checkbox->setChecked(
		config.booleanValue("server-tls",false) ||
		config.booleanValue("server-tls-connection",false) ) ;
	m_tls_starttls->setChecked( !config.booleanValue("server-tls-connection",false) ) ;
	m_tls_tunnel->setChecked( config.booleanValue("server-tls-connection",false) ) ;
	m_tls_certificate_edit_box->setText( qstr(config.value("server-tls-certificate")) ) ;

	//

	auto * layout = new QVBoxLayout ;
	//: page title of smtp-server page
	layout->addWidget( newTitle(tr("SMTP server")) ) ;
	layout->addWidget( server_group ) ;
	layout->addWidget( auth_group ) ;
	layout->addWidget( m_account_group ) ;
	if( m_trust_group )
		layout->addWidget( m_trust_group ) ;
	layout->addWidget( tls_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_name , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_tls_certificate_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_pwd , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_tls_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_tls_checkbox , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_tls_browse_button , SIGNAL(clicked()) , this , SLOT(browseCertificate()) ) ;
	if( m_trust_address )
		connect( m_trust_address , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;

	onToggle() ;
}

void SmtpServerPage::browseCertificate()
{
	QString s = browse( m_tls_certificate_edit_box->text() ) ;
	if( ! s.trimmed().isEmpty() )
		m_tls_certificate_edit_box->setText( s ) ;
}

QString SmtpServerPage::browse( QString /*ignored*/ )
{
	return QFileDialog::getOpenFileName( this ) ;
}

std::string SmtpServerPage::nextPage()
{
	return next1() ;
}

void SmtpServerPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "smtp-server-port" , value_number(m_port_edit_box) ) ;
	dumpItem( stream , for_install , "smtp-server-auth" , value_yn(m_auth_checkbox) ) ;
	dumpItem( stream , for_install , "smtp-server-auth-mechanism" , std::string("plain") ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "smtp-server-account-name" , G::Base64::encode(value_utf8(m_account_name)) ) ;
		dumpItem( stream , for_install , "smtp-server-account-password" , G::Base64::encode(value_utf8(m_account_pwd)) ) ;
	}
	dumpItem( stream , for_install , "smtp-server-trust" , value_utf8(m_trust_address) ) ;
	dumpItem( stream , for_install , "smtp-server-tls" , value_yn(m_tls_checkbox->isChecked() && m_tls_starttls->isChecked()) ) ;
	dumpItem( stream , for_install , "smtp-server-tls-connection" , value_yn(m_tls_checkbox->isChecked() && m_tls_tunnel->isChecked()) ) ;
	dumpItem( stream , for_install , "smtp-server-tls-certificate" , value_path(m_tls_checkbox->isChecked()?m_tls_certificate_edit_box:nullptr) ) ;
}

void SmtpServerPage::onToggle()
{
	m_account_group->setEnabled( m_auth_checkbox->isChecked() ) ;
	m_tls_starttls->setEnabled( m_tls_checkbox->isChecked() ) ;
	m_tls_tunnel->setEnabled( m_tls_checkbox->isChecked() ) ;
	m_tls_certificate_label->setEnabled( m_tls_checkbox->isChecked() ) ;
	m_tls_certificate_edit_box->setEnabled( m_tls_checkbox->isChecked() ) ;
	m_tls_browse_button->setEnabled( m_tls_checkbox->isChecked() ) ;
	if( m_trust_group )
		m_trust_group->setEnabled( m_auth_checkbox->isChecked() ) ;
}

bool SmtpServerPage::isComplete()
{
	return
		! m_port_edit_box->text().trimmed().isEmpty() &&
		(
			!m_tls_checkbox->isChecked() ||
			m_can_generate ||
			!m_tls_certificate_edit_box->text().trimmed().isEmpty()
		) &&
		(
			m_have_account ||
			!m_auth_checkbox->isChecked() || (
				! m_account_name->text().trimmed().isEmpty() &&
				! m_account_pwd->text().trimmed().isEmpty() )
		) ;
}

// ==

FilterPage::FilterPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ,
	bool installing , bool is_windows ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_installing(installing) ,
		m_is_windows(is_windows) ,
		m_dot_exe(is_windows?".exe":"") ,
		m_dot_script(is_windows?".js":".sh") ,
		m_first_show(true) ,
		m_pop_page_with_filter_copy(false) ,
		m_server_filter_spam_default("spam-edit:127.0.0.1:783")
{
	//: label for an edit box that contains the filename of a server-side filter script
	m_server_filter_label = new QLabel( tr("Filter:") ) ;
	m_server_filter_edit_box = new QLineEdit ;
	m_server_filter_label->setBuddy( m_server_filter_edit_box ) ;

	//: server-side filtering options...
	m_server_filter_choice_none = new QRadioButton( tr("None") ) ;
	//: run the specified filter script
	m_server_filter_choice_script = new QRadioButton( tr("Script") ) ;
	//: use the spamassassin 'spamd' daemon
	m_server_filter_choice_spamd = new QRadioButton( tr("Spamd") ) ;
	//: copy emails into directories for multiple pop clients
	m_server_filter_choice_copy = new QRadioButton( tr("Copy") ) ;

	auto * filter_choice_layout = new QVBoxLayout ;
	filter_choice_layout->addWidget( m_server_filter_choice_none ) ;
	filter_choice_layout->addWidget( m_server_filter_choice_script ) ;
	filter_choice_layout->addWidget( m_server_filter_choice_spamd ) ;
	filter_choice_layout->addWidget( m_server_filter_choice_copy ) ;

	//: client-side filtering options: none or script
	m_client_filter_choice_none = new QRadioButton( tr("None") ) ;
	m_client_filter_choice_script = new QRadioButton( tr("Script") ) ;
	tip( m_client_filter_choice_script , tr("--client-filter") ) ;

	auto * client_filter_choice_layout = new QVBoxLayout ;
	client_filter_choice_layout->addWidget( m_client_filter_choice_none ) ;
	client_filter_choice_layout->addWidget( m_client_filter_choice_script ) ;

	//: label for an edit box that contains the filename of a client-side filter script
	m_client_filter_label = new QLabel( tr("Filter:") ) ;
	m_client_filter_edit_box = new QLineEdit ;
	m_client_filter_label->setBuddy( m_client_filter_edit_box ) ;

	auto * script_layout = new QHBoxLayout ;
	script_layout->addWidget( m_server_filter_label ) ;
	script_layout->addWidget( m_server_filter_edit_box ) ;

	auto * client_script_layout = new QHBoxLayout ;
	client_script_layout->addWidget( m_client_filter_label ) ;
	client_script_layout->addWidget( m_client_filter_edit_box ) ;

	auto * server_layout = new QVBoxLayout ;
	server_layout->addLayout( filter_choice_layout ) ;
	server_layout->addLayout( script_layout ) ;

	//: group label for server-side filtering options
	QGroupBox * server_group = new QGroupBox( tr("Server") ) ;
	server_group->setLayout( server_layout ) ;

	auto * client_layout = new QVBoxLayout ;
	client_layout->addLayout( client_filter_choice_layout ) ;
	client_layout->addLayout( client_script_layout ) ;

	//: group label for client-side filtering options
	QGroupBox * client_group = new QGroupBox( tr("Client") ) ;
	client_group->setLayout( client_layout ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of filters page
	layout->addWidget( newTitle(tr("Filters")) ) ;
	layout->addWidget( server_group ) ;
	layout->addWidget( client_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	// directories are fixed by the first page, so keep the paths locked down
	m_server_filter_edit_box->setEnabled( false ) ;
	m_client_filter_edit_box->setEnabled( false ) ;

	connect( m_server_filter_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_server_filter_choice_none , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
	connect( m_server_filter_choice_none , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_server_filter_choice_script , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
	connect( m_server_filter_choice_script , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_server_filter_choice_spamd , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
	connect( m_server_filter_choice_spamd , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_server_filter_choice_copy , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
	connect( m_server_filter_choice_copy , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;

	connect( m_client_filter_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_client_filter_choice_script , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
	connect( m_client_filter_choice_script , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;

	m_server_filter = config.value( "filter" ) ;
	m_client_filter = config.value( "client-filter" ) ;
	m_first_show = true ;

	m_server_filter_choice_none->setChecked( true ) ;
	m_client_filter_choice_none->setChecked( true ) ;

	//onToggle() ;
}

std::string FilterPage::nextPage()
{
	return next1() ;
}

void FilterPage::onShow( bool )
{
	PopPage & pop_page = dynamic_cast<PopPage&>( dialog().page("pop") ) ;
	DoWhatPage & do_what_page = dynamic_cast<DoWhatPage&>( dialog().page("dowhat") ) ;
	DirectoryPage & dir_page = dynamic_cast<DirectoryPage&>( dialog().page("directory") ) ;

	G::Path script_dir = m_is_windows ? dir_page.configDir() : ( dir_page.installDir() / "lib" / "emailrelay" ) ;
	G::Path exe_dir = m_is_windows ? dir_page.installDir() : ( dir_page.installDir() / "lib" / "emailrelay" ) ;

	m_server_filter_script_path_default = script_dir / ("emailrelay-filter"+m_dot_script) ;
	m_server_filter_copy_default = "copy:pop" ;
	m_client_filter_script_path_default = script_dir / ("emailrelay-client-filter"+m_dot_script) ;
	m_pop_page_with_filter_copy = do_what_page.pop() && pop_page.withFilterCopy() ;

	if( m_pop_page_with_filter_copy )
	{
		m_server_filter_choice_none->setEnabled( false ) ;
		m_server_filter_choice_script->setEnabled( false ) ;
		m_server_filter_choice_spamd->setEnabled( false ) ;
		m_server_filter_choice_copy->setEnabled( false ) ;
		//: the edit boxes are disabled because of what was selected on the pop-server page
		QString tooltip = tr("see pop server page") ;
		tip( m_server_filter_choice_none , tooltip ) ;
		tip( m_server_filter_choice_script , tooltip ) ;
		tip( m_server_filter_choice_spamd , tooltip ) ;
		tip( m_server_filter_choice_copy , tooltip ) ;
	}
	else
	{
		m_server_filter_choice_none->setEnabled( true ) ;
		m_server_filter_choice_script->setEnabled( true ) ;
		m_server_filter_choice_spamd->setEnabled( true ) ;
		m_server_filter_choice_copy->setEnabled( true ) ;
		tip( m_server_filter_choice_script , tr("--filter:file") ) ;
		tip( m_server_filter_choice_spamd , tr("--filter:spam-edit") ) ;
		tip( m_server_filter_choice_copy , tr("--filter:copy") ) ;
	}

	if( m_installing )
	{
		// if installing the the directories can change on each show
		// and there is no existing config to preserve
		m_server_filter_script_path = m_server_filter_script_path_default ;
		m_server_filter_copy = m_server_filter_copy_default ;
		m_server_filter_spam = m_server_filter_spam_default ;
		m_client_filter_script_path = m_client_filter_script_path_default ;
	}
	else if( m_first_show )
	{
		// if reconfiguring then set the initial checkboxes from the configuration
		// value, unless overridden by the pop page (below)
		m_server_filter_script_path = m_server_filter_script_path_default ;
		m_server_filter_copy = m_server_filter_copy_default ;
		m_server_filter_spam = m_server_filter_spam_default ;
		if( m_server_filter.empty() )
		{
			m_server_filter_choice_none->setChecked( true ) ;
		}
		else if( m_server_filter.find("spam:") == 0U || m_server_filter.find("spam-edit:") == 0U )
		{
			m_server_filter_choice_spamd->setChecked( true ) ;
			m_server_filter_spam = m_server_filter ;
		}
		else if( m_server_filter.find("emailrelay-filter-copy") != std::string::npos ||
			m_server_filter.find("copy:") != std::string::npos )
		{
			m_server_filter_choice_copy->setChecked( true ) ;
			m_server_filter_copy = m_server_filter ;
		}
		else
		{
			m_server_filter_choice_script->setChecked( true ) ;
			m_server_filter_script_path = m_server_filter ;
		}

		m_client_filter_script_path = m_client_filter_script_path_default ;
		if( m_client_filter.empty() )
		{
			m_client_filter_choice_none->setChecked( true ) ;
		}
		else
		{
			m_client_filter_choice_script->setChecked( true ) ;
			m_client_filter_script_path = m_client_filter ;
		}
	}

	if( m_pop_page_with_filter_copy )
	{
		m_server_filter_choice_copy->setChecked( true ) ;
	}

	m_first_show = false ;
	onToggle() ;
}

void FilterPage::onToggle()
{
	if( m_server_filter_choice_none->isChecked() )
	{
		m_server_filter_edit_box->setText( qstr("") ) ;
	}
	else if( m_server_filter_choice_script->isChecked() )
	{
		m_server_filter_edit_box->setText( qstr(m_server_filter_script_path.str()) ) ;
	}
	else if( m_server_filter_choice_spamd->isChecked() )
	{
		m_server_filter_edit_box->setText( qstr(m_server_filter_spam) ) ;
	}
	else if( m_server_filter_choice_copy->isChecked() )
	{
		m_server_filter_edit_box->setText( qstr(m_server_filter_copy.str()) ) ;
	}

	if( m_client_filter_choice_none->isChecked() )
	{
		m_client_filter_edit_box->setText( qstr("") ) ;
	}
	else if( m_client_filter_choice_script->isChecked() )
	{
		m_client_filter_edit_box->setText( qstr(m_client_filter_script_path.str()) ) ;
	}
}

void FilterPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "filter-server" , value_path(m_server_filter_edit_box) ) ;
	dumpItem( stream , for_install , "filter-client" , value_path(m_client_filter_edit_box) ) ;
}

// ==

SmtpClientPage::SmtpClientPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool have_account ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_have_account(have_account)
{
	//: internet address, hostname of remote smtp server
	QLabel * server_label = new QLabel( tr("Hostname:") ) ;
	m_server_edit_box = new QLineEdit ;
	server_label->setBuddy( m_server_edit_box ) ;

	tip( m_server_edit_box , tr("--forward-to") ) ;
	std::string address = config.value("forward-to") ;
	address = address.empty() ? config.value("as-client") : address ;
	address = address.empty() ? std::string("smtp.example.com:25") : address ;
	std::string net_address = G::Str::head( address , address.find_last_of(".:") , std::string() ) ;
	std::string port = G::Str::tail( address , address.find_last_of(".:") , std::string() ) ;
	m_server_edit_box->setText( qstr(net_address) ) ;

	//: internet address, port number of remote smtp server
	QLabel * port_label = new QLabel( tr("Port:") ) ;
	m_port_edit_box = new QLineEdit( qstr(port) ) ;
	port_label->setBuddy( m_port_edit_box ) ;

	auto * server_layout = new QHBoxLayout ;
	server_layout->addWidget( server_label ) ;
	server_layout->addWidget( m_server_edit_box ) ;
	server_layout->addWidget( port_label ) ;
	server_layout->addWidget( m_port_edit_box ) ;
	server_layout->setStretchFactor( m_server_edit_box , 4 ) ;

	QGroupBox * server_group = new QGroupBox( tr("Remote server") ) ;
	server_group->setLayout( server_layout ) ;

	m_tls_checkbox = new QCheckBox( tr("Use TLS encryption") ) ;
	tip( m_tls_checkbox , tr("--client-tls, --client-tls-connection") ) ;
	const bool config_tls = config.booleanValue( "client-tls" , false ) ;
	const bool config_tls_connection = config.booleanValue( "client-tls-connection" , false ) ;
	m_tls_checkbox->setChecked( config_tls || config_tls_connection ) ;
	//: not translatable, see RFC-2487
	m_tls_starttls = new QRadioButton( tr("STARTTLS")) ;
	m_tls_starttls->setChecked( !config_tls_connection ) ;
	tip( m_tls_starttls , tr("--client-tls") ) ;
	//: 'implicit' because encryption is assumed to be always active, see RFC-8314 3.
	m_tls_tunnel = new QRadioButton( tr("Implicit TLS (smtps)") ) ;
	m_tls_tunnel->setChecked( config_tls_connection ) ;
	tip( m_tls_tunnel , tr("--client-tls-connection") ) ;

	auto * tls_layout = new QHBoxLayout ;
	tls_layout->addWidget( m_tls_checkbox ) ;
	tls_layout->addWidget( m_tls_starttls ) ;
	tls_layout->addWidget( m_tls_tunnel ) ;

	QGroupBox * tls_group = new QGroupBox( tr("TLS encryption") ) ;
	tls_group->setLayout( tls_layout ) ;

	//: client should supply authentication credentials when connecting to the server
	m_auth_checkbox = new QCheckBox( tr("Supply authentication") ) ;
	m_auth_checkbox->setChecked( config.contains("client-auth") ) ;
	tip( m_auth_checkbox , tr("--client-auth") ) ;

	auto * auth_layout = new QVBoxLayout ;
	auth_layout->addWidget( m_auth_checkbox ) ;

	QGroupBox * auth_group = new QGroupBox( tr("Authentication") ) ;
	auth_group->setLayout( auth_layout ) ;

	//: smtp client account, username
	QLabel * account_name_label = new QLabel( tr("Name:") ) ;
	m_account_name = new QLineEdit ;
	tip( m_account_name , NameTip() ) ;
	account_name_label->setBuddy( m_account_name ) ;

	//: smtp client account, password
	QLabel * account_pwd_label = new QLabel( tr("Password:") ) ;
	m_account_pwd = new QLineEdit ;
	tip( m_account_pwd , PasswordTip() ) ;
	m_account_pwd->setEchoMode( QLineEdit::Password ) ;
	account_pwd_label->setBuddy( m_account_pwd ) ;

	if( testMode() && !have_account )
	{
		m_auth_checkbox->setChecked( true ) ;
		m_account_name->setText( "me" ) ;
		m_account_pwd->setText( "secret" ) ;
	}

	auto * account_layout = new QGridLayout ;
	account_layout->addWidget( account_name_label , 0 , 0 ) ;
	account_layout->addWidget( m_account_name , 0 , 1 ) ;
	account_layout->addWidget( account_pwd_label , 1 , 0 ) ;
	account_layout->addWidget( m_account_pwd , 1 , 1 ) ;

	m_account_group =
		m_have_account ?
			//: group label for username/password edit box when installing
			new QGroupBox( tr("New Account") ) :
			//: group label for username/password edit box when reconfiguring
			new QGroupBox( tr("Account") ) ;
	m_account_group->setLayout( account_layout ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of smtp-client page
	layout->addWidget( newTitle(tr("SMTP client")) ) ;
	layout->addWidget( server_group ) ;
	layout->addWidget( auth_group ) ;
	layout->addWidget( m_account_group ) ;
	layout->addWidget( tls_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_port_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_server_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_name , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_account_pwd , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_tls_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_tls_checkbox , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_tls_starttls , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_tls_starttls , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_tls_tunnel , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_tls_tunnel , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_auth_checkbox , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;

	onToggle() ;
}

void SmtpClientPage::onToggle()
{
	m_account_group->setEnabled( m_auth_checkbox->isChecked() ) ;
	m_tls_starttls->setEnabled( m_tls_checkbox->isChecked() ) ;
	m_tls_tunnel->setEnabled( m_tls_checkbox->isChecked() ) ;
}

std::string SmtpClientPage::nextPage()
{
	return next1() ;
}

void SmtpClientPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "smtp-client-host" , value_utf8(m_server_edit_box) ) ;
	dumpItem( stream , for_install , "smtp-client-port" , value_number(m_port_edit_box) ) ;
	dumpItem( stream , for_install , "smtp-client-tls" , value_yn(m_tls_checkbox->isChecked()&&!m_tls_tunnel->isChecked()) ) ;
	dumpItem( stream , for_install , "smtp-client-tls-connection" , value_yn(m_tls_checkbox->isChecked()&&m_tls_tunnel->isChecked()) ) ;
	dumpItem( stream , for_install , "smtp-client-auth" , value_yn(m_auth_checkbox) ) ;
	dumpItem( stream , for_install , "smtp-client-auth-mechanism" , std::string("plain") ) ;
	if( for_install )
	{
		dumpItem( stream , for_install , "smtp-client-account-name" , G::Base64::encode(value_utf8(m_account_name)) ) ;
		dumpItem( stream , for_install , "smtp-client-account-password" , G::Base64::encode(value_utf8(m_account_pwd)) ) ;
	}
}

bool SmtpClientPage::isComplete()
{
	return
		! m_port_edit_box->text().trimmed().isEmpty() &&
		! m_server_edit_box->text().trimmed().isEmpty() && (
		m_have_account ||
		! m_auth_checkbox->isChecked() || (
			! m_account_name->text().trimmed().isEmpty() &&
			! m_account_pwd->text().trimmed().isEmpty() ) ) ;
}

// ==

LoggingPage::LoggingPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ) :
		Gui::Page(dialog,name,next_1,next_2)
{
	m_config_log_file = config.value( "log-file" ) ;

	//: enable debug-level logging
	m_log_level_debug_checkbox = new QCheckBox( tr("Debug messages") ) ;
	tip( m_log_level_debug_checkbox , tr("--debug") ) ;
	//: enable more verbose logging
	m_log_level_verbose_checkbox = new QCheckBox( tr("Verbose logging") ) ;
	tip( m_log_level_verbose_checkbox , tr("--verbose") ) ;

	m_log_output_syslog_checkbox = new QCheckBox( tr("Write to the system log") ) ;
	tip( m_log_output_syslog_checkbox , tr("--syslog") ) ;

	m_log_output_file_checkbox = new QCheckBox( tr("Write to log file") ) ;
	m_log_output_file_checkbox->setChecked( true ) ;
	tip( m_log_output_file_checkbox , tr("--log-file") ) ;

	m_log_output_file_label = new QLabel( tr("Log file:") ) ;
	m_log_output_file_edit_box = new QLineEdit ;
	m_log_output_file_label->setBuddy( m_log_output_file_edit_box ) ;
	m_log_output_file_browse_button = new QPushButton( tr("Browse") ) ;
	m_log_output_file_browse_button->setVisible( false ) ; // moot

	m_log_fields_time_checkbox = new QCheckBox( tr("Timestamps") ) ;
	tip( m_log_fields_time_checkbox , tr("--log-format=time") ) ;

	m_log_fields_address_checkbox = new QCheckBox( tr("Network addresses") ) ;
	tip( m_log_fields_address_checkbox , tr("--log-format=address") ) ;

	m_log_fields_port_checkbox = new QCheckBox( tr("TCP ports") ) ;
	tip( m_log_fields_port_checkbox , tr("--log-format=port") ) ;

	m_log_fields_msgid_checkbox = new QCheckBox( tr("Message ids") ) ;
	tip( m_log_fields_msgid_checkbox , tr("--log-format=msgid") ) ;

	auto * log_output_file_layout = new QHBoxLayout ;
	log_output_file_layout->addWidget( m_log_output_file_label ) ;
	log_output_file_layout->addWidget( m_log_output_file_edit_box ) ;
	log_output_file_layout->addWidget( m_log_output_file_browse_button ) ;

	auto * log_level_layout = new QVBoxLayout ;
	log_level_layout->addWidget( m_log_level_verbose_checkbox ) ;
	log_level_layout->addWidget( m_log_level_debug_checkbox ) ;

	auto * log_output_layout = new QVBoxLayout ;
	log_output_layout->addWidget( m_log_output_syslog_checkbox ) ;
	log_output_layout->addWidget( m_log_output_file_checkbox ) ;
	log_output_layout->addLayout( log_output_file_layout ) ;

	auto * log_fields_layout = new QGridLayout ;
	log_fields_layout->addWidget( m_log_fields_time_checkbox , 0 , 0 ) ;
	log_fields_layout->addWidget( m_log_fields_address_checkbox , 1 , 0 ) ;
	log_fields_layout->addWidget( m_log_fields_port_checkbox , 0 , 1 ) ;
	log_fields_layout->addWidget( m_log_fields_msgid_checkbox , 1 , 1 ) ;

	bool syslog_override = config.booleanValue("syslog",false) ;
	bool as_client = config.booleanValue("as-client",false) ;
	bool no_syslog = config.booleanValue("no-syslog",false) ;
	bool syslog = syslog_override || !(as_client||no_syslog) ; // true by default

	m_log_output_syslog_checkbox->setChecked( syslog ) ;
	m_log_level_verbose_checkbox->setChecked( config.booleanValue("verbose",true) ) ; // true, because windows users
	m_log_level_debug_checkbox->setChecked( config.booleanValue("debug",false) ) ;
	m_log_level_debug_checkbox->setEnabled( config.booleanValue("debug",false) ) ; // todo, enable if debugging is built-in
	m_log_fields_time_checkbox->setChecked( config.valueContains("log-format","time") || config.booleanValue("log-time",true) ) ;
	m_log_fields_address_checkbox->setChecked( config.valueContains("log-format","address") || config.booleanValue("log-address",false) ) ;
	m_log_fields_port_checkbox->setChecked( config.valueContains("log-format","port") ) ;
	m_log_fields_msgid_checkbox->setChecked( config.valueContains("log-format","msgid") ) ;

	//: group label for the logging verbosity level
	QGroupBox * level_group = new QGroupBox( tr("Level") ) ;
	level_group->setLayout( log_level_layout ) ;

	//: group label for the logging output selection
	QGroupBox * output_group = new QGroupBox( tr("Output") ) ;
	output_group->setLayout( log_output_layout ) ;

	//: group label for the selection of additional logging information fields
	QGroupBox * fields_group = new QGroupBox( tr("Extra information") ) ;
	fields_group->setLayout( log_fields_layout ) ;

	//

	auto * layout = new QVBoxLayout ;
	//: page title of logging page
	layout->addWidget( newTitle(tr("Logging")) ) ;
	layout->addWidget(level_group) ;
	layout->addWidget(fields_group) ;
	layout->addWidget(output_group) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_log_output_file_edit_box , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;
	connect( m_log_output_file_checkbox , SIGNAL(toggled(bool)), this, SIGNAL(pageUpdateSignal()));
	connect( m_log_output_file_checkbox , SIGNAL(toggled(bool)), this, SLOT(onToggle()) ) ;
	connect( m_log_output_file_browse_button , SIGNAL(clicked()) , this , SLOT(browseLogFile()) ) ;

	onToggle() ;
}

std::string LoggingPage::nextPage()
{
	return next1() ;
}

bool LoggingPage::isComplete()
{
	G_DEBUG( "LoggingPage::isComplete: " << m_log_output_file_checkbox->isChecked() << " " << value_utf8(m_log_output_file_edit_box) ) ;
	return
		!m_log_output_file_checkbox->isChecked() ||
		!m_log_output_file_edit_box->text().trimmed().isEmpty() ;
}

void LoggingPage::browseLogFile()
{
	QString s = browse(m_log_output_file_edit_box->text()) ;
	if( ! s.trimmed().isEmpty() )
		m_log_output_file_edit_box->setText( s ) ;
}

QString LoggingPage::browse( QString /*ignored*/ )
{
	return QFileDialog::getOpenFileName( this ) ;
}

void LoggingPage::onShow( bool )
{
	// initialise after contruction because we need the directory-page state
	if( m_config_log_file.empty() )
	{
		DirectoryPage & dir_page = dynamic_cast<DirectoryPage&>( dialog().page("directory") ) ;
		G::Path default_log_file = dir_page.runtimeDir() / "emailrelay-log-%d.txt" ;
		m_log_output_file_edit_box->setText( qstr(default_log_file.str()) ) ;
	}
	else
	{
		m_log_output_file_edit_box->setText( qstr(m_config_log_file.str()) ) ;
	}

	onToggle() ;
}

void LoggingPage::onToggle()
{
	// directories are fixed by the first page, so keep everything locked down
	m_log_output_file_edit_box->setEnabled( false ) ;
	m_log_output_file_browse_button->setEnabled( false ) ;
	m_log_output_file_label->setEnabled( m_log_output_file_checkbox->isChecked() ) ;
}

void LoggingPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "logging-verbose" , value_yn(m_log_level_verbose_checkbox) ) ;
	dumpItem( stream , for_install , "logging-debug" , value_yn(m_log_level_debug_checkbox) ) ;
	dumpItem( stream , for_install , "logging-syslog" , value_yn(m_log_output_syslog_checkbox) ) ;
	dumpItem( stream , for_install , "logging-file" , value_path(m_log_output_file_checkbox->isChecked()?m_log_output_file_edit_box:nullptr) ) ;
	dumpItem( stream , for_install , "logging-time" , value_yn(m_log_fields_time_checkbox) ) ;
	dumpItem( stream , for_install , "logging-address" , value_yn(m_log_fields_address_checkbox) ) ;
	dumpItem( stream , for_install , "logging-port" , value_yn(m_log_fields_port_checkbox) ) ;
	dumpItem( stream , for_install , "logging-msgid" , value_yn(m_log_fields_msgid_checkbox) ) ;
}

// ==

ListeningPage::ListeningPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool next_is_next2 ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_next_is_next2(next_is_next2)
{
	//: server listening-address options...
	m_all_checkbox = new QRadioButton( tr("Any address") ) ;
	//: listen on any ipv4 address
	m_ipv4_checkbox = new QRadioButton( tr("Any IPv&4") ) ;
	//: listen on any ipv6 address
	m_ipv6_checkbox = new QRadioButton( tr("Any IPv&6") ) ;
	//: listen on the ipv4 and ipv6 'localhost' addresses
	m_loopback_checkbox = new QRadioButton( tr("Localhost") ) ;
	//: listen on specific addresses given in the edit-box
	m_list_checkbox = new QRadioButton( tr("List") ) ;

	m_listening_interface = new QLineEdit ;
	tip( m_listening_interface , tr("--interface") ) ;

	if( config.contains("interface") )
	{
		m_value = config.value( "interface" ) ;
		if( m_value == "0.0.0.0" )
		{
			m_ipv4_checkbox->setChecked( true ) ;
		}
		else if( m_value == "::" )
		{
			m_ipv6_checkbox->setChecked( true ) ;
		}
		else if( m_value == "127.0.0.1,::1" || m_value == "::1,127.0.0.1" )
		{
			m_loopback_checkbox->setChecked( true ) ;
		}
		else if( m_value.empty() )
		{
			m_all_checkbox->setChecked( true ) ;
		}
		else
		{
			m_list_checkbox->setChecked( true ) ;
			m_listening_interface->setEnabled( true ) ;
		}
		m_listening_interface->setText( qstr(m_value) ) ;
	}
	else
	{
		m_all_checkbox->setChecked( true ) ;
	}
	m_listening_interface->setEnabled( m_list_checkbox->isChecked() ) ;

	auto * listening_layout = new QGridLayout ;
	listening_layout->addWidget( m_all_checkbox , 0 , 0 ) ;
	listening_layout->addWidget( m_ipv4_checkbox , 1 , 0 ) ;
	listening_layout->addWidget( m_ipv6_checkbox , 2 , 0 ) ;
	listening_layout->addWidget( m_loopback_checkbox , 3 , 0 ) ;
	listening_layout->addWidget( m_list_checkbox , 4 , 0 ) ;
	listening_layout->addWidget( m_listening_interface , 4 , 1 ) ;

	//: group label for the network address that the server should listen on
	QGroupBox * listening_group = new QGroupBox( tr("Listen on") ) ;
	listening_group->setLayout( listening_layout ) ;

	//

	m_remote_checkbox = new QCheckBox( tr("Allow remote clients") ) ;
	tip( m_remote_checkbox , tr("--remote-clients") ) ;
	m_remote_checkbox->setChecked( config.booleanValue("remote-clients",false) ) ;

	auto * connections_layout = new QHBoxLayout ;
	connections_layout->addWidget( m_remote_checkbox ) ;

	//: group label for allow-remote-clients checkbox
	QGroupBox * connections_group = new QGroupBox( tr("Clients") ) ;
	connections_group->setLayout( connections_layout ) ;

	//

	auto * layout = new QVBoxLayout ;
	//: page title of connections page
	layout->addWidget( newTitle(tr("Connections")) ) ;
	layout->addWidget( listening_group ) ;
	layout->addWidget( connections_group ) ;
	layout->addStretch() ;
	setLayout( layout ) ;

	connect( m_all_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_all_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_ipv4_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_ipv4_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_ipv6_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_ipv6_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_loopback_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_loopback_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_list_checkbox , SIGNAL(toggled(bool)) , this , SLOT(onToggle()) ) ;
	connect( m_list_checkbox , SIGNAL(toggled(bool)) , this , SIGNAL(pageUpdateSignal()) ) ;
	connect( m_listening_interface , SIGNAL(textChanged(QString)), this, SLOT(onTextChanged()) ) ;
	connect( m_listening_interface , SIGNAL(textChanged(QString)), this, SIGNAL(pageUpdateSignal()) ) ;

	onToggle() ;
}

std::string ListeningPage::nextPage()
{
	return m_next_is_next2 ? next2() : next1() ;
}

void ListeningPage::onTextChanged()
{
	if( m_list_checkbox->isChecked() )
		m_value = value_utf8( m_listening_interface ) ;
}

void ListeningPage::onToggle()
{
	m_listening_interface->setEnabled( m_list_checkbox->isChecked() ) ;
	std::string value ;
	if( m_all_checkbox->isChecked() )
		value.clear() ;
	else if( m_ipv4_checkbox->isChecked() )
		value = "0.0.0.0" ;
	else if( m_ipv6_checkbox->isChecked() )
		value = "::" ;
	else if( m_loopback_checkbox->isChecked() )
		value = "127.0.0.1,::1" ;
	else if( m_list_checkbox->isChecked() )
		value = normalise( m_value ) ;
	m_listening_interface->setText( qstr(value) ) ;
	m_listening_interface->setEnabled( m_list_checkbox->isChecked() ) ;
}

bool ListeningPage::isComplete()
{
	G_DEBUG( "ListeningPage::isComplete" ) ;
	return
		m_list_checkbox->isChecked() ?
			!m_listening_interface->text().trimmed().isEmpty() :
			true ;
}

std::string ListeningPage::normalise( const std::string & s )
{
	return G::Str::join( "," , G::Str::splitIntoTokens(s," ,") ) ;
}

void ListeningPage::dump( std::ostream & stream , bool for_install ) const
{
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "listening-interface" , normalise(value_utf8(m_listening_interface)) ) ;
	dumpItem( stream , for_install , "listening-remote" , value_yn(m_remote_checkbox) ) ;
}

// ==

StartupPage::StartupPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 , bool is_mac ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_is_mac(is_mac)
{
	m_on_boot_checkbox = new QCheckBox( tr("At system startup, running as a service") ) ;
	m_at_login_checkbox = new QCheckBox( tr("When logging in") ) ;
	m_add_menu_item_checkbox = new QCheckBox( tr("Add to start menu") ) ;
	m_add_desktop_item_checkbox = new QCheckBox( tr("Add to desktop") ) ;

	auto * auto_layout = new QVBoxLayout ;
	auto * manual_layout = new QVBoxLayout ;
	auto_layout->addWidget( m_on_boot_checkbox ) ;
	auto_layout->addWidget( m_at_login_checkbox ) ;
	manual_layout->addWidget( m_add_menu_item_checkbox ) ;
	manual_layout->addWidget( m_add_desktop_item_checkbox ) ;

	m_on_boot_checkbox->setEnabled( config.booleanValue("=dir-boot-enabled",false) ) ;
	m_at_login_checkbox->setEnabled( config.booleanValue("=dir-autostart-enabled",false) ) ;
	m_add_menu_item_checkbox->setEnabled( config.booleanValue("=dir-menu-enabled",false) ) ;
	m_add_desktop_item_checkbox->setEnabled( config.booleanValue("=dir-desktop-enabled",false) ) ;

	m_on_boot_checkbox->setChecked( config.booleanValue("start-on-boot",false) ) ;
	m_at_login_checkbox->setChecked( config.booleanValue("start-at-login",false) ) ;
	m_add_menu_item_checkbox->setChecked( config.booleanValue("start-link-menu",false) ) ;
	m_add_desktop_item_checkbox->setChecked( config.booleanValue("start-link-desktop",false) ) ;

	QGroupBox * auto_group = new QGroupBox( tr("Automatic") ) ;
	auto_group->setLayout( auto_layout ) ;

	QGroupBox * manual_group = new QGroupBox( tr("Manual") ) ;
	manual_group->setLayout( manual_layout ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of server-startup page
	layout->addWidget( newTitle(tr("Server startup")) ) ;
	layout->addWidget( auto_group ) ;
	layout->addWidget( manual_group ) ;
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
	Gui::Page::dump( stream , for_install ) ;
	dumpItem( stream , for_install , "start-page" , value_yn(true) ) ; // since not necessarily used at all -- see guimain.cpp
	dumpItem( stream , for_install , "start-on-boot-enabled" , value_yn(m_on_boot_checkbox->isEnabled()) ) ;
	dumpItem( stream , for_install , "start-on-boot" , value_yn(m_on_boot_checkbox) ) ;
	dumpItem( stream , for_install , "start-at-login" , value_yn(m_at_login_checkbox) ) ;
	dumpItem( stream , for_install , "start-link-menu" , value_yn(m_add_menu_item_checkbox) ) ;
	dumpItem( stream , for_install , "start-link-desktop" , value_yn(m_add_desktop_item_checkbox) ) ;
	if( for_install )
		dumpItem( stream , for_install , "start-is-mac" , value_yn(m_is_mac) ) ;
}

// ==

ReadyPage::ReadyPage( Gui::Dialog & dialog , const G::MapFile & , const std::string & name , const std::string & next_1 ,
	const std::string & next_2 , bool installing ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_installing(installing)
{
	m_label = new QLabel( text() ) ;

	auto * layout = new QVBoxLayout ;
	if( installing )
		layout->addWidget( newTitle(tr("Ready to install")) ) ;
	else
		layout->addWidget( newTitle(tr("Ready to configure")) ) ;
	layout->addWidget( m_label ) ;
	setLayout( layout ) ;
}

void ReadyPage::onShow( bool )
{
}

QString ReadyPage::text() const
{
	QString para = m_installing ?
		tr("E-MailRelay will now be installed.") :
		tr("E-MailRelay will now be configured.") ;
	return QString("<center><p>") + para + QString("</p></center>") ;
}

std::string ReadyPage::nextPage()
{
	return next1() ;
}

bool ReadyPage::isReadyToFinishPage() const
{
	return true ;
}

void ReadyPage::dump( std::ostream & s , bool for_install ) const
{
	Gui::Page::dump( s , for_install ) ;
}

// ==

LogWatchThread::LogWatchThread( G::Path path ) :
	m_path(path)
{
	m_stream.open( m_path.iopath() , std::ios_base::ate ) ;
}

void LogWatchThread::run()
{
	while( !m_stream.is_open() )
	{
		m_stream.open( m_path.iopath() ) ;
		if( !m_stream.is_open() )
			msleep( 100 ) ;
	}
	std::string line ;
	for(;;)
	{
		line.clear() ;
		std::getline( m_stream , line ) ;
		if( !line.empty() ) // !eof
			emit newLine( GQt::qstring_from_u8string(line) ) ;
		m_stream.clear( m_stream.rdstate() & ~(std::ios_base::failbit | std::ios_base::eofbit) ) ;
		msleep( 100 ) ;
	}
}

ProgressPage::ProgressPage( Gui::Dialog & dialog , const G::MapFile & , const std::string & name ,
	const std::string & next_1 , const std::string & next_2 ,
	Installer & installer , bool installing ) :
		Gui::Page(dialog,name,next_1,next_2) ,
		m_text_pos(0) ,
		m_installer(installer) ,
		m_state(0)
{
	m_text_edit = new QTextEdit ;
	m_text_edit->setReadOnly( true ) ;
	m_text_edit->setWordWrapMode( QTextOption::NoWrap ) ;
	m_text_edit->setLineWrapMode( QTextEdit::NoWrap ) ;
	//m_text_edit->setFontFamily( "courier" ) ;

	auto * layout = new QVBoxLayout ;
	//: page title of installation-or-reconfiguration progress page
	layout->addWidget( newTitle(installing?tr("Installing"):tr("Configuring")) ) ;
	layout->addWidget(m_text_edit) ;
	setLayout( layout ) ;
}

void ProgressPage::onShow( bool back )
{
	if( ! back )
	{
		// log the install variables
		{
			std::stringstream ss ;
			dialog().dumpInstallVariables( ss ) ;
			if( testMode() )
			{
				std::ofstream f ;
				G::File::open( f , "installer.txt" , G::File::Text() ) ;
				f << ss.str() ;
			}
			std::string line ;
			while( !std::getline(ss,line).eof() )
			{
				if( line.find("-password=") == std::string::npos )
					G_LOG( "ProgressPage::onShow: install: " << line ) ;
			}
		}

		// start running the installer
		std::stringstream ss ;
		dialog().dumpInstallVariables( ss ) ;
		m_installer.start( ss ) ; // reads from istream

		m_text = QString() ;
		m_text_edit->setPlainText( m_text ) ;

		// run a continuous zero-length timer that calls onInstallTimeout()
		m_install_timer = std::make_unique<QTimer>( this ) ;
		connect( m_install_timer.get() , SIGNAL(timeout()) , this , SLOT(onInstallTimeout()) ) ;
		m_state = 0 ;
		m_install_timer->start() ;
	}
}

void ProgressPage::onLaunch()
{
	if( m_logwatch_thread == nullptr )
	{
		G::Path log_path = m_installer.addLauncher() ;
		m_logwatch_thread = new LogWatchThread( log_path ) ;
		connect( m_logwatch_thread , &LogWatchThread::newLine , this , &ProgressPage::onLogWatchLine ) ;
	}
	if( m_install_timer == nullptr )
	{
		m_install_timer = std::make_unique<QTimer>( this ) ;
		connect( m_install_timer.get() , SIGNAL(timeout()) , this , SLOT(onInstallTimeout()) ) ;
	}
	m_state = 10 ;
	m_install_timer->start() ;
}

void ProgressPage::onInstallTimeout()
{
	try
	{
		if( m_state == 0 || m_state == 10 )
		{
			if( m_installer.next() )
			{
				addLineFromOutput( m_installer.output() ) ;
				m_state += 1 ;
			}
			else
			{
				m_state += 2 ;
			}
		}
		else if( m_state == 1 || m_state == 11 )
		{
			m_installer.run() ; // doesnt throw
			replaceLineFromOutput( m_installer.output() ) ;
			m_state -= 1 ;
		}
		else if( m_state == 2 || m_state == 12 )
		{
			m_install_timer->stop() ;
			if( m_installer.failed() )
			{
				if( m_state == 2 )
					addLine( qstr(m_installer.failedText()) ) ;
				else
					m_installer.back() ;
				m_state += 1 ;
			}
			else
			{
				if( m_state == 2 )
					addLine( qstr(m_installer.finishedText()) ) ;
				m_state += 2 ;
				if( m_logwatch_thread )
					m_logwatch_thread->start() ;
			}
		}
		emit pageUpdateSignal() ; // NOLINT
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

void ProgressPage::onLogWatchLine( QString line )
{
	if( !line.isEmpty() )
		addLine( line ) ;
}

void ProgressPage::addLineFromOutput( const Installer::Output & output )
{
	addLine( format(output) ) ;
}

QString ProgressPage::format( const Installer::Output & output )
{
	// returns a formatted "progress" line typically in one of the
	// following forms:
	//
	//   1. <action>...
	//   2. <action> [<subject>]...
	//   3. <action>... <result>
	//   4. <action> [<subject>]... <result>
	//   5. <action>... <error>
	//   6. <action> [<subject>]... <error>
	//   7. <action>... <error-more>
	//   8. <action> [<subject>]... <error-more>
	//   9. <action>... <error>: <error-more>
	//  10. <action> [<subject>]... <error>: <error-more>
	//
	// * the "action", "result" and "error" fields have been translated
	// * the "subject" and "error-more" fields are un-translated
	//
	// the qt translation mechanism is used to format the line -- un-translated
	// strings are distinguished so that the translator can choose move them
	// to the end of the line or not use them at all (for example, to avoid
	// mixed character sets) and they are bound to higher substitution numbers
	// to facilitate this
	//
	// note that in some error situations (see 7 and 8 above) the "error" string
	// can be empty with all the error information contained in the "error-more"
	// string (eg. for system errors) -- translators should ensure that some sort
	// of error message is displayed in this case

	QString action = qstr( output.action ) ;
	QString subject = qstr( output.subject ) ;
	QString result = qstr( output.result ) ;
	QString error = qstr( output.error ) ;
	QString error_more = qstr( output.error_more ) ;

	if( result.isEmpty() && error.isEmpty() && error_more.isEmpty() )
	{
		if( subject.isEmpty() )
			//: installer progress item, no subject, not yet run
			return tr("%1... ","1").arg(action) ;
		else
			//: installer progress item, untranslated subject, not yet run
			return tr("%1 [%2]... ","2").arg(action,subject) ;
	}
	else if( !result.isEmpty() )
	{
		if( subject.isEmpty() )
			//: installer progress item, no subject, with non-error result
			return tr("%1... %2","3").arg(action,result) ;
		else
			//: installer progress item, untranslated subject, with non-error result
			return tr("%1 [%3]... %2","4").arg(action,result,subject) ;
	}
	else if( error_more.isEmpty() )
	{
		if( subject.isEmpty() )
			//: installer progress item, no subject, with translated error result
			return tr("%1... %2","5").arg(action,error) ;
		else
			//: installer progress item, untranslated subject, with translated error result
			return tr("%1 [%3]... %2","6").arg(action,error,subject) ;
	}
	else if( error.isEmpty() )
	{
		if( subject.isEmpty() )
			//: installer progress item, no subject, with native error result
			return tr("%1... %2","7").arg(action,error_more) ;
		else
			//: installer progress item, untranslated subject, with native error result
			return tr("%1 [%3]... %2","8").arg(action,error_more,subject) ;
	}
	else
	{
		if( subject.isEmpty() )
			//: installer progress item, no subject, with translated error result and untranslated error subject
			return tr("%1... %2: %3","9").arg(action,error,error_more) ;
		else
			//: installer progress item, untranslated subject, with translated error result and untranslated error subject
			return tr("%1 [%3]... %2: %4","10").arg(action,error,subject,error_more) ;
	}
}

void ProgressPage::replaceLineFromOutput( const Installer::Output & output )
{
	m_text.resize( m_text_pos ) ; // remove old
	addLineFromOutput( output ) ; // add new
}

void ProgressPage::addLine( const QString & line )
{
	addText( line + QString("\n") ) ;
}

void ProgressPage::addText( const QString & text )
{
	m_text_pos = m_text.size() ;
	m_text.append( text ) ;
	m_text_edit->setPlainText( m_text ) ;
}

std::string ProgressPage::nextPage()
{
	return next1() ;
}

void ProgressPage::dump( std::ostream & s , bool for_install ) const
{
	Gui::Page::dump( s , for_install ) ;
}

bool ProgressPage::isFinishPage() const
{
	return true ;
}

bool ProgressPage::isFinishing()
{
	return !m_installer.done() && m_state < 2 ;
}

bool ProgressPage::isComplete()
{
	return m_state >= 4 ;
}

bool ProgressPage::canLaunch()
{
	return m_state == 4 || m_state == 13 ;
}

