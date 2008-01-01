//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file pages.h
///

#ifndef G_PAGES_H
#define G_PAGES_H

#include "qt.h"
#include "installer.h"
#include "dir.h"
#include "gpath.h"
#include "gdialog.h"
#include "gpage.h"

class QCheckBox; 
class QComboBox; 
class QRadioButton; 
class QGroupBox; 
class QTextEdit; 
class QLabel; 
class QLineEdit; 
class QPushButton; 
class DetailsPage; 
class EvaluatePage; 
class FinishPage; 
class RegisterPage; 
class TitlePage; 

class TitlePage : public GPage 
{
public:
	TitlePage( GDialog & dialog , const std::string & name , 
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;

private:
	QLabel * m_label ;
	QLabel * m_credit ;
};

class LicensePage : public GPage 
{
public:
	LicensePage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private:
	QTextEdit * m_text_edit ;
	QCheckBox * m_agree_checkbox ;
};

class DirectoryPage : public GPage 
{Q_OBJECT
public:
	DirectoryPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
		const Dir & dir , bool installing ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private slots:
	void browseInstall() ;
	void browseSpool() ;
	void browseConfig() ;

private:
	QString browse( QString ) ;

private:
	const Dir & m_dir ;
	bool m_installing ;
	QLabel * m_install_dir_title ;
	QLabel * m_install_dir_label ;
	QLineEdit * m_install_dir_edit_box ;
	QPushButton * m_install_dir_browse_button ;
	QLabel * m_spool_dir_title ;
	QLabel * m_spool_dir_label ;
	QLineEdit * m_spool_dir_edit_box ;
	QPushButton * m_spool_dir_browse_button ;
	QLabel * m_config_dir_title ;
	QLabel * m_config_dir_label ;
	QLineEdit * m_config_dir_edit_box ;
	QPushButton * m_config_dir_browse_button ;
} ;

class DoWhatPage : public GPage 
{Q_OBJECT
public:
	DoWhatPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private:
	QCheckBox * m_pop_checkbox ;
	QCheckBox * m_smtp_checkbox ;
	QRadioButton * m_immediate_checkbox ;
	QRadioButton * m_periodically_checkbox ;
	QRadioButton * m_on_demand_checkbox ;
	QComboBox * m_period_combo ;
	QGroupBox * m_forwarding_group ;

private slots:
	void onToggle() ;
} ;

class PopPage : public GPage 
{Q_OBJECT
public:
	explicit PopPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private slots:
	void onToggle() ;

private:
	QLineEdit * m_port_edit_box ;
	QRadioButton * m_one ;
	QRadioButton * m_shared ;
	QRadioButton * m_pop_by_name ;
	QCheckBox * m_no_delete_checkbox ;
	QCheckBox * m_auto_copy_checkbox ;
} ;

class PopAccountsPage : public GPage 
{
public:
	explicit PopAccountsPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private:
	QComboBox * m_mechanism_combo ;
	QLineEdit * m_name_1 ;
	QLineEdit * m_pwd_1 ;
	QLineEdit * m_name_2 ;
	QLineEdit * m_pwd_2 ;
	QLineEdit * m_name_3 ;
	QLineEdit * m_pwd_3 ;
} ;

class PopAccountPage : public GPage 
{
public:
	explicit PopAccountPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private:
	QComboBox * m_mechanism_combo ;
	QLineEdit * m_name_1 ;
	QLineEdit * m_pwd_1 ;
} ;

class SmtpServerPage : public GPage 
{Q_OBJECT
public:
	SmtpServerPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private:
	QLineEdit * m_port_edit_box ;
	QCheckBox * m_auth_checkbox ;
	QComboBox * m_mechanism_combo ;
	QGroupBox * m_account_group ;
	QLineEdit * m_account_name ;
	QLineEdit * m_account_pwd ;
	QLineEdit * m_trust_address ;
	QGroupBox * m_trust_group ;

private slots:
	void onToggle() ;
} ;

class SmtpClientPage : public GPage 
{Q_OBJECT
public:
	SmtpClientPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private:
	QLineEdit * m_server_edit_box ;
	QLineEdit * m_port_edit_box ;
	QCheckBox * m_tls_checkbox ;
	QCheckBox * m_auth_checkbox ;
	QComboBox * m_mechanism_combo ;
	QGroupBox * m_account_group ;
	QLineEdit * m_account_name ;
	QLineEdit * m_account_pwd ;

private slots:
	void onToggle() ;
} ;

class StartupPage : public GPage 
{
public:
	StartupPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
		const Dir & dir ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;

private:
	QCheckBox * m_on_boot_checkbox ;
	QCheckBox * m_at_login_checkbox ;
	QCheckBox * m_add_menu_item_checkbox ;
	QCheckBox * m_add_desktop_item_checkbox ;
	QCheckBox * m_verbose_checkbox ;
} ;

class LoggingPage : public GPage 
{
public:
	LoggingPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;

private:
	QCheckBox * m_debug_checkbox ;
	QCheckBox * m_verbose_checkbox ;
	QCheckBox * m_syslog_checkbox ;
} ;

class ListeningPage : public GPage 
{Q_OBJECT
public:
	ListeningPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual bool isComplete() ;

private slots:
	void onToggle() ;

private:
	QCheckBox * m_remote_checkbox ;
	QRadioButton * m_all_radio ;
	QRadioButton * m_one_radio ;
	QLineEdit * m_listening_interface ;
} ;

class ReadyPage : public GPage 
{
public:
	ReadyPage( GDialog & dialog , const std::string & name , const std::string & next_1 , 
		const std::string & next_2 , bool finish , bool close , bool installing ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual void onShow( bool back ) ;

private:
	QString text() const ;
	std::string verb( bool ) const ;

private:
	QLabel * m_label ;
	bool m_installing ;
} ;

class ProgressPage : public GPage 
{Q_OBJECT
public:
	ProgressPage( GDialog & dialog , const std::string & name , const std::string & next_1 , 
		const std::string & next_2 , bool finish , bool close , G::Path argv0 , G::Path dump_file ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
	virtual void onShow( bool back ) ;
	virtual bool closeButton() const ;
	virtual bool isComplete() ;

private slots:
	void poke() ;

private:
	void addLine( const std::string & ) ;

private:
	QTextEdit * m_text_edit ;
	QTimer * m_timer ;
	Installer m_installer ;
	QString m_text ;
	G::Path m_dump_file ;
} ;

class EndPage_ : public GPage 
{
public:
	EndPage_( GDialog & dialog , const std::string & name ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , const std::string & , const std::string & ) const ;
} ;

#endif
