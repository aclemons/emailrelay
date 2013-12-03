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
// emailrelay-resubmit.js
//
// A utility script for Windows that looks for all failed e-mails in the 
// E-MailRelay spool directory and resubmits them. However, if an e-mail has 
// been retried five times already then it is not submitted again.
//
// usage: cscript //nologo emailrelay-resubmit.js [<spool-dir>]
//

// configuration
//
var cfg_store = "c:/windows/spool/emailrelay" ;
var cfg_retry_limit = 5 ;
var cfg_debug = false ;

// parse the command line
//
var args = WScript.Arguments
if( args.length >= 1 )
{
	cfg_store = args(0) ;
}

// debugging
//
function debug( line )
{
	if( cfg_debug )
	{
		WScript.StdOut.WriteLine( "debug: " + line ) ;
	}
}

// check the spool directory
//
var fso = WScript.CreateObject( "Scripting.FileSystemObject" ) ;
if( ! fso.FolderExists( cfg_store ) )
{
	WScript.StdOut.WriteLine( "invalid spool directory: \"" + cfg_store + "\"" ) ;
	WScript.Quit( 1 ) ;
}

// for each file...
//
var folder = fso.GetFolder( cfg_store ) ;
var re_reason = new RegExp( "MailRelay-Reason: " , "" ) ;
var re_bad = new RegExp( ".*\.bad" , "i" ) ;
var iter = new Enumerator( folder.Files ) ;
for( ; ! iter.atEnd() ; iter.moveNext() )
{
	// if a failed envelope file...
	var path = new String(iter.item()) ;
	debug( "path: " + path ) ;
	if( path.match(re_bad) )
	{
		// count the failure lines
		var stream = fso.OpenTextFile( path , 1 ) ;
		var failures = 0 ;
		while( !stream.AtEndOfStream )
		{
			var line = stream.ReadLine() ;
			debug( "line: " + line ) ;
			if( line.match(re_reason) )
			{
				failures++ ;
			}
		}
		stream.Close() ;
		debug( "failures: " + failures ) ;
		if( failures < cfg_retry_limit )
		{
			// remove the ".bad" suffix
			var new_path = path.substr( 0 , path.length-4 ) ;
			debug( "rename: " + path + " -> " + new_path ) ;
			fso.MoveFile( path , new_path ) ;
		}
	}
}
WScript.Quit( 0 ) ;

