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
// pages.h
//

#ifndef G_PAGES_H
#define G_PAGES_H

#include <QObject>
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
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;

private:
	QLabel * m_label ;
};

class LicensePage : public GPage 
{
public:
	LicensePage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
	virtual bool isComplete() ;

private:
	QTextEdit * m_text_edit ;
	QCheckBox * m_agree_check_box ;
};

class DirectoryPage : public GPage 
{Q_OBJECT
public:
	DirectoryPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual void reset() ;
	virtual std::string nextPage() ;
	virtual bool isComplete() ;

private slots:
	void browseInstall() ;
	void browseSpool() ;

private:
	QString browse( QString ) ;

private:
	QLabel * m_install_dir_title ;
	QLabel * m_install_dir_label ;
	QLineEdit * m_install_dir_edit_box ;
	QPushButton * m_install_dir_browse_button ;
	QLabel * m_spool_dir_title ;
	QLabel * m_spool_dir_label ;
	QLineEdit * m_spool_dir_edit_box ;
	QPushButton * m_spool_dir_browse_button ;
} ;

class DoWhatPage : public GPage 
{Q_OBJECT
public:
	DoWhatPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
	virtual bool isComplete() ;

private:
	QCheckBox * m_pop_check_box ;
	QCheckBox * m_smtp_check_box ;
	QRadioButton * m_immediate_check_box ;
	QRadioButton * m_periodically_check_box ;
	QRadioButton * m_on_demand_check_box ;
	QComboBox * m_period_combo_box ;
	QGroupBox * m_forwarding_box ;

private slots:
	void onToggle() ;
} ;

class PopPage : public GPage 
{Q_OBJECT
public:
	explicit PopPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
	virtual bool isComplete() ;

private slots:
	void onToggle() ;

private:
	QLineEdit * m_port_edit_box ;
	QRadioButton * m_one ;
	QRadioButton * m_shared ;
	QRadioButton * m_pop_by_name ;
	QCheckBox * m_no_delete_check_box ;
	QCheckBox * m_auto_copy_check_box ;
} ;

class PopAccountsPage : public GPage 
{
public:
	explicit PopAccountsPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
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
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
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
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
	virtual bool isComplete() ;

private:
	QLineEdit * m_port_edit_box ;
	QCheckBox * m_auth_check_box ;
	QComboBox * m_mechanism_combo ;
	QGroupBox * m_account_box ;
	QLineEdit * m_account_name ;
	QLineEdit * m_account_pwd ;

private slots:
	void onToggle() ;
} ;

class SmtpClientPage : public GPage 
{Q_OBJECT
public:
	SmtpClientPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
	virtual bool isComplete() ;

private:
	QLineEdit * m_server_edit_box ;
	QLineEdit * m_port_edit_box ;
	QCheckBox * m_auth_check_box ;
	QComboBox * m_mechanism_combo ;
	QGroupBox * m_account_box ;
	QLineEdit * m_account_name ;
	QLineEdit * m_account_pwd ;

private slots:
	void onToggle() ;
} ;

class StartupPage : public GPage 
{
public:
	StartupPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;

private:
	QCheckBox * m_on_boot_check_box ;
	QCheckBox * m_at_login_check_box ;
	QCheckBox * m_add_menu_item_check_box ;
	QCheckBox * m_add_desktop_item_check_box ;
	QCheckBox * m_verbose_check_box ;
} ;

class FinalPage : public GPage 
{
public:
	FinalPage( GDialog & dialog , const std::string & name ,
		const std::string & next_1 = std::string() , const std::string & next_2 = std::string() ) ;

	virtual std::string nextPage() ;
} ;

#endif
