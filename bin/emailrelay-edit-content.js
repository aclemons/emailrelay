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
// emailrelay-edit-content.js
//
// An example "--filter" script for Windows that edits the message's content 
// file.
//
// In this example every "teh" is changed to "the".
//
try 
{
	// parse the command-line to get the content filename
	var content = WScript.Arguments(0) ;

	// open the content file
	var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;
	var in_ = fs.OpenTextFile( content , 1 , false ) ;

	// create the new content file
	var out_ = fs.OpenTextFile( content + ".tmp" , 8 , true ) ;

	// read the headers
	while( !in_.AtEndOfStream )
	{
		var line = in_.ReadLine() ;
		out_.WriteLine( line ) ;
		if( line == "" )
			break ;
	}

	// read and edit the body
	var re = new RegExp( "\\bteh\\b" , "gi" ) ;
	while( !in_.AtEndOfStream )
	{
		var line = in_.ReadLine() ;
		line = line.replace( re , "the" ) ;
		out_.WriteLine( line ) ;
	}

	// replace the content file
	in_.Close() ;
	out_.Close() ;
	fs.MoveFile( content + ".tmp" , content ) ;

	// successful exit
	WScript.Quit( 0 ) ;
} 
catch( e ) 
{
	// report errors using the special <<...>> markers
	WScript.StdOut.WriteLine( "<<" + e + ">>" ) ;
	WScript.Quit( 1 ) ;
}
