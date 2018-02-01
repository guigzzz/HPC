#ifndef  puzzle_endpoint_client_hpp
#define  puzzle_endpoint_client_hpp

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <vector>
#include <memory>
#include <map>

#include "puzzler/core/protocol.hpp"
#include "puzzler/core/endpoint.hpp"

namespace puzzler{

class EndpointClient
	: public Endpoint
{
private:
	EndpointClient(EndpointClient &) = delete;
	void operator =(const EndpointClient &) = delete;

	std::string m_clientId, m_clientClass;

public:
	
	EndpointClient(
			std::string clientId,
			std::string clientClass,
			std::unique_ptr<Connection> &conn,
			std::shared_ptr<ILog> &log
		)
		: Endpoint(conn, log)
		, m_clientId(clientId)
		, m_clientClass(clientClass)
	{}
			
	void Run()
	{
		try{
			auto beginConnect=std::make_shared<Packet_ClientBeginConnect>(m_clientId, m_clientClass);
			Log(Log_Info, "Connecting with clientId=%s, clientClass=%s", m_clientId.begin(), m_clientClass.begin());
			SendPacket(beginConnect);
			
			auto endConnect=RecvPacket<Packet_ServerCompleteConnect>();
			Log(Log_Info, "Connected to server=%s, running=%s", endConnect->serverId.c_str(), endConnect->serverClass.c_str());
			
			auto disconnect=RecvPacket<Packet_Disconnect>();
			Log(Log_Info, "Disconnect from server=%s, reason=%s", endConnect->serverId.c_str(), disconnect->reason.c_str());

		}catch(std::exception &e){
			Log(Log_Fatal, "Exception : %s.", e.what());
			throw;
		}
	}
};

}; // bitecoin

#endif
