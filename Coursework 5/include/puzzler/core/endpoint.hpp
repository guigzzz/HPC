#ifndef  puzzler_endpoint_hpp
#define  puzzler_endpoint_hpp

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <vector>
#include <memory>

#include "puzzler/core/log.hpp"
#include "puzzler/core/protocol.hpp"

namespace puzzler{
	
class Endpoint
	: public ILog
{
private:
	std::unique_ptr<Connection> m_conn;	

	Endpoint(Endpoint &); // = delete;
	void operator =(const Endpoint &); // = delete;

	std::shared_ptr<ILog> m_log;
protected:
	Endpoint(std::unique_ptr<Connection> &conn, std::shared_ptr<ILog> log)
		: m_conn(std::move(conn))
		, m_log(log)
	{}
		
	virtual void vLog(int level, const char *str, va_list args) override
	{
		m_log->vLog(level, str, args);
	}
	
	std::shared_ptr<Packet> RecvPacket(uint32_t commandId=0)
	{
		std::shared_ptr<Packet> packet=Packet::Recv(m_conn.get());
		if(commandId!=0){
			if(commandId!=packet->CommandId())
				throw std::runtime_error("Endpoint::RecvPacket - Expected packet of one type, but got something different.");
		}
		return packet;
	}
	
	template<class T>
	std::shared_ptr<T> RecvPacket()
	{
		std::shared_ptr<Packet> packet=Packet::Recv(m_conn.get());
		
		std::shared_ptr<T> res=std::dynamic_pointer_cast<T>(packet);
		if(!res)
			Throw<std::runtime_error>()<<"Endpoint::RecvPacket - Expected packet of type "<<typeid(T).name()<<" but got "<<typeid(*packet).name()<<".";
		return res;
	}
	
	void SendPacket(const std::shared_ptr<Packet> &packet)
	{
		packet->Send(m_conn.get());
	}
public:
};

}; // puzzle

#endif
