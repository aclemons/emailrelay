//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// Copying and distribution of this file, with or without modification,
// are permitted in any medium without royalty provided the copyright
// notice and this notice are preserved.  This file is offered as-is,
// without any warranty.
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
		if( line === "" )
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
	fs.DeleteFile( content ) ;
	fs.MoveFile( content + ".tmp" , content ) ;

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
