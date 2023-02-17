#!/usr/bin/env python3
#
# Copyright (C) 2020-2023 <richardwvm@users.sourceforge.net>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
# ===
#
# emailrelay-ldap-verify.py
#
# Example address verifier script using LDAP.
#
# See also: https://www.python-ldap.org/en/python-ldap-3.3.0
#

import sys
import ldap

try:
	Arg1 = sys.argv[1]
except:
	print("error")
	print("Usage: emailrelay-ldap-verify.py [--emailrelay-version | <email-address>]")
	sys.exit(3)

if Arg1 == "--emailrelay-version":
	print("2.0")
	sys.exit(0)

AtChar = Arg1.find("@")

if AtChar == -1:
	print("invalid mailbox")
	print("malformed e-mail address (no at sign)")
	sys.exit(2)

LocalDomain = "@domain.co.uk"

if Arg1[AtChar:] != LocalDomain:
	print("invalid mailbox")
	print("invalid mailbox: %s" % Arg1)
	sys.exit(2)

LDAPSServer = "ldaps-server.domain.com"
LDAPSPort = "636"
LDAPSUsername = "DOMAIN\\username"
LDAPSPassword = "password"
UserSearchPath = "CN=Users,DC=domain,DC=com"
PublicFolderSearchPath = "CN=Microsoft Exchange System Objects,DC=domain,DC=com"
ErrorLevel = 1

try:
	connect = ldap.initialize("ldaps://" + LDAPSServer + ":" + LDAPSPort)
	connect.set_option(ldap.OPT_NETWORK_TIMEOUT, 3)
	connect.set_option(ldap.OPT_REFERRALS, 0)
	connect.set_option(ldap.OPT_X_TLS_REQUIRE_CERT, ldap.OPT_X_TLS_NEVER)
	connect.set_option(ldap.OPT_X_TLS_NEWCTX, 0)
	connect.simple_bind_s(LDAPSUsername, LDAPSPassword)
	result = connect.search_s(UserSearchPath,
														ldap.SCOPE_SUBTREE,
														"(&(objectCategory=person)(objectclass=user)(!(userAccountControl:1.2.840.113556.1.4.803:=2))(proxyAddresses=SMTP:%s))" % (sys.argv[1]),
														["mail"])
	if len(result) != 1:
		result = connect.search_s(PublicFolderSearchPath,
															ldap.SCOPE_SUBTREE,
															"(&(objectclass=publicFolder)(proxyAddresses=SMTP:%s))" % (sys.argv[1]),
															["mail"])
		if len(result) != 1:
			print("invalid mailbox")
			print("invalid mailbox: %s" % (sys.argv[1]))
			ErrorLevel = 2
		else:
			print()
			print(sys.argv[1])
	else:
		print()
		print(sys.argv[1])
except ldap.LDAPError as e:
	print("temporary error")
	print("ldap error: %s" % str(e))
	ErrorLevel = 3
except:
	print("temporary error")
	print("exception")
	ErrorLevel = 3

sys.exit(ErrorLevel)
