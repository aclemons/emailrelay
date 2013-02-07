#
## Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
## 
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

.SUFFIXES: .css_ .css .png_ .png

.css_.css:
	cmd /c copy $*.css_ $*.css

.png_.png:
	cmd /c copy $*.png_ $*.png

all: emailrelay.css emailrelay-doxygen.css gsmtp-classes.png gnet-classes.png sequence-3.png gnet-client.png gsmtp-serverprotocol.png auth.png

#emailrelay.css: emailrelay.css_
#emailrelay-doxygen.css: emailrelay-doxygen.css_
#gsmtp-classes.png: gsmtp-classes.png_
#gnet-classes.png: gnet-classes.png_
#sequence-3.png: sequence-3.png_
#gnet-client.png: gnet-client.png_
#gsmtp-serverprotocol.png: gsmtp-serverprotocol.png_
#auth.png: auth.png_

