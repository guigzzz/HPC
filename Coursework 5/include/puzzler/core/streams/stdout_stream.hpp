#ifndef  puzzler_core_streams_stdout_hpp
#define  puzzler_core_streams_stdout_hpp

#include "puzzler/core/stream.hpp"

namespace puzzler{

  class StdoutStream
    : public Stream
  {
  private:
    // No implementation for either
    StdoutStream(const StdoutStream &); // = delete;
    StdoutStream &operator=(const StdoutStream &); // = delete;

    uint64_t m_offset;

    WithBinaryIO m_withBinary;
  public:
    StdoutStream()
      : m_offset(0)
    {}

    virtual void Send(size_t cbData, const void *pData)
    {
      do{
        int sent=write(STDOUT_FILENO, pData, cbData);
        if(sent==0)
          throw std::runtime_error("StdoutStream::Send - No data was written.");
        if(sent<0)
          throw std::runtime_error("StdoutStream::Send - Error while writing");
        m_offset+=sent;
        cbData-=sent;
        pData=sent+(uint8_t*)pData;
      }while(cbData>0);
    }

    virtual void Recv(size_t , void *)
    {
      throw std::runtime_error("StdoutStream::Recv - no such operation.");
    }

    //! Return the current offset from some arbitrary starting point
    virtual uint64_t SendOffset() const
    { return m_offset; }

    virtual uint64_t RecvOffset() const
    { return 0; }
  };

}; // puzzler

#endif
