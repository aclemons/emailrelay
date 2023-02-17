//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file geventloophandles_st.cpp
///

#include "gdef.h"
#include "geventloophandles.h"

// stubs for no multi-threading...
bool GNet::EventLoopHandlesMt::enabled() noexcept { return false ; }
GNet::EventLoopHandles::EventLoopHandles() = default ;
GNet::EventLoopHandles::~EventLoopHandles() = default ;
GNet::WaitThread::~WaitThread() = default ;
GNet::EventLoopHandlesMt::EventLoopHandlesMt() = default ;
GNet::EventLoopHandlesMt::~EventLoopHandlesMt() = default ;
GNet::EventLoopHandles::Rc GNet::EventLoopHandlesMt::wait( DWORD ) { return {RcType::other} ; }
GNet::EventLoopHandlesMt::HandlePtr GNet::EventLoopHandlesMt::handles() noexcept { return HandlePtr(nullptr) ; }
void GNet::EventLoopHandlesMt::init() {}
void GNet::EventLoopHandlesMt::update( bool , std::size_t ) {}
std::size_t GNet::EventLoopHandlesMt::shuffle( Rc ) { return 0U ; }
bool GNet::EventLoopHandlesMt::overflow( std::size_t ) const noexcept { return false ; }
std::string GNet::EventLoopHandlesMt::help( bool on_add , std::size_t ) const { return {} ; }

