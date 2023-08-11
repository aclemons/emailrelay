//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file guipages.h
///

#ifndef G_MAIN_GUI_PAGES_H
#define G_MAIN_GUI_PAGES_H

#include "gdef.h"
#include "gqt.h"
#include "guidialog.h"
#include "guipage.h"
#include "installer.h"
#include "gmapfile.h"
#include "gpath.h"
#include "gstringarray.h"
#include <fstream>

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

class TitlePage : public Gui::Page
{Q_OBJECT
public:
	TitlePage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;

private:
	QLabel * m_label ;
	QLabel * m_credit ;
};

class LicensePage : public Gui::Page
{Q_OBJECT
public:
	LicensePage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool accepted ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;
	std::string helpUrl( const std::string & ) const override ;

private:
	QTextEdit * m_text_edit ;
	QCheckBox * m_agree_checkbox ;
};

class DirectoryPage : public Gui::Page
{Q_OBJECT
public:
	DirectoryPage( Gui::Dialog & dialog , const G::MapFile & , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ,
		bool installing , bool is_windows , bool is_mac ) ;

	G::Path installDir() const ;
	G::Path spoolDir() const ;
	G::Path runtimeDir() const ;
	G::Path configDir() const ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;

private slots:
	void onOtherDirChange() ;
	void onInstallDirChange() ;
	void browseInstall() ;
	void browseSpool() ;
	void browseConfig() ;
	void browseRuntime() ;

private:
	QString browse( QString ) ;
	G::Path normalise( const G::Path & ) const ;

private:
	bool m_installing ;
	QLabel * m_install_dir_label ;
	QString m_install_dir_start ;
	QLineEdit * m_install_dir_edit_box ;
	QPushButton * m_install_dir_browse_button ;
	QLabel * m_spool_dir_label ;
	QString m_spool_dir_start ;
	QLineEdit * m_spool_dir_edit_box ;
	QPushButton * m_spool_dir_browse_button ;
	QLabel * m_config_dir_label ;
	QString m_config_dir_start ;
	QLineEdit * m_config_dir_edit_box ;
	QPushButton * m_config_dir_browse_button ;
	QLabel * m_runtime_dir_label ;
	QString m_runtime_dir_start ;
	QLineEdit * m_runtime_dir_edit_box ;
	QPushButton * m_runtime_dir_browse_button ;
	bool m_is_mac ;
	bool m_other_dir_changed ;
} ;

class DoWhatPage : public Gui::Page
{Q_OBJECT
public:
	DoWhatPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;
	bool pop() const ;

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

class PopPage : public Gui::Page
{Q_OBJECT
public:
	explicit PopPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ,
		bool have_accounts ) ;
	bool withFilterCopy() const ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;

private slots:
	void onToggle() ;

private:
	QLineEdit * m_port_edit_box ;
	QRadioButton * m_one ;
	QRadioButton * m_shared ;
	QRadioButton * m_pop_by_name ;
	QCheckBox * m_no_delete_checkbox ;
	QCheckBox * m_pop_filter_copy_checkbox ;
	bool m_have_accounts ;
	QLineEdit * m_name_1 ;
	QLineEdit * m_pwd_1 ;
	QLineEdit * m_name_2 ;
	QLineEdit * m_pwd_2 ;
	QLineEdit * m_name_3 ;
	QLineEdit * m_pwd_3 ;
} ;

class SmtpServerPage : public Gui::Page
{Q_OBJECT
public:
	SmtpServerPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ,
		bool have_account , bool is_windows ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;

	QString browse( QString ) ;

private:
	bool m_have_account ;
	bool m_can_generate ;
	QLineEdit * m_port_edit_box ;
	QCheckBox * m_auth_checkbox ;
	QGroupBox * m_account_group ;
	QLineEdit * m_account_name ;
	QLineEdit * m_account_pwd ;
	QLineEdit * m_trust_address ;
	QGroupBox * m_trust_group ;
	QCheckBox * m_tls_checkbox ;
	QRadioButton * m_tls_starttls ;
	QRadioButton * m_tls_tunnel ;
	QPushButton * m_tls_browse_button ;
	QLabel * m_tls_certificate_label ;
	QLineEdit * m_tls_certificate_edit_box ;

private slots:
	void onToggle() ;
	void browseCertificate() ;
} ;

class FilterPage : public Gui::Page
{Q_OBJECT
public:
	FilterPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ,
		bool installing , bool is_windows ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	void onShow( bool ) override ;

private slots:
	void onToggle() ;

private:
	QLabel * m_server_filter_label ;
	QRadioButton * m_server_filter_choice_none ;
	QRadioButton * m_server_filter_choice_script ;
	QRadioButton * m_server_filter_choice_spamd ;
	QRadioButton * m_server_filter_choice_copy ;
	QLineEdit * m_server_filter_edit_box ;
	QRadioButton * m_client_filter_choice_none ;
	QRadioButton * m_client_filter_choice_script ;
	QLabel * m_client_filter_label ;
	QLineEdit * m_client_filter_edit_box ;
	bool m_installing ;
	bool m_is_windows ;
	std::string m_dot_exe ;
	std::string m_dot_script ;
	//
	bool m_first_show ;
	bool m_pop_page_with_filter_copy ;
	std::string m_server_filter ;
	G::Path m_server_filter_script_path ;
	G::Path m_server_filter_script_path_default ;
	G::Path m_server_filter_copy ;
	G::Path m_server_filter_copy_default ;
	std::string m_server_filter_spam ;
	std::string m_server_filter_spam_default ;
	std::string m_client_filter ;
	G::Path m_client_filter_script_path ;
	G::Path m_client_filter_script_path_default ;
} ;

class SmtpClientPage : public Gui::Page
{Q_OBJECT
public:
	SmtpClientPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool have_account ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;

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

class StartupPage : public Gui::Page
{Q_OBJECT
public:
	StartupPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ,
		bool is_mac ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;

private:
	bool m_is_mac ;
	QCheckBox * m_on_boot_checkbox ;
	QCheckBox * m_at_login_checkbox ;
	QCheckBox * m_add_menu_item_checkbox ;
	QCheckBox * m_add_desktop_item_checkbox ;
} ;

class LoggingPage : public Gui::Page
{Q_OBJECT
public:
	LoggingPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;
	void onShow( bool back ) override ;

private slots:
	void browseLogFile() ;
	void onToggle() ;

private:
	QString browse( QString ) ;

private:
	G::Path m_config_log_file ;
	QCheckBox * m_log_level_verbose_checkbox ;
	QCheckBox * m_log_level_debug_checkbox ;
	QCheckBox * m_log_output_syslog_checkbox ;
	QCheckBox * m_log_output_file_checkbox ;
	QLabel * m_log_output_file_label ;
	QLineEdit * m_log_output_file_edit_box ;
	QPushButton * m_log_output_file_browse_button ;
	QCheckBox * m_log_fields_time_checkbox ;
	QCheckBox * m_log_fields_address_checkbox ;
} ;

class ListeningPage : public Gui::Page
{Q_OBJECT
public:
	ListeningPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 , bool next_is_next2 ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	bool isComplete() override ;
	static std::string normalise( const std::string & ) ;

private slots:
	void onToggle() ;
	void onTextChanged() ;

private:
	bool m_next_is_next2 ;
	QCheckBox * m_remote_checkbox ;
	QRadioButton * m_all_checkbox ;
	QRadioButton * m_ipv4_checkbox ;
	QRadioButton * m_ipv6_checkbox ;
	QRadioButton * m_loopback_checkbox ;
	QRadioButton * m_list_checkbox ;
	QLineEdit * m_listening_interface ;
	std::string m_value ;
} ;

class ReadyPage : public Gui::Page
{Q_OBJECT
public:
	ReadyPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name , const std::string & next_1 ,
		const std::string & next_2 , bool installing ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	void onShow( bool back ) override ;
	bool isReadyToFinishPage() const override ;

private:
	QString text() const ;

private:
	QLabel * m_label ;
	bool m_installing ;
} ;

class LogWatchThread : public QThread
{Q_OBJECT
public:
	explicit LogWatchThread( G::Path ) ;
	void run() override ;

private:
	G::Path m_path ;
	std::ifstream m_stream ;

signals:
	void newLine( QString ) ;
} ;

class ProgressPage : public Gui::Page
{Q_OBJECT
public:
	ProgressPage( Gui::Dialog & dialog , const G::MapFile & config , const std::string & name , const std::string & next_1 ,
		const std::string & next_2 , Installer & , bool installing ) ;

	std::string nextPage() override ;
	void dump( std::ostream & , bool ) const override ;
	void onShow( bool back ) override ;
	void onLaunch() override ;
	bool isFinishPage() const override ;
	bool isFinishing() override ;
	bool canLaunch() override ;
	bool isComplete() override ;

private slots:
	void onInstallTimeout() ;
	void onLogWatchLine( QString ) ;

private:
	QString format( const Installer::Output & ) ;
	void addOutput( const Installer::Output & ) ;
	void replaceOutput( const Installer::Output & ) ;
	void addLine( const QString & text ) ;
	void addText( const QString & text ) ;

private:
	G::Path m_argv0 ;
	QTextEdit * m_text_edit ;
	QString m_text ;
	int m_text_pos ;
	std::unique_ptr<QTimer> m_install_timer ;
	Installer & m_installer ;
	int m_state ;
	LogWatchThread * m_logwatch_thread{nullptr} ;
} ;

#endif
