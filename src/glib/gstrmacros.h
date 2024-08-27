/*
   Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//
// gstrmacros.h
//

#ifndef G_STR_MACROS_H
#define G_STR_MACROS_H

#ifdef G_STR_IMP
#undef G_STR_IMP
#endif
#ifdef G_STR
#undef G_STR
#endif
#ifdef G_STR_PASTE_IMP
#undef G_STR_PASTE_IMP
#endif
#ifdef G_STR_PASTE
#undef G_STR_PASTE
#endif

#define G_STR_IMP(a) #a
#define G_STR(a) G_STR_IMP(a)
#define G_STR_PASTE_IMP(a,b) a##b
#define G_STR_PASTE(a,b) G_STR_PASTE_IMP(a,b)

#endif
