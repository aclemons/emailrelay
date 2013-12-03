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
// emailrelay-edit-envelope.js
//
// An example "--filter" script for Windows that edits the message's envelope 
// file.
//
// In this example the "Authentication" field in the envelope file is modified.
// This field is used as the AUTH parameter when the emailrelay server forwards
// the message to the downstream server.
//
try 
{
	// parse the command-line to get the envelope filename
	var content = WScript.Arguments(0) ;
	var envelope = content.substr(0,content.length-7) + "envelope.new" ;

	// open the envelope file
	var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;
	var ts = fs.OpenTextFile( envelope , 1 , false ) ;

	// read the contents of the envelope file
	var txt = ts.ReadAll() ;
	ts.Close() ;

	// configuration -- this is what we are putting into the envelope file
	var auth = "secret" ;

	// make the change 
	var re = new RegExp( "X-MailRelay-Authentication: *\\w*" ) ;
	txt = txt.replace( re , "X-MailRelay-Authentication: " + auth ) ;

	// write the envelope file back out
	ts = fs.OpenTextFile( envelope , 2 , false ) ;
	ts.Write( txt ) ;
	ts.Close() ;

	// successful exit
	WScript.Quit( 0 ) ;
} 
catch( e ) 
{
	// report errors using the special <<...>> markers
	WScript.StdOut.WriteLine( "<<" + e + ">>" ) ;
	WScript.Quit( 1 ) ;
}
