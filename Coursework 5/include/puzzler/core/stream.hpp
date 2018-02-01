#ifndef  puzzler_core_stream_hpp
#define  puzzler_core_stream_hpp

#if defined(_WIN32) || defined(_WIN64)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#endif

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <memory>

#if !defined(__CYGWIN__) && (defined(_WIN32) || defined(_WIN64))
#include "Winsock2.h"
#else
#include <arpa/inet.h>
#endif

namespace puzzler{

  class Stream
  {
  private:
    // No implementation for either
    Stream(const Stream &); // = delete;
    Stream &operator=(const Stream &); // = delete;
  protected:
    Stream()
    {}
  public:
    virtual ~Stream()
    {}

    virtual void Send(size_t cbData, const void *pData) =0;
    virtual void Recv(size_t cbData, void *pData) =0;

    //! Return the current offset from some arbitrary starting point
    virtual uint64_t SendOffset() const =0;
    virtual uint64_t RecvOffset() const =0;
  };

}; // puzzler

#endif
