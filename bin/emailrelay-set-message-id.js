//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// Copying and distribution of this file, with or without modification,
// are permitted in any medium without royalty provided the copyright
// notice and this notice are preserved.  This file is offered as-is,
// without any warranty.
// ===
//
// emailrelay-set-message-id.js
//
// An example "--filter" script that sets a message-id header if there
// is none.
//
try
{
	var domain = "example.com" ;

	var content = WScript.Arguments( 0 ) ;
	var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;
	var in_ = fs.OpenTextFile( content , 1 , false ) ;
	var out_ = fs.OpenTextFile( content + ".tmp" , 8 , true ) ;

	var re_message_id = /^Message-ID:/i ;
	var re_fold = /^[ \t]/ ;

	var in_message_id = 0 ;
	var have_message_id = 0 ;
	while( !in_.AtEndOfStream )
	{
		var line = in_.ReadLine() ;
		if( line === "" )
		{
			if( !have_message_id )
			{
				var now = new Date() ;
				var lhs = (now.getTime()/1000) + "." + (Math.random()*1000).toFixed(0) ) ;
				var new_message_id = lhs + "@" + domain ;
				out_.WriteLine( "Message-ID: " + new_message_id ) ;
			}
			out_.WriteLine( line ) ;
			break ;
		}

		if( line.match(re_message_id) )
		{
			have_message_id = 1 ;
			in_message_id = 1 ;
		}
		else if( in_message_id && line.match(re_fold) )
		{
		}
		else
		{
			in_message_id = 0 ;
			out_.WriteLine( line ) ;
		}
	}
	while( !in_.AtEndOfStream )
	{
		var body_line = in_.ReadLine() ;
		out_.WriteLine( body_line ) ;
	}

	in_.Close() ;
	out_.Close() ;
	fs.DeleteFile( content ) ;
	fs.MoveFile( content + ".tmp" , content ) ;

	WScript.Quit( 0 ) ;
}
catch( e )
{
	WScript.StdOut.WriteLine( "<<edit failed>>" ) ;
	WScript.StdOut.WriteLine( "<<" + e + ">>" ) ;
	WScript.Quit( 1 ) ;
}
