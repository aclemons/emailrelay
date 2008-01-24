do shell script "ps -p$$ -w -w -oppid | tail -1"
set ppid to the result
do shell script "ps -p"&ppid&" -w -w -ocommand | tail -1 | sed 's:/MacOS.*::'"
set base_dir to the result
do shell "\""&script base_dir&"/Resources/Scripts/emailrelay\""
set output_text to the result
display dialog output_text with title "E-MailRelay Error" buttons {"OK"}
