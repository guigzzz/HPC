#ifndef  puzzler_endpoint_server_hpp
#define  puzzler_endpoint_server_hpp

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <vector>
#include <memory>

#include "puzzler/core/protocol.hpp"

namespace puzzler{

class EndpointServer
	: public Endpoint
{
private:
	EndpointServer(EndpointServer &); // = delete;
	void operator =(const EndpointServer &); // = delete;

	uint32_t m_protocol;
	std::string m_serverId, m_serverClass;
	std::string m_clientId, m_clientClass;

public:
	
	EndpointServer(
			std::string serverId,
			std::string serverClass,
			std::unique_ptr<Connection> &conn,
			int logLevel=1
		)
		: Endpoint(conn, std::make_shared<LogDest>(serverId, logLevel))
		, m_serverId(serverId)
		, m_serverClass(serverClass)
	{}
		
	void Run()
	{
		try{
			Log(Log_Info, "Waiting for client, serverId=%s, serverClass=%s\n", m_serverId.c_str(), m_serverClass.c_str());
			auto beginConnect=RecvPacket<Packet_ClientBeginConnect>();
			m_clientId=beginConnect->clientId;
			m_clientClass=beginConnect->clientClass;
			
			Log(Log_Info, "Received connection from clientId=%s, clientClass=%s\n", m_clientId.c_str(), m_clientClass.c_str());		
			
			auto completeConnect = std::make_shared<Packet_ServerCompleteConnect>(m_serverId, m_serverClass);
			SendPacket(completeConnect);
			
			Log(Log_Verbose, "Connected to client.");
			
			auto disconnect = std::make_shared<Packet_Disconnect>();
			SendPacket(disconnect);
			
		}catch(std::exception &e){
			Log(Log_Fatal, "Exception : %s.\n", e.what());
			throw;
		}
	}
};

}; // bitecoin

#endif
