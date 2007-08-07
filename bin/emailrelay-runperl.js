//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file emailrelay-runperl.js
///
var cfg_perl="perl -S -T -w"
var cfg_perl_script="spamassassin"

/// parse our command line
var args = WScript.Arguments
var filename = args(0)

/// prepare a command using CMD.EXE to do file redirection
var cmd_in = "\"" + filename + "\""
var cmd_out = "\"" + filename + ".tmp\""
var cmd_err = "\"" + filename + ".err\""
var cmd_perl = "cmd /c " + cfg_perl + " " + cfg_perl_script
var cmd = cmd_perl + " < " + cmd_in + " > " + cmd_out + " 2> " + cmd_err

/// run the command
var sh = WScript.CreateObject("WScript.Shell")
var rc = sh.Run( cmd , 0 , true )

/// check the file redirection
var fs = WScript.CreateObject("Scripting.FileSystemObject")
if( !fs.FileExists(filename+".tmp") || !fs.FileExists(filename+".err") )
{
	WScript.Echo("<<file redirection error>>")
	WScript.Quit( 2 )
}

/// success or failure
if( rc == 0 )
{
	fs.DeleteFile( filename )
	fs.MoveFile( filename + ".tmp" , filename )
	fs.DeleteFile( filename + ".err" )
	WScript.Quit( 0 )
}
else
{
	fs.DeleteFile( filename + ".tmp" )
	var error = fs.OpenTextFile( filename + ".err" , 1 )
	if( ! error.AtEndOfStream )
	{
		var reason = error.ReadLine()
		WScript.Echo( "<<" + reason + ">>" )
	}
	error.Close()
	fs.DeleteFile( filename + ".err" )
	WScript.Quit( rc )
}

