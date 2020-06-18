//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// emailrelay-service-install.js
//
// Runs "emailrelay-service --install" and then opens the Windows service
// control panel.
//

try
{
	var title = "E-MailRelay Service Install" ;
	var shell = WScript.CreateObject( "WScript.Shell" ) ;
	var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;
	var this_dir = fs.GetParentFolderName(WScript.ScriptFullName) ;

	// find the installation program's configuration file
	var config_file_1 = this_dir + "\\emailrelay-service.cfg" ;
	var config_file_2 = fs.GetParentFolderName(this_dir) + "\\emailrelay-service.cfg" ;
	var config_file = fs.FileExists(config_file_1) ? config_file_1 : config_file_2 ;
	if( !fs.FileExists(config_file) )
		throw "No installation config file [" + config_file_1 + "] or [" + config_file_2 + "]" ;
	var dir_install = fs.GetParentFolderName( config_file ) ;

	// read and parse the config file
	var dir_config = "" ;
	{
		var f = fs.OpenTextFile( config_file ) ;
		var config_text = f.ReadAll() ;
		f.Close() ;

		var re_config = new RegExp( "^dir.config=([ \\S]*)" , "m" ) ;
		var re_dequote = new RegExp( "\"?([^\"]*)" ) ;

		config_text.match( re_config ) ;
		dir_config = RegExp.lastParen ;
		dir_config.match( re_dequote ) ;
		dir_config = RegExp.lastParen ;
	}
	if( !dir_config )
		throw "Cannot parse the config file [" + config_file + "]" ;

	// check for the startup batch file containing the server configuration
	var startup_batch_file = dir_config + "\\emailrelay-start.bat" ;
	if( !fs.FileExists(startup_batch_file) )
		throw "No startup batch file [" + startup_batch_file + "]" ;

	// check for the service wrapper
	var service_wrapper = dir_install + "\\emailrelay-service.exe" ;
	if( !fs.FileExists(startup_batch_file) )
		throw "No service wrapper [" + service_wrapper + "]" ;

	var ok = shell.Popup( "About to run [" + service_wrapper + " --install]" , 0 , title , 1 ) ;
	if( ok === 1 )
	{
		// do the service wrapper installation
		var exec = shell.exec( "cmd.exe /c \"" + service_wrapper + "\" --install" ) ;
		//var exec = shell.exec( service_wrapper + " --install" ) ;
		while( exec.Status === 0 )
		{
			WScript.Sleep( 100 ) ;
		}

		// report the results
		var stdout_text = exec.StdOut.ReadAll() ;
		var stderr_text = exec.StdErr.ReadAll() ;
		var success = !stderr_text && ( exec.ExitCode === 0 ) ;
		var text = (success?"Service installed":"Service installation failed") + ": " +
			"exit " + exec.ExitCode + "\n" +
			stdout_text + "\n" +
			stderr_text ;
		var topmost = 262144 ;
		var foreground = 65536 ;
		var icon_info = 0 ;
		var icon_hand = 16 ;
		var icon_question = 1 ;
		shell.Popup( text , 0 , title , topmost+foreground+(success?icon_info:icon_hand) ) ;

		// optionally open the services control panel
		if( success )
		{
			if( 1 === shell.Popup( "Open the services control panel?" , 0 , title , topmost+foreground+icon_question ) )
				shell.exec( "cmd.exe /c %windir%\\system32\\services.msc" ) ;
		}
	}
	else
	{
		throw "Cancelled" ;
	}

	WScript.Quit( 0 ) ;
}
catch( e )
{
	shell.Popup( "Error: " + e , 0 , "E-MailRelay Service Install" , 16 ) ;
	WScript.Quit( 1 ) ;
}

