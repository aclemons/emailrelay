//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glog.h
///

#ifndef G_LOG_H
#define G_LOG_H

#include "gdef.h"
#include "glogstream.h"
#include "glogoutput.h"

#define G_LOG_IMP( expr , severity ) do { if(G::LogOutput::Instance::at(severity)) { auto log_stream = G::LogOutput::Instance::start((severity),__FILE__,__LINE__) ; log_stream << expr ; G::LogOutput::Instance::output(log_stream) ; } } while(0) /* NOLINT bugprone-macro-parentheses */
#define G_LOG_IMP_IF( cond , expr , severity ) do { if(G::LogOutput::Instance::at(severity)&&(cond)) { auto log_stream = G::LogOutput::Instance::start((severity),__FILE__,__LINE__) ; log_stream << expr ; G::LogOutput::Instance::output(log_stream) ; } } while(0) /* NOLINT bugprone-macro-parentheses */
#define G_LOG_IMP_ONCE( expr , severity ) do { static bool done__ = false ; if(!done__&&G::LogOutput::Instance::at(severity)) { auto log_stream = G::LogOutput::Instance::start((severity),__FILE__,__LINE__) ; log_stream << expr ; G::LogOutput::Instance::output(log_stream) ; } done__ = true ; } while(0) /* NOLINT bugprone-macro-parentheses */

#if defined(G_WITH_DEBUG) || ( defined(_DEBUG) && ! defined(G_NO_DEBUG) )
#define G_DEBUG( expr ) G_LOG_IMP( expr , G::LogOutput::Severity::Debug )
#define G_DEBUG_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::LogOutput::Severity::Debug )
#define G_DEBUG_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::LogOutput::Severity::Debug )
#else
#define G_DEBUG( expr )
#define G_DEBUG_IF( cond , expr )
#define G_DEBUG_ONCE( group , expr )
#endif

#if ! defined(G_NO_LOG)
#define G_LOG( expr ) G_LOG_IMP( expr , G::LogOutput::Severity::InfoVerbose )
#define G_LOG_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::LogOutput::Severity::InfoVerbose )
#define G_LOG_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::LogOutput::Severity::InfoVerbose )
#else
#define G_LOG( expr )
#define G_LOG_IF( cond , expr )
#define G_LOG_ONCE( expr )
#endif

#if ! defined(G_NO_LOG_MORE)
#define G_LOG_MORE( expr ) G_LOG_IMP( expr , G::LogOutput::Severity::InfoMoreVerbose )
#define G_LOG_MORE_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::LogOutput::Severity::InfoMoreVerbose )
#define G_LOG_MORE_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::LogOutput::Severity::InfoMoreVerbose )
#else
#define G_LOG_MORE( expr )
#define G_LOG_MORE_IF( cond , expr )
#define G_LOG_MORE_ONCE( expr )
#endif

#if ! defined(G_NO_LOG_S)
#define G_LOG_S( expr ) G_LOG_IMP( expr , G::LogOutput::Severity::InfoSummary )
#define G_LOG_S_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::LogOutput::Severity::InfoSummary )
#define G_LOG_S_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::LogOutput::Severity::InfoSummary )
#else
#define G_LOG_S( expr )
#define G_LOG_S_IF( cond , expr )
#define G_LOG_S_ONCE( expr )
#endif

#if ! defined(G_NO_WARNING)
#define G_WARNING( expr ) G_LOG_IMP( expr , G::LogOutput::Severity::Warning )
#define G_WARNING_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::LogOutput::Severity::Warning )
#define G_WARNING_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::LogOutput::Severity::Warning )
#else
#define G_WARNING( expr )
#define G_WARNING_IF( cond , expr )
#define G_WARNING_ONCE( expr )
#endif

#if ! defined(G_NO_ERROR)
#define G_ERROR( expr ) G_LOG_IMP( expr , G::LogOutput::Severity::Error )
#else
#define G_ERROR( expr )
#endif

#endif
