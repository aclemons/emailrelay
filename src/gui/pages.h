//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#include "gdef.h"
#include "gmapfile.h"
#include "gpath.h"
#include "gstrings.h"
#include "qt.h"
#include "gdialog.h"
#include "gpage.h"
#include "installer.h"

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
	TitlePage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;

private:
	QLabel * m_label ;
	QLabel * m_credit ;
};

class LicensePage : public GPage
{
public:
	LicensePage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool accepted ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;

private:
	QTextEdit * m_text_edit ;
	QCheckBox * m_agree_checkbox ;
};

class DirectoryPage : public GPage
{Q_OBJECT
public:
	DirectoryPage( GDialog & dialog , const G::MapFile & , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
		bool installing , bool is_mac ) ;

	G::Path installDir() const ;
	G::Path runtimeDir() const ;
	G::Path configDir() const ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;

private slots:
	void browseInstall() ;
	void browseSpool() ;
	void browseConfig() ;
	void browseRuntime() ;

private:
	QString browse( QString ) ;
	G::Path normalise( const G::Path & ) const ;

private:
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
	QLabel * m_runtime_dir_title ;
	QLabel * m_runtime_dir_label ;
	QLineEdit * m_runtime_dir_edit_box ;
	QPushButton * m_runtime_dir_browse_button ;
	bool m_is_mac ;
} ;

class DoWhatPage : public GPage
{Q_OBJECT
public:
	DoWhatPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;

private:
	QCheckBox * m_pop_checkbox ;
	QCheckBox * m_smtp_checkbox ;
	QRadioButton * m_immediate_checkbox ;
	QRadioButton * m_on_disconnect_checkbox ;
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
	explicit PopPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
		bool have_accounts ) ;
	bool withFilterCopy() const ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;

private slots:
	void onToggle() ;

private:
	QLineEdit * m_port_edit_box ;
	QRadioButton * m_one ;
	QRadioButton * m_shared ;
	QRadioButton * m_pop_by_name ;
	QCheckBox * m_no_delete_checkbox ;
	QCheckBox * m_filter_copy_checkbox ;
	bool m_have_accounts ;
	QLineEdit * m_name_1 ;
	QLineEdit * m_pwd_1 ;
	QLineEdit * m_name_2 ;
	QLineEdit * m_pwd_2 ;
	QLineEdit * m_name_3 ;
	QLineEdit * m_pwd_3 ;
} ;

class SmtpServerPage : public GPage
{Q_OBJECT
public:
	SmtpServerPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool have_account ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;
	QString browse( QString ) ;

private:
	bool m_have_account ;
	QLineEdit * m_port_edit_box ;
	QCheckBox * m_auth_checkbox ;
	QGroupBox * m_account_group ;
	QLineEdit * m_account_name ;
	QLineEdit * m_account_pwd ;
	QLineEdit * m_trust_address ;
	QGroupBox * m_trust_group ;
	QCheckBox * m_tls_checkbox ;
	QPushButton * m_tls_browse_button ;
	QLabel * m_tls_certificate_label ;
	QLineEdit * m_tls_certificate_edit_box ;
	QHBoxLayout * m_tls_inner_layout ;

private slots:
	void onToggle() ;
	void browseCertificate() ;
} ;

class FilterPage : public GPage
{Q_OBJECT
public:
	FilterPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
		bool is_windows ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual std::string helpName() const ;
	virtual void onShow( bool ) ;

private slots:
	void onToggle() ;

private:
	QCheckBox * m_filter_checkbox ;
	QLabel * m_filter_label ;
	QLineEdit * m_filter_edit_box ;
	QCheckBox * m_client_filter_checkbox ;
	QLabel * m_client_filter_label ;
	QLineEdit * m_client_filter_edit_box ;
	bool m_is_windows ;
	std::string m_dot_exe ;
	std::string m_dot_script ;
	//
	bool m_pop_page_with_filter_copy ;
	G::Path m_filter_copy_path ;
	G::Path m_filter_path ;
	G::Path m_filter_path_default ;
	G::Path m_client_filter_path ;
	G::Path m_client_filter_path_default ;
} ;

class SmtpClientPage : public GPage
{Q_OBJECT
public:
	SmtpClientPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close , bool have_account ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;

private:
	bool m_have_account ;
	QLineEdit * m_server_edit_box ;
	QLineEdit * m_port_edit_box ;
	QCheckBox * m_tls_checkbox ;
	QCheckBox * m_auth_checkbox ;
	QRadioButton * m_tls_starttls ;
	QRadioButton * m_tls_tunnel ;
	QGroupBox * m_account_group ;
	QLineEdit * m_account_name ;
	QLineEdit * m_account_pwd ;

private slots:
	void onToggle() ;
} ;

class StartupPage : public GPage
{
public:
	StartupPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ,
		bool start_on_boot_able , bool is_mac ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;

private:
	bool m_is_mac ;
	QCheckBox * m_on_boot_checkbox ;
	QCheckBox * m_at_login_checkbox ;
	QCheckBox * m_add_menu_item_checkbox ;
	QCheckBox * m_add_desktop_item_checkbox ;
} ;

class LoggingPage : public GPage
{Q_OBJECT
public:
	LoggingPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual std::string helpName() const ;
	virtual bool isComplete() ;
	virtual void onShow( bool back ) ;

private slots:
	void browseLogFile() ;
	void onToggle() ;

private:
	QString browse( QString ) ;

private:
	G::Path m_config_log_file ;
	QCheckBox * m_verbose_checkbox ;
	QCheckBox * m_debug_checkbox ;
	QCheckBox * m_syslog_checkbox ;
	QCheckBox * m_logfile_checkbox ;
	QLabel * m_logfile_label ;
	QLineEdit * m_logfile_edit_box ;
	QPushButton * m_logfile_browse_button ;
} ;

class ListeningPage : public GPage
{Q_OBJECT
public:
	ListeningPage( GDialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool finish , bool close ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;

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
	ReadyPage( GDialog & dialog , const G::MapFile & config , const std::string & name , const std::string & next_1 ,
		const std::string & next_2 , bool finish , bool close , bool installing ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual void onShow( bool back ) ;
	virtual std::string helpName() const ;

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
	ProgressPage( GDialog & dialog , const G::MapFile & config , const std::string & name , const std::string & next_1 ,
		const std::string & next_2 , bool finish , bool close , Installer & ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
	virtual void onShow( bool back ) ;
	virtual bool closeButton() const ;
	virtual bool isComplete() ;
	virtual std::string helpName() const ;

private slots:
	void poke() ;

private:
	void addLine( const std::string & ) ;

private:
	G::Path m_argv0 ;
	QTextEdit * m_text_edit ;
	QTimer * m_timer ;
	Installer & m_installer ;
	QString m_text ;
} ;

class EndPage_ : public GPage
{
public:
	EndPage_( GDialog & dialog , const G::MapFile & config , const std::string & name ) ;

	virtual std::string nextPage() ;
	virtual void dump( std::ostream & , bool ) const ;
} ;

#endif
