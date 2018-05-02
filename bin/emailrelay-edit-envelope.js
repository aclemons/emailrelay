//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// In this example the "MailFromAuthOut" field in the envelope file is
// modified; this is used as the MAIL FROM AUTH parameter when the emailrelay
// server forwards the message to the downstream server. In principle
// (RFC-2554) when relaying the outgoing MAIL FROM AUTH value should be the
// same as the incoming value iff the submitting client was authenticated
// and trusted; or it can be the the submitting client's authentication id
// if there was no incoming value; or it can be "<>".
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

	// read the incoming values
	var re_auth_in = new RegExp( "^X-MailRelay-MailFromAuthIn: *(\\S*)" , "m" ) ;
	txt.match( re_auth_in ) ;
	var auth_in = RegExp.lastParen ;
	var re_authentication = new RegExp( "^X-MailRelay-Authentication: *(\\S*)" , "m" ) ;
	txt.match( re_authentication ) ;
	var authentication = RegExp.lastParen ;

	// choose the auth-out value
	var auth_out = "<>" ;
	if( authentication )
	{
		function xtext( x ) { return x } // todo
		auth_out = auth_in ? auth_in : xtext(authentication) ;
	}

	// set the auth-out
	var re = new RegExp( "X-MailRelay-MailFromAuthOut: *\\S*" ) ;
	txt = txt.replace( re , "X-MailRelay-MailFromAuthOut: " + auth_out ) ;

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
