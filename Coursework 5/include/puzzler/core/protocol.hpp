#ifndef puzzler_core_protocol_hpp
#define puzzler_core_protocol_hpp

// Sigh. You stay classy, windows.
#define NOMINMAX

#include <cstdint>

#include "puzzler/core/connection.hpp"

#include <sstream>
#include <typeinfo>

namespace puzzler{
	
	/* All packets use the following protocol:
		8 byte packet length (including header).
		4 byte command id.
		4 byte sentinel. Can be any value
		[length-20] bytes of packet data
		4 byte sentinel. Must match the original sentinel
	
		The server will indicate any sort of error with a ServerError packet,
		which can be returned for any client packet.
	
	*/
	
	
	class Packet{
	private:
		struct send_context_t{
			uint64_t length;
			uint32_t sentinel;
			uint64_t beginOffset;
		};
	
		send_context_t BeginSend(Connection *pConnection) const
		{
			send_context_t context={
				Length(),
				uint32_t(clock()+rand()),
				pConnection->SendOffset()
			}	;
			while(context.sentinel==0){
			  context.sentinel=uint32_t(clock()+rand());
			}
			
			if(context.length<20)
				throw std::logic_error("Packet::BeginSend - Cannot have a length of less than 20 bytes.");
			
			uint32_t command=CommandId();
			pConnection->Send(context.length);
			pConnection->Send(command);
			pConnection->Send(context.sentinel);
			
			//fprintf(stderr, "Sent[length=%llu, command=%u, sentinel=%u\n", context.length, command, context.sentinel);
			
			return context;
		}
		
		void EndSend(Connection *pConnection, const send_context_t &ctxt) const
		{
			pConnection->Send(ctxt.sentinel);
			
			uint64_t endOffset=pConnection->SendOffset();
			if(endOffset<ctxt.beginOffset)
				throw std::runtime_error("Packet::EndSend - 64-bit offset has wrapped, cannot handle more than 2^64 bytes of data.");
			
			uint64_t sent=endOffset-ctxt.beginOffset;
			if(sent != ctxt.length){
				std::stringstream acc;
				acc<<"Packet::EndSend - Sent data count ("<<sent<<") does not match what we said in header("<<ctxt.length<<") for packet type "<<CommandName();
				throw std::logic_error(acc.str());
			}
		}
		
		static std::shared_ptr<Packet> CreatePacket(uint32_t command);
		
		void operator=(const Packet&); // = delete; // no implementation
		Packet(const Packet &); // = delete;
	protected:
		Packet()
		{}
			
		virtual void SendPayload(Connection *pConnection) const =0;
		virtual void RecvPayload(Connection *pConnection) =0;
		
		virtual uint64_t PayloadLength() const =0;
	public:
		virtual uint32_t CommandId() const=0;

	  virtual const char *CommandName() const=0;
		
		uint64_t Length() const
		{ return PayloadLength()+20; }
	
		void Send(Connection *pConnection) const
		{
			send_context_t ctxt=BeginSend(pConnection);
			SendPayload(pConnection);
			EndSend(pConnection, ctxt);
		}
	
		static std::shared_ptr<Packet> Recv(Connection *pConnection)
		{
			uint64_t length=0;
			uint32_t command=0, sentinelHeader=0, sentinelFooter=0;
			
			uint64_t beginOffset=pConnection->RecvOffset();
			
			pConnection->Recv(length);
			pConnection->Recv(command);
			pConnection->Recv(sentinelHeader);
			
			//fprintf(stderr, "Recvd[length=%llu, command=%u, sentinel=%u\n", length, command, sentinelHeader);
			
			if(length<20)
				throw std::runtime_error("Packet::Recv - Received packet length of < 20 bytes, which is not possible.");
			
			std::shared_ptr<Packet> res=CreatePacket(command);
			
			res->RecvPayload(pConnection);
			
			pConnection->Recv(sentinelFooter);
			if(sentinelHeader!=sentinelFooter)
				throw std::runtime_error("Packet::Recv - trailing sentinel does not match leading sentinel.");
			
			uint64_t endOffset=pConnection->RecvOffset();
			
			if(endOffset < beginOffset)
				throw std::runtime_error("Packet::Recv - Offset has wrapped, we don't support more than 2^64 bytes sent.");
			if(endOffset-beginOffset != length){
			  std::stringstream tmp;
			  tmp<<"Packet::Recv - Received bytes does not match what as said in header, expected="<<length<<", taken=";
			  tmp<<(endOffset-beginOffset)<<", suggest checking SendPayload/RecvPayload of ";
			  tmp<<typeid(res).name();
			  throw std::runtime_error(tmp.str());
			}
			
			return res;
		}
	};
	
	enum{
		Command_any=0,	// Not a valid packet, used as a wildcard
		
		Command_Error=1,
		
		Command_ClientBeginConnect=2,
		Command_ServerCompleteConnect=3,

		Command_Disconnect=4
	};
	
	/*! After this packet is sent the connection is effectively shut, no further traffic is possible */
	class Packet_Error
		: public Packet
	{
	protected:
		virtual void RecvPayload(Connection *pConnection) override
		{
			pConnection->Recv(errorMessage);
		}

		virtual void SendPayload(Connection *pConnection) const override
		{
			pConnection->Send(errorMessage);
		}		
		
		virtual uint64_t PayloadLength() const override
		{ return 4+errorMessage.size(); }
	public:
		virtual uint32_t CommandId() const override
		{ return Command_Error; }

	  	  virtual const char *CommandName() const override
	  { return "Error"; }
	
		std::string errorMessage;	// ASCII string identifying the error
	};
	
	class Packet_ClientBeginConnect
		: public Packet
	{
	protected:
		virtual void RecvPayload(Connection *pConnection)
		{
			pConnection->Recv(clientId);
			pConnection->Recv(clientClass);
		}	
	
		virtual void SendPayload(Connection *pConnection) const
		{
			pConnection->Send(clientId);
			pConnection->Send(clientClass);
		}
		
		virtual uint64_t PayloadLength() const override
		{ return 8+clientId.size()+clientClass.size(); }
	
	public:
		Packet_ClientBeginConnect()
		{}
	
		Packet_ClientBeginConnect(std::string _clientId, std::string _clientClass)
			: protocolVersion(0)
			, clientId(_clientId)
			, clientClass(_clientClass)
		{	
		}
	
		virtual uint32_t CommandId() const override
		{ return Command_ClientBeginConnect; }

	  	  virtual const char *CommandName() const override
	  { return "ClientBeginConnect"; }
	
		uint32_t protocolVersion;	// Indicates the level of protocol supported by the client
		std::string clientId;		// ASCII string identifying the client (the person)
		std::string clientClass;	// ASCII string identifying the program (the piece of code)
	};
	
	class Packet_ServerCompleteConnect
		: public Packet
	{
	protected:
		virtual void RecvPayload(Connection *pConnection)
		{
			pConnection->Recv(protocolVersion);
			pConnection->Recv(serverId);
			pConnection->Recv(serverClass);
		}
	
		virtual void SendPayload(Connection *pConnection) const
		{
			pConnection->Send(protocolVersion);
			pConnection->Send(serverId);
			pConnection->Send(serverClass);
		}
		
		virtual uint64_t PayloadLength() const override
		{ return 4+4+serverId.size()+4+serverClass.size(); }
	public:
		Packet_ServerCompleteConnect(std::string _serverId="<invalid>", std::string _serverClass="<invalid>")
			: protocolVersion(0)
			, serverId(_serverId)
			, serverClass(_serverClass)
		{}
	
		virtual uint32_t CommandId() const override
		{ return Command_ServerCompleteConnect; }

	  virtual const char *CommandName() const override
	  { return "ServerCompleteConnect"; }
		
		uint32_t protocolVersion;	// Indicates the protocol level being used (wil be <= level indicated by client). Network order.
		std::string serverId;		// ASCII string identifying server identity
		std::string serverClass;	// ASCII string identifying the server program
	};
	
	
	/*! Gracefully disconnect from either side. No further traffic is possible after this is sent */
	class Packet_Disconnect
		: public Packet
	{
	protected:
		virtual void RecvPayload(Connection *pConnection) override
		{
			pConnection->Recv(reason);
		}

		virtual void SendPayload(Connection *pConnection) const override
		{
			pConnection->Send(reason);
		}		
		
		virtual uint64_t PayloadLength() const override
		{ return 4+reason.size(); }
	public:
		Packet_Disconnect()
			: reason("No way, I don't need it, I don't need your love to disconnect")
		{}
	
		virtual uint32_t CommandId() const override
		{ return Command_Disconnect; }

	  virtual const char *CommandName() const override
	  { return "Disconnect"; }
	
		std::string reason;	// ASCII string explaining why disconnect happened
	};
	
	
	std::shared_ptr<Packet> Packet::CreatePacket(uint32_t command)
	{
		switch(command){
		case Command_Error:
			return std::make_shared<Packet_Error>();
		case	Command_ClientBeginConnect:
			return std::make_shared<Packet_ClientBeginConnect>();
		case Command_ServerCompleteConnect:
			return std::make_shared<Packet_ServerCompleteConnect>();
		case Command_Disconnect:
			return std::make_shared<Packet_Disconnect>();
		default:
			{
				std::stringstream acc;
				acc<<"Packet::CreatePacket - Received a packet command id ("<<command<<") that isn't understood.";
				throw std::runtime_error(acc.str());
			}
		};
	}


}; // puzzle

#endif
