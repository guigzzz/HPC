#ifndef puzzler_core_persist_hpp
#define puzzler_core_persist_hpp

#include "puzzler/core/stream.hpp"

#include <complex>

namespace puzzler{

  class PersistContext;

  class Persistable
  {
  public:
    virtual ~Persistable()
    {}

    virtual void Persist(PersistContext &ctxt) =0;
  };

  class PersistContext
  {
  private:
    bool m_sending;
    Stream *m_pStream;
  public:
    PersistContext(Stream *pStream, bool isSending)
      : m_sending(isSending)
      , m_pStream(pStream)
    {}

    template<class T>
    PersistContext &SendOrRecv(T &x)
    {
      x.persist(*this);
      return *this;
    }

    PersistContext &SendOrRecv(uint32_t &x)
    {
      uint32_t raw=htonl(x);
      if(m_sending){
        m_pStream->Send(4, &raw);
      }else{
        m_pStream->Recv(4, &raw);
      }
      x=ntohl(raw);
      return *this;
    }

    PersistContext &SendOrRecv(uint64_t &x)
    {
      uint32_t hi=(x>>32), lo=uint32_t(x&0xFFFFFFFFUL);
      SendOrRecv(hi);
      SendOrRecv(lo);
      x=(uint64_t(hi)<<32) | lo;
      return *this;
    }

    PersistContext &SendOrRecv(int32_t &x)
    {
      return SendOrRecv(reinterpret_cast<uint32_t&>(x));
    }
    
    PersistContext &SendOrRecv(float &x)
    {
      // This is a bit dangerous - it assumes the endianness of the
      // integer matches the floats. This is true in every architecture
      // I'm aware of. The only other portable way I can think of is
      // to use frexp/ldexp and encode as explicit parts.
      union{
        float d32;
        uint32_t u32;
      }u;
      u.d32=x;
      SendOrRecv(u.u32);
      x=u.d32;
      return *this;
    }

    PersistContext &SendOrRecv(double &x)
    {
      // This is a bit dangerous - it assumes the endianness of the
      // integer matches the floats. This is true in every architecture
      // I'm aware of. The only other portable way I can think of is
      // to use frexp/ldexp and encode as explicit parts.
      union{
        double d64;
        uint64_t u64;
      }u;
      u.d64=x;
      SendOrRecv(u.u64);
      x=u.d64;
      return *this;
    }


    PersistContext &SendOrRecv(std::string &x)
    {
      uint32_t len=x.size();
      SendOrRecv(len);
      if(m_sending){
        m_pStream->Send(len, x.data());
      }else{
        std::vector<char> tmp(len);
        m_pStream->Recv(len, &tmp[0]);
        x.assign(tmp.begin(), tmp.end());
      }
      return *this;
    }

    PersistContext &SendOrRecv(std::string &x, std::string required)
    {
      if(m_sending && x!=required){
	throw std::runtime_error("PersistContext::SendOrRecv - Attempt to send required string which didn't match.");
      }

      std::string tmp=x;
      SendOrRecv(tmp);
      if(tmp!=required){
	throw std::runtime_error("PersistContext::SendOrRecv - Received string which didn't match required string.");
      }
      std::swap(x,tmp);

      return *this;
    }

    template<class A, class B>
    PersistContext &SendOrRecv(std::pair<A,B> &x)
    {
      SendOrRecv(x.first);
      SendOrRecv(x.second);
      return *this;
    }

    PersistContext &SendOrRecv(std::vector<bool> &x)
    {
      uint32_t n=x.size();
      SendOrRecv(n);
      x.resize(n);

      std::vector<uint8_t> backing((n+7)/8, 0);
      if(m_sending){
	for(uint32_t i=0; i<n; i++){
	  if(x[i])
	    backing[i/8] |= 1<<(i%8);
	}
	m_pStream->Send(backing.size(), &backing[0]);
      }else{
	m_pStream->Recv(backing.size(), &backing[0]);
	for(uint32_t i=0; i<n; i++){
	  x[i] = ((backing[i/8] >> (i%8)) & 1) == 1;
	}
      }
      return *this;
    }

    PersistContext &SendOrRecv(std::vector<uint8_t> &x)
    {
      uint32_t n=x.size();
      SendOrRecv(n);
      x.resize(n);
      if(m_sending){
        m_pStream->Send(x.size(), &x[0]);
      }else{
        m_pStream->Recv(x.size(), &x[0]);
      }
      return *this;
    }

    template<class T>
    PersistContext &SendOrRecv(std::vector<T> &x)
    {
      uint32_t n=x.size();
      SendOrRecv(n);
      x.resize(n);
      for(uint32_t i=0; i<n; i++){
	SendOrRecv(x[i]);
      }
      return *this;
    }
    
    template<class T>
    PersistContext &SendOrRecv(std::complex<T> &x)
    {
      T tmp=x.real();
      SendOrRecv(tmp);
      x.real(tmp);
      tmp=x.imag();
      SendOrRecv(tmp);
      x.imag(tmp);
      return *this;
    }

  };

}; // puzzler

#endif
