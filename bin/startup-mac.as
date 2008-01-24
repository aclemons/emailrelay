-- runs the 'Resources/Scripts/emailrelay' shell script
do shell script "ps -p$$ -w -w -oppid | tail -1"
set ppid to the result
do shell script "ps -p"&ppid&" -w -w -ocommand | tail -1 | sed 's:/MacOS.*::'"
set base_dir to the result
do shell script "\""&base_dir&"/Resources/Scripts/emailrelay\" || true"
set output_text to the result
if output_text is "" then
	display dialog "Started okay" with title "E-MailRelay" buttons {"OK"}
else
	display dialog output_text with title "Error" buttons {"Cancel"}
end if
