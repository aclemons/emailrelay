//
// emailrelay-runperl.js
//
// An example JavaScript wrapper that runs a perl script
// for E-MailRelay. The perl script's standard input
// will the be the e-mail's content file. The perl script
// should process this to its standard output, and
// terminate with a zero exit code.
//
// Eg:
//   emailrelay --as-server --filter "cscript.exe //nologo emailrelay-runperl.js"
//
// Uses CMD.EXE which may be not be available on some
// versions of Windows.
//
// Edit the next two lines as necessary, but avoid spaces in paths.
//
var cfg_perl="perl -S -T -w"
var cfg_perl_script="spamassassin"

// parse our command line
var args = WScript.Arguments
var filename = args(0)

// prepare a command using CMD.EXE to do file redirection
var cmd_in = "\"" + filename + "\""
var cmd_out = "\"" + filename + ".tmp\""
var cmd_err = "\"" + filename + ".err\""
var cmd_perl = "cmd /c " + cfg_perl + " " + cfg_perl_script
var cmd = cmd_perl + " < " + cmd_in + " > " + cmd_out + " 2> " + cmd_err

// run the command
var sh = WScript.CreateObject("WScript.Shell")
var rc = sh.Run( cmd , 0 , true )

// check the file redirection
var fs = WScript.CreateObject("Scripting.FileSystemObject")
if( !fs.FileExists(filename+".tmp") || !fs.FileExists(filename+".err") )
{
	WScript.Echo("<<file redirection error>>")
	WScript.Quit( 2 )
}

// success or failure
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


