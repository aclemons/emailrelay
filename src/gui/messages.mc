
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
 Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
 Warning=0x2:STATUS_SEVERITY_WARNING
 Error=0x3:STATUS_SEVERITY_ERROR
)

FacilityNames=(Application=0x0:FACILITY_APPLICATION
)

MessageId=0x1
SymbolicName=CATEGORY_1
Language=English
General
.

MessageIdTypedef=DWORD

MessageId=1001
Severity=Informational
Facility=Application
SymbolicName=MSG_INFO
Language=English
%1!S!
.

MessageId=1002
Severity=Warning
Facility=Application
SymbolicName=MSG_WARNING
Language=English
%1!S!
.

MessageId=1003
Severity=Error
Facility=Application
SymbolicName=MSG_ERROR
Language=English
%1!S!
.

MessageId=1011
Severity=Informational
Facility=Application
SymbolicName=MSG_INFO
Language=English
%1
.

MessageId=1012
Severity=Warning
Facility=Application
SymbolicName=MSG_WARNING
Language=English
%1
.

MessageId=1013
Severity=Error
Facility=Application
SymbolicName=MSG_ERROR
Language=English
%1
.

