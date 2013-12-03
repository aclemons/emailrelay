//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// emailrelay-runperl.js
//
// An example "--filter" script for Windows that runs a perl script
// to process e-mails.
//
// The perl script is expected to read the e-mail content on its
// standard input and write it out again on standard output, with
// a zero exit code on success (like spamassassin, for example).
//
// The E-MailRelay command-line should look something like this:
//
//   emailrelay --as-server --filter "c:/program\ files/emailrelay/emailrelay-runperl.js"
//
// Note the backslash to escape the space in the path.
//

var rc = 1 ;
try
{
	// configuration -- edit these lines as necessary, but avoid spaces in paths
	var cfg_perl = "perl -S -T -w" ;
	var cfg_perl_script = "spamassassin" ;

	// parse our command line
	var args = WScript.Arguments ;
	var filename = args(0) ;

	// prepare a perl commandline with quotes and redirection etc
	var cmd_in = "\"" + filename + "\"" ;
	var cmd_out = "\"" + filename + ".tmp\"" ;
	var cmd_err = "\"" + filename + ".err\"" ;
	var cmd_perl = "cmd /c " + cfg_perl + " " + cfg_perl_script ;
	var cmd = cmd_perl + " < " + cmd_in + " > " + cmd_out + " 2> " + cmd_err ;

	// run the perl command
	var sh = WScript.CreateObject("WScript.Shell") ;
	rc = sh.Run( cmd , 0 , true ) ;

	// check the file redirection worked
	var fs = WScript.CreateObject("Scripting.FileSystemObject") ;
	if( !fs.FileExists(filename+".tmp") || !fs.FileExists(filename+".err") )
	{
		throw "file redirection error" ;
	}

	// check for perl script errors
	if( rc != 0 )
	{
		// read one line of the perl script's standard error
		var reason = "non-zero exit" ;
		var errorstream = fs.OpenTextFile( filename + ".err" , 1 )
		if( ! errorstream.AtEndOfStream )
			reason = errorstream.ReadLine() ;
		errorstream.Close() ;
		
		// clean up
		fs.DeleteFile( filename + ".err" ) ;
		fs.DeleteFile( filename + ".tmp" ) ;

		throw reason ;
	}

	// clean up
	fs.DeleteFile( filename ) ;
	fs.DeleteFile( filename + ".err" ) ;

	// install the perl script output as the new content file
	fs.MoveFile( filename + ".tmp" , filename ) ;

	// successful exit
	WScript.Quit( 0 ) ;
}
catch( e ) 
{
	// report errors using the special <<...>> markers
	WScript.StdOut.WriteLine( "<<" + e + ">>" ) ;
	WScript.Quit( rc ) ;
}

