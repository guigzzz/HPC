#ifndef puzzler_connection_socket_hpp
#define puzzler_connection_socket_hpp

#include "puzzler/core/connection.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <iostream>

namespace puzzler{

namespace detail{
	
class ConnectionOverSocket
	: public Connection
{
private:	
	friend std::unique_ptr<Connection> puzzler::OpenConnection_Socket(std::vector<std::string> &spec);

	int m_socket;
	uint64_t m_sendOffset, m_recvOffset;

	ConnectionOverSocket(int _socket)
		: m_socket(_socket)
		, m_sendOffset(0)
		, m_recvOffset(0)
	{
		if(m_socket==-1)
			throw std::invalid_argument("ConnectionOverSocket - Socket is invalid.");
	}
	
	~ConnectionOverSocket()
	{
		if(m_socket!=-1){
			shutdown(m_socket, 2);
			close(m_socket);
			m_socket=-1;
		}
	}
public:
	std::unique_ptr<Connection> Create(int _socket)
	{
		return std::unique_ptr<Connection>(new ConnectionOverSocket(_socket));
	}

	virtual void Send(size_t cbData, const void *pData) override
	{
		if(cbData > 0x7FFFFFFFUL)
			throw std::logic_error("SendPacket - 64-bit packet lengths not tested.");
		
		const uint8_t *pWrite=(const uint8_t *)pData;
		
		uint64_t todo=cbData;
		while(todo){
			int done=send(m_socket, (char*)pWrite, todo, 0);
			if(done<=0){
				int e=errno;
				std::stringstream acc;
				acc<<"Send - Received error while reading writing to socket ("<<e<<" = "<<strerror(e)<<")";
				throw std::runtime_error(acc.str());
			}
			m_sendOffset+=done;
			pWrite += done;
			todo -= done;
		}
	}
	
	virtual void Recv(size_t cbData, void *pData) override
	{
		uint8_t *pRead=(uint8_t*)pData;
		
		uint64_t todo=cbData;
		while(todo){
			int done=recv(m_socket, (char*)pRead, todo, 0);
			if(done<=0){
				int e=errno;
				std::stringstream acc;
				acc<<"Recv - Received error while reading reading from socket ("<<e<<" = "<<strerror(e)<<")";
				throw std::runtime_error(acc.str());
			}
			m_recvOffset+=done;
			pRead += done;
			todo -= done;
		}
	}
	
	virtual uint64_t SendOffset() const override
	{ return m_sendOffset; }
	
	virtual uint64_t RecvOffset() const override
	{ return m_recvOffset; }
};

}; // detail

std::unique_ptr<Connection> OpenConnection_Socket(std::vector<std::string> &spec)
{
	if(spec.size()<1)
		throw std::invalid_argument("OpenConnection_Socket - Not enough spec arguments.");
	
	if(spec[0]=="tcp-server"){
		if(spec.size()!=2)
			throw std::invalid_argument("OpenConnection_Socket - Spec should be 'tcp-server portNum'.");
		
		int portNum=atoi(spec[1].c_str());
		fprintf(stderr, "Will listen for connection on port %d\n", portNum);
		
		int sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(sockListen==-1){
			Throw<std::runtime_error>() << "OpenConnection_Socket - Couldn't create socket, errno="<<errno;
		}
		
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family=AF_INET;
		server_addr.sin_port=htons(portNum);
		server_addr.sin_addr.s_addr=INADDR_ANY;
		
		if(bind(sockListen, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
			int e=errno;
			close(sockListen);
			Throw<std::runtime_error>() << "OpenConnection_Socket - Couldn't bind server socket, errno="<<e;
		}
		
		if(listen(sockListen,1)<0){
			int e=errno;
			close(sockListen);
			Throw<std::runtime_error>() << "OpenConnection_Socket - Couldn't listen on server socket, errno="<<e;
		}
		
		struct sockaddr_in client_addr;
		socklen_t client_addr_length = sizeof(client_addr);
		int fd = accept(sockListen, (struct sockaddr *) &client_addr, &client_addr_length);
		if(fd<0){
			int e=errno;
			close(sockListen);
			Throw<std::runtime_error>() << "OpenConnection_Socket - Error on server accept, errno="<<e;
		}	

		// Roll on C++14
		return std::unique_ptr<Connection>(new detail::ConnectionOverSocket(fd));		
		
	}else if(spec[0]=="tcp-client"){
		if(spec.size()!=3)
			throw std::invalid_argument("OpenConnection_Socket - Spec should be 'tcp-client address port'.");
		
		std::string addr=spec[1];
		std::string port=spec[2];
		fprintf(stderr, "Will try to connect to address %s at port %s\n", addr.c_str(), port.c_str());
	
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;    // IPv4 and IPv6
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = IPPROTO_TCP;
		
		struct addrinfo *result;
		int e=getaddrinfo(addr.c_str(), port.c_str(), &hints, &result);
		if(e!=0){
			Throw<std::runtime_error>()<<"OpenConnection_Socket - Resolving addr='"<<addr<<"', port='"<<port<<"', got err="<<gai_strerror(e);
		}
		
		int sock=-1;
		
		struct addrinfo *curr=result;
		while(curr){
			sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
			if (sock == -1)
				continue;
			e=errno;

			if (connect(sock, curr->ai_addr, curr->ai_addrlen) != -1)
				break;                  /* Success */
			e=errno;

			close(sock);
			
			curr=curr->ai_next;
		}		
		freeaddrinfo(result);
		
		if(curr==NULL){
			Throw<std::runtime_error>()<<"OpenConnection_Socket - Couldn't connect, errno="<<e;
		}

		// Roll on C++14
		return std::unique_ptr<Connection>(new detail::ConnectionOverSocket(sock));		
	}else{
		Throw<std::runtime_error>() << "OpenConnection_Socket - Didn't understand spec header '"<<spec[0]<<"'.";
		return 0;
	}
}

}; // puzzler

#endif
