#ifndef puzzler_connection_file_hpp
#define puzzler_connection_file_hpp

#include "puzzler/core/connection.hpp"

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <errno.h>
#include <sstream>

#include <iostream>

#if defined(__CYGWIN__) || !(defined(_WIN32) || defined(_WIN64))
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <unistd.h>
void set_binary_io()
{}
#else
// http://stackoverflow.com/questions/341817/is-there-a-replacement-for-unistd-h-for-windows-visual-c
// http://stackoverflow.com/questions/13198627/using-file-descriptors-in-visual-studio-2010-and-windows
// Note: I could have just included <io.h> and msvc would whinge mightily, but carry on
	
#include <io.h>
#include <fcntl.h>

#define read _read
#define write _write
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

void set_binary_io()
{
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
}
#endif

namespace puzzler{

namespace detail{
	
class ConnectionOverFile
	: public Connection
{
private:	
	friend std::unique_ptr<Connection> puzzler::OpenConnection_File(std::vector<std::string> &spec);

	int m_fdSend, m_fdRecv;
	uint64_t m_sendOffset, m_recvOffset;

	ConnectionOverFile(int fdSend, int fdRecv)
		: m_fdSend(fdSend)
		, m_fdRecv(fdRecv)
		, m_sendOffset(0)
		, m_recvOffset(0)
	{
		if(fdSend==-1 || fdRecv==-1)
			throw std::invalid_argument("ConnectionOverFile - one of the file descriptors is invalid.");
	}
	
	~ConnectionOverFile()
	{
		close(m_fdSend);
		close(m_fdRecv);
	}
public:
	std::unique_ptr<Connection> Create(int fdSend, int fdRecv)
	{
		return std::unique_ptr<Connection>(new ConnectionOverFile(fdSend, fdRecv));
	}

	virtual void Send(size_t cbData, const void *pData) override
	{
		if(cbData > 0x7FFFFFFFUL)
			throw std::logic_error("SendPacket - 64-bit packet lengths not tested.");
		
		const uint8_t *pWrite=(const uint8_t *)pData;
		
		uint64_t todo=cbData;
		while(todo){
			int done=write(m_fdSend, pWrite, todo);
			if(done<=0){
				int e=errno;
				std::stringstream acc;
				acc<<"Send - Received error while reading writing to file ("<<e<<" = "<<strerror(e)<<")";
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
			int done=read(m_fdRecv, pRead, todo);
			if(done<=0){
				int e=errno;
				std::stringstream acc;
				acc<<"Recv - Received error while reading reading from file ("<<e<<" = "<<strerror(e)<<")";
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

std::unique_ptr<Connection> OpenConnection_File(std::vector<std::string> &spec)
{
	if(spec.size()!=3)
		throw std::runtime_error("OpenConnection_File - Wrong number of components in spec.");
	
	if(spec[0]!="file")
		throw std::runtime_error("OpenConnection_File - First component in spec is not 'file'.");
	
	int fdRecv=-1, fdSend=-1;

	if(spec[2]=="-" || spec[1]=="-"){
	  set_binary_io();
	}
	
	std::cerr<<"Dest = "<<spec[2]<<"\n";
	if(spec[2]=="-"){
		fdSend=dup(STDOUT_FILENO);
	}else{
		std::cerr<<"Opening '"<<spec[2]<<"' for writing\n.";
		fdSend=open(spec[2].c_str(), O_WRONLY|O_BINARY);
		std::cerr<<"Open\n";
	}
	if(fdSend==-1){
		throw std::runtime_error("OpenConnection_File - Couldn't open '"+spec[2]+"' for writing.");
	}
	
	std::cerr<<"Source = "<<spec[1]<<"\n";
	if(spec[1]=="-"){
		fdRecv=dup(STDIN_FILENO);
	}else{
		std::cerr<<"Opening '"<<spec[1]<<"' for reading\n.";
		fdRecv=open(spec[1].c_str(), O_RDONLY|O_BINARY);
		std::cerr<<"Open\n";
	}
	if(fdRecv==-1){
		close(fdSend);
		fdSend=-1;
		throw std::runtime_error("OpenConnection_File - Couldn't open '"+spec[1]+"' for reading.");
	}
	
	// Roll on C++14
	return std::unique_ptr<Connection>(new detail::ConnectionOverFile(fdSend, fdRecv));
}

}; // puzzler

#endif
