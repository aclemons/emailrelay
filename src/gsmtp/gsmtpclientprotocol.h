//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsmtpclientprotocol.h
///

#ifndef G_SMTP_CLIENT_PROTOCOL_H
#define G_SMTP_CLIENT_PROTOCOL_H

#include "gdef.h"
#include "gsmtpclientreply.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gfilter.h"
#include "gsaslclient.h"
#include "gsaslclientsecrets.h"
#include "gslot.h"
#include "gstringarray.h"
#include "gstringview.h"
#include "glimits.h"
#include "gtimer.h"
#include "gexception.h"
#include <vector>
#include <memory>
#include <iostream>

namespace GSmtp
{
	class ClientProtocol ;
}

//| \class GSmtp::ClientProtocol
/// Implements the client-side SMTP protocol.
///
class GSmtp::ClientProtocol : private GNet::TimerBase
{
public:
	G_EXCEPTION( NotReady , tx("not ready") ) ;
	G_EXCEPTION( TlsError , tx("tls/ssl error") ) ;
	G_EXCEPTION( BadSelector , tx("no client authentication account") ) ;
	G_EXCEPTION_CLASS( SmtpError , tx("smtp error") ) ;

	class Sender /// An interface used by ClientProtocol to send protocol messages.
	{
	public:
		virtual bool protocolSend( G::string_view , std::size_t offset , bool go_secure ) = 0 ;
			///< Called by the Protocol class to send network data to
			///< the peer.
			///<
			///< The offset gives the location of the payload within the
			///< string-view.
			///<
			///< Returns false if not all of the string was sent due to
			///< flow control. In this case ClientProtocol::sendComplete() should
			///< be called as soon as the full string has been sent.
			///<
			///< Throws on error, eg. if disconnected.

		virtual ~Sender() = default ;
			///< Destructor.
	} ;

	struct Config /// A structure containing GSmtp::ClientProtocol configuration parameters.
	{
		std::string thishost_name ; // EHLO parameter
		unsigned int response_timeout {0U} ;
		unsigned int ready_timeout {0U} ;
		bool use_starttls_if_possible {false} ;
		bool must_use_tls {false} ;
		bool authentication_fallthrough {false} ; // try MAIL FROM even if authentication failed
		bool anonymous{false} ; // MAIL..AUTH=
		bool must_accept_all_recipients {false} ;
		bool eightbit_strict {false} ; // fail 8bit messages to non-8bitmime server
		bool binarymime_strict {false} ; // fail binarymime messages to non-chunking server
		bool smtputf8_strict {false} ; // fail utf8 mailbox names via non-smtputf8 server
		bool pipelining {false} ; // send mail-to and all rcpt-to commands together
		std::size_t reply_size_limit {G::Limits<>::net_buffer} ; // sanity check
		std::size_t bdat_chunk_size {1000000} ; // n, TPDU size N=n+7+ndigits, ndigits=(int(log10(n))+1)
		bool crlf_only {false} ; // CR-LF line endings, not as loose as RFC-2821 2.3.7
		bool try_reauthentication {false} ; // try a new EHLO and AUTH if the client account changes
		Config() ;
		Config & set_thishost_name( const std::string & ) ;
		Config & set_response_timeout( unsigned int ) noexcept ;
		Config & set_ready_timeout( unsigned int ) noexcept ;
		Config & set_use_starttls_if_possible( bool = true ) noexcept ;
		Config & set_must_use_tls( bool = true ) noexcept ;
		Config & set_authentication_fallthrough( bool = true ) noexcept ;
		Config & set_anonymous( bool = true ) noexcept ;
		Config & set_must_accept_all_recipients( bool = true ) noexcept ;
		Config & set_eightbit_strict( bool = true ) noexcept ;
		Config & set_binarymime_strict( bool = true ) noexcept ;
		Config & set_smtputf8_strict( bool = true ) noexcept ;
		Config & set_pipelining( bool = true ) noexcept ;
		Config & set_reply_size_limit( std::size_t ) noexcept ;
		Config & set_crlf_only( bool = true ) noexcept ;
		Config & set_try_reauthentication( bool = true ) noexcept ;
	} ;

	struct DoneInfo /// Parameters for GSmtp::ClientProtocol::doneSignal()
	{
		int response_code ; // smtp result code, or 0 for an internal non-smtp error, or -1 for filter-abandon, or -2 for a filter-fail
		std::string response ; // response text, empty iff sent successfully
		std::string reason ; // additional reason text (cf. GSmtp::Filter)
		G::StringArray rejects ; // rejected RCPT-TO addresses
	} ;

	ClientProtocol( GNet::ExceptionSink , Sender & sender ,
		const GAuth::SaslClientSecrets & secrets , const std::string & sasl_client_config ,
		const Config & config , bool in_secure_tunnel ) ;
			///< Constructor. The Sender interface is used to send protocol
			///< messages to the peer. The references are kept.

	G::Slot::Signal<const DoneInfo&> & doneSignal() noexcept ;
		///< Returns a signal that is raised once the protocol has finished
		///< with a given message.
		///<
		///< If 'must_accept_all_recipients' is false and the message was
		///< successfully sent to only some of the recipients then this is
		///< signalled as an error with a non-empty reject list.

	G::Slot::Signal<> & filterSignal() noexcept ;
		///< Returns a signal that is raised when the protocol needs
		///< to do message filtering. The signal callee must call
		///< filterDone() when the filter has finished.

	void start( std::weak_ptr<GStore::StoredMessage> ) ;
		///< Starts transmission of the given message. The doneSignal()
		///< is used to indicate that the message has been processed
		///< and the shared object should remain valid until then.
		///< Precondition: GStore::StoredMessage::toCount() != 0

	void finish() ;
		///< Called after the last message has been sent. Sends a quit
		///< command and shuts down the socket.

	void sendComplete() ;
		///< To be called when a blocked connection becomes unblocked.
		///< See ClientProtocol::Sender::protocolSend().

	void filterDone( Filter::Result result , const std::string & response , const std::string & reason ) ;
		///< To be called when the Filter interface has done its thing.
		///< If the result is Result::ok then the message processing
		///< continues; otherwise the message processing fails with
		///< a doneSignal() code of -1 for Result::abandon or -2 for
		///< Result::fail.

	void secure() ;
		///< To be called when the secure socket protocol has been
		///< successfully established.

	bool apply( const std::string & rx ) ;
		///< Called on receipt of a line of text from the remote server.
		///< Returns true if the protocol is done and the doneSignal()
		///< has been emitted.

public:
	~ClientProtocol() override = default ;
	ClientProtocol( const ClientProtocol & ) = delete ;
	ClientProtocol( ClientProtocol && ) = delete ;
	ClientProtocol & operator=( const ClientProtocol & ) = delete ;
	ClientProtocol & operator=( ClientProtocol && ) = delete ;

private: // overrides
	void onTimeout() override ; // Override from GNet::TimerBase.

private:
	enum class State
	{
		Init ,
		Started ,
		ServiceReady ,
		SentEhlo ,
		SentHelo ,
		Auth ,
		SentMail ,
		Filtering ,
		SentRcpt ,
		SentData ,
		SentDataStub ,
		SentBdatMore ,
		SentBdatLast ,
		Data ,
		SentDot ,
		StartTls ,
		SentTlsEhlo ,
		MessageDone ,
		Quitting
	} ;
	struct ServerInfo
	{
		bool has_starttls {false} ;
		bool has_auth {false} ;
		bool secure {false} ;
		bool has_8bitmime {false} ;
		bool has_binarymime {false} ; // RFC-3030
		bool has_chunking {false} ; // RFC-3030
		bool has_pipelining {false} ;
		bool has_smtputf8 {false} ;
		G::StringArray auth_mechanisms ;
	} ;
	struct MessageState
	{
		std::weak_ptr<GStore::StoredMessage> ptr ;
		std::string id ;
		std::string selector ;
		std::size_t content_size {0U} ;
		std::size_t to_index {0U} ;
		std::size_t to_accepted {0U} ; // count of accepted recipients
		G::StringArray to_rejected ; // list of rejected recipients
		std::size_t chunk_data_size {0U} ;
		std::string chunk_data_size_str ;
	} ;
	struct SessionState
	{
		ServerInfo server ;
		bool secure {false} ;
		bool authenticated {false} ;
		std::string auth_selector ;
		std::string auth_mechanism ;
		bool ok( const std::string & s ) const
		{
			return !authenticated || auth_selector == s ;
		}
	} ;
	struct Protocol
	{
		State state {State::Init} ;
		G::StringArray reply_lines ;
		std::size_t replySize() const ;
	} ;

private:
	using BodyType = GStore::MessageStore::BodyType ;
	GStore::StoredMessage & message() ;
	std::string checkSendable() ;
	bool endOfContent() ;
	bool applyEvent( const ClientReply & event ) ;
	void raiseDoneSignal( int , const std::string & , const std::string & = {} ) ;
	void startFiltering() ;
	static GAuth::SaslClient::Response initialResponse( const GAuth::SaslClient & , G::string_view ) ;
	//
	void sendEot() ;
	void sendCommandLines( const std::string & ) ;
	void sendRsp( const GAuth::SaslClient::Response & ) ;
	void send( G::string_view ) ;
	void send( G::string_view , G::string_view , G::string_view = {} , G::string_view = {} , bool = false ) ;
	std::size_t sendContentLines() ;
	bool sendNextContentLine( std::string & ) ;
	void sendEhlo() ;
	void sendHelo() ;
	bool sendMailFrom() ;
	void sendRcptTo() ;
	bool sendBdatAndChunk( std::size_t , const std::string & , bool ) ;
	//
	bool sendContentLineImp( const std::string & , std::size_t ) ;
	void sendChunkImp( const char * , std::size_t ) ;
	bool sendImp( G::string_view , std::size_t sensitive_from = std::string::npos ) ;

private:
	Sender & m_sender ;
	std::unique_ptr<GAuth::SaslClient> m_sasl ;
	Config m_config ;
	const bool m_in_secure_tunnel ;
	bool m_eightbit_warned {false} ;
	bool m_binarymime_warned {false} ;
	bool m_utf8_warned {false} ;
	G::Slot::Signal<const DoneInfo &> m_done_signal ;
	G::Slot::Signal<> m_filter_signal ;
	Protocol m_protocol ;
	MessageState m_message_state ;
	GStore::StoredMessage * m_message_p {nullptr} ;
	std::vector<char> m_message_buffer ;
	std::string m_message_line ;
	SessionState m_session ;
} ;

inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_thishost_name( const std::string & s ) { thishost_name = s ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_response_timeout( unsigned int t ) noexcept { response_timeout = t ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_ready_timeout( unsigned int t ) noexcept { ready_timeout = t ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_use_starttls_if_possible( bool b ) noexcept { use_starttls_if_possible = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_must_use_tls( bool b ) noexcept { must_use_tls = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_authentication_fallthrough( bool b ) noexcept { authentication_fallthrough = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_anonymous( bool b ) noexcept { anonymous = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_must_accept_all_recipients( bool b ) noexcept { must_accept_all_recipients = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_eightbit_strict( bool b ) noexcept { eightbit_strict = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_binarymime_strict( bool b ) noexcept { binarymime_strict = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_smtputf8_strict( bool b ) noexcept { smtputf8_strict = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_pipelining( bool b ) noexcept { pipelining = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_reply_size_limit( std::size_t n ) noexcept { reply_size_limit = n ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_crlf_only( bool b ) noexcept { crlf_only = b ; return *this ; }
inline GSmtp::ClientProtocol::Config & GSmtp::ClientProtocol::Config::set_try_reauthentication( bool b ) noexcept { try_reauthentication = b ; return *this ; }

#endif
