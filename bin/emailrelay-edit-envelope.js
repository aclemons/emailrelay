//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// Copying and distribution of this file, with or without modification,
// are permitted in any medium without royalty provided the copyright
// notice and this notice are preserved.  This file is offered as-is,
// without any warranty.
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
	var envelope = WScript.Arguments(1) ;

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
	WScript.StdOut.WriteLine( "<<edit failed>>" ) ;
	WScript.StdOut.WriteLine( "<<" + e + ">>" ) ;
	WScript.Quit( 1 ) ;
}
