#ifndef  puzzler_core_connection_hpp
#define  puzzler_core_connection_hpp

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include <vector>
#include <memory>

#if defined(_WIN32) || defined(_WIN64)
#include "Winsock2.h"
#else
#include <arpa/inet.h>
#endif

namespace puzzler{
	
class Connection
{
private:	
	// No implementaton for either
	Connection(const Connection &); // = delete;
	Connection &operator=(const Connection &); // = delete;

	void CheckString(unsigned n, const char *data) const
	{
		for(unsigned i=0;i<n;i++){
			if(!(isprint(data[i]) || isspace(data[i])))
				throw std::runtime_error("Connection::CheckString - Attempt to send or receive non printable ASCII character.");
		}
	}
protected:
	Connection()
	{}
public:
	virtual ~Connection()
	{}

	virtual void Send(size_t cbData, const void *pData) =0;
	virtual void Recv(size_t cbData, void *pData) =0;
	
	//! Return the current offset from some arbitrary starting point		
	virtual uint64_t SendOffset() const =0;
	virtual uint64_t RecvOffset() const =0;
		
	void Send(uint32_t val)
	{
		val=htonl(val);
		Send(4, &val);
	}
	
	void Recv(uint32_t &val)
	{
		Recv(4, &val);
		val=ntohl(val);
	}
	
	void Send(uint64_t val)
	{
		Send(uint32_t(val>>32));
		Send(uint32_t(val&0xFFFFFFFFull));
	}
	
	void Recv(uint64_t &val)
	{
		uint32_t hi=0, lo=0;
		Recv(hi);
		Recv(lo);
		val=(((uint64_t)hi)<<32) | lo;
	}
	
	void Send(const std::string &val)
	{
		uint32_t length=val.size();
		if(length!=val.size())
			throw std::runtime_error("Connection::Send - String has more than 2^32 characters.");
		CheckString(val.size(), val.data());
		Send(uint32_t(val.size()));
		Send(val.size(), val.data());
	}
	
	void Recv(std::string &val)
	{
		uint32_t length=0;
		Recv(length);
		std::vector<char> tmp(length, 0);
		Recv(length, &tmp[0]);
		CheckString(length, &tmp[0]);
		val=std::string(tmp.begin(), tmp.end());
	}
	

	
	template<class T>
	void Send(const T &x)
	{
		x.Send(this);
	}
	
	template<class T>
	void Recv(T &x)
	{
		x.Recv(this);
	}	
	
	void Send(const std::vector<uint8_t> &v)
	{
		uint32_t length=v.size();
		if(length!=v.size())
			throw std::invalid_argument("Connection::Send - Vector has more than 32 elements.");
		Send(length);
		Send(length, &v[0]);
	}
	
	void Recv(std::vector<uint8_t> &v)
	{
		uint32_t length=0;
		Recv(length);
		v.resize(length);
		Recv(length, &v[0]);
	}
	
	template<class T>
	void Send(const std::vector<T> &v)
	{
		uint32_t length=v.size();
		if(length!=v.size())
			throw std::invalid_argument("Connection::Send - Vector has more than 32 elements.");
		Send(length);
		for(unsigned i=0;i<length;i++){
			Send(v[i]);
		}
	}
	
	template<class T>
	void Recv(std::vector<T> &v)
	{
		uint32_t length=0;
		Recv(length);
		v.resize(length);
		for(unsigned i=0;i<length;i++){
			Recv(v[i]);
		}
	}
};

std::unique_ptr<Connection> OpenConnection_File(std::vector<std::string> &spec);
std::unique_ptr<Connection> OpenConnection_Socket(std::vector<std::string> &spec);

std::unique_ptr<Connection> OpenConnection(std::vector<std::string> &spec)
{
	if(spec.size()==0)
		throw std::invalid_argument("OpenConnection - Empty connection string.");
	
	if(spec[0]=="file"){
		return OpenConnection_File(spec);
	}else if(spec[0]=="tcp-server" || spec[0]=="tcp-client"){
		return OpenConnection_Socket(spec);
	}else{
		throw std::invalid_argument("OpenConnection - Didn't understand connection header '"+spec[0]+"'.");
	}
}

	namespace detail{
		
		template<class TException>
		class throw_helper
		{
		private:	
			bool m_valid;
			std::stringstream m_acc;
		
			throw_helper(const throw_helper &src);	// = delete;
			void operator = (const throw_helper &src); // = delete;	
		public:
			throw_helper()
				: m_valid(true)
			{}	
		
			throw_helper(throw_helper &&src)
				: m_valid(src.m_valid)
			{
				src.m_valid=false;
			}
			
			template<class T>
			std::stringstream &operator<<(const T &x)
			{
				m_acc<<x;
				return m_acc;
			}
			
			~throw_helper()
			{
				if(m_valid)
					throw TException(m_acc.str());
				m_valid=false;
			}
		};
	};

	template<class T>
	detail::throw_helper<T> Throw()
	{ return detail::throw_helper<T>(); }

}; // puzzler

#endif

#include "connections/connection_file.hpp"
#include "connections/connection_socket.hpp"

