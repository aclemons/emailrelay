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
//
// emailrelay-change-auth.js
//
// An example "--filter" script for Windows that changes the 
// authentication field in the message's envelope file. This 
// field is used as the AUTH parameter in the MAIL command 
// when the emailrelay server forwards the message to the 
// downstream server. By default it is the name used by the
// client when it authenticated with emailrelay.
//
try 
{
	var auth = "secret" ; // change this
	var content = WScript.Arguments(0) ;
	var envelope = content.substr(0,content.length-7) + "envelope.new" ;
	var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;
	var ts = fs.OpenTextFile( envelope , 1 , false ) ;
	var txt = ts.ReadAll() ;
	ts.Close() ;
	var re = new RegExp( "X-MailRelay-Authentication: *\\w*" ) ;
	txt = txt.replace( re , "X-MailRelay-Authentication: " + auth ) ;
	ts = fs.OpenTextFile( envelope , 2 , false ) ;
	ts.Write( txt ) ;
	ts.Close() ;
	WScript.Quit( 0 ) ;
} 
catch( e ) 
{
	WScript.Echo( "<<" + e + ">>" ) ;
	WScript.Quit( 1 ) ;
}
