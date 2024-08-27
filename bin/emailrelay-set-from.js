//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// Copying and distribution of this file, with or without modification,
// are permitted in any medium without royalty provided the copyright
// notice and this notice are preserved.  This file is offered as-is,
// without any warranty.
// ===
//
// emailrelay-set-from.js
//
// An example "--filter" script that edits the content originator fields
// (ie. From, Sender and Reply-To) to a fixed value.
//
// Also consider setting the envelope-from field by editing the envelope
// file, as in emailrelay-edit-envelope.js.
//
// See also: emailrelay-set-from.pl, RFC-2822
//
try
{
	var new_from = 'noreply@example.com' ;
	var new_sender = '' ;
	var new_reply_to = new_from ;

	var content = WScript.Arguments( 0 ) ;
	var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;
	var in_ = fs.OpenTextFile( content , 1 , false ) ;
	var out_ = fs.OpenTextFile( content + ".tmp" , 8 , true ) ;

	var re_from = /^From:/i ;
	var re_sender = /^Sender:/i ;
	var re_reply_to = /^Reply-To:/i ;
	var re_fold = /^[ \t]/ ;

	var in_edit = 0 ;
	while( !in_.AtEndOfStream )
	{
		var line = in_.ReadLine() ;
		if( line === "" )
		{
			out_.WriteLine( line ) ;
			break ;
		}

		if( line.match(re_from) && new_from !== null )
		{
			in_edit = 1 ;
			line = "From: " + new_from ;
			out_.WriteLine( line ) ;
		}
		else if( line.match(re_sender) && new_sender !== null )
		{
			in_edit = 1 ;
			line = "Sender: " + new_sender ;
			if( new_sender !== "" )
			{
				out_.WriteLine( line ) ;
			}
		}
		else if( line.match(re_reply_to) && new_reply_to !== null )
		{
			in_edit = 1 ;
			line = "Reply-To: " + new_reply_to ;
			out_.WriteLine( line ) ;
		}
		else if( in_edit && line.match(re_fold) )
		{
		}
		else
		{
			in_edit = 0 ;
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
