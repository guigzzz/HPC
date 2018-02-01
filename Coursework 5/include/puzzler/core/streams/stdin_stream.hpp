#ifndef  puzzler_core_streams_stdin_hpp
#define  puzzler_core_streams_stdin_hpp

#include "puzzler/core/stream.hpp"

namespace puzzler{

  class StdinStream
    : public Stream
  {
  private:
    // No implementation for either
    StdinStream(const StdinStream &); // = delete;
    StdinStream &operator=(const StdinStream &); // = delete;

    uint64_t m_offset;

    WithBinaryIO m_withBinary;
  public:
    StdinStream()
      : m_offset(0)
    {}

    virtual void Send(size_t , const void *)
    {
      throw std::runtime_error("StdoutStream::Recv - no such operation.");
    }

    virtual void Recv(size_t cbData, void *pData)
    {
      int got;
      do{
        got=read(STDIN_FILENO, pData, cbData);
        if(got==0)
          throw std::runtime_error("StdoutStream::Recv - End of file.");
        if(got<0)
          throw std::runtime_error("StdoutStream::Recv - Error while reading.");
        m_offset+=got;
        cbData-=got;
        pData=got+(uint8_t*)pData;
      }while(cbData>0);
    }


    //! Return the current offset from some arbitrary starting point
    virtual uint64_t SendOffset() const
    { return 0; }

    virtual uint64_t RecvOffset() const
    { return m_offset; }
  };

}; // puzzler

#endif
