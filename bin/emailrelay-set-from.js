//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// emailrelay-set-from.js
//
// An example "--filter" script that edits the content originator fields
// (ie. From, Sender and Reply-To) to a fixed value.
//
// See also: RFC-2822
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
catch
{
	WScript.StdOut.WriteLine( "<<edit failed>>" ) ;
	WScript.StdOut.WriteLine( "<<" + e + ">>" ) ;
	WScript.Quit( 1 ) ;
}
