#ifndef puzzler_core_util_hpp
#define puzzler_core_util_hpp

#if defined(_WIN32) || defined(_WIN64)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#endif

#include <time.h>
#include <cstdio>
#include <cstdarg>

#include <stdarg.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <stdexcept>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#if defined(__CYGWIN__) || !(defined(_WIN32) || defined(_WIN64))
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <unistd.h>
namespace puzzler
{
  class WithBinaryIO
  {
  public:
    WithBinaryIO()
    {}
  };
}
#else
// http://stackoverflow.com/questions/341817/is-there-a-replacement-for-unistd-h-for-windows-visual-c
// http://stackoverflow.com/questions/13198627/using-file-descriptors-in-visual-studio-2010-and-windows
// Note: I could have just included <io.h> and msvc would whinge mightily, but carry on

#include <io.h>
#include <fcntl.h>

#ifndef read
#define read _read
#endif
#ifndef write
#define write _write
#endif
#ifndef STDIN_FILEN
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

namespace puzzler
{
  class WithBinaryIO
  {
  private:
    int m_stdinPrev, m_stdoutPrev;
  public:
    WithBinaryIO()
    {
      fflush(stdout);
      m_stdinPrev=_setmode(_fileno(stdin), _O_BINARY);
      m_stdoutPrev=_setmode(_fileno(stdout), _O_BINARY);
    }

    ~WithBinaryIO()
    {
      fflush(stdout);
      m_stdinPrev=_setmode(_fileno(stdin), _O_BINARY);
      m_stdoutPrev=_setmode(_fileno(stdout), _O_BINARY);
    }
  };
}
#endif


namespace puzzler
{



  /*
    Times are represented using a 64 bit number representing nano-seconds
    from unix epoch (see time() for epoch definition).
  */
  typedef uint64_t timestamp_t;

#if !defined(__CYGWIN__) && defined(_WIN32) || defined(_WIN64)
  inline timestamp_t now()
  {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t tt = ft.dwHighDateTime;
    tt = (tt<<32) + ft.dwLowDateTime;
    tt *=100;
    tt -= 11644473600000000000ULL;
    return tt;;
  }
#elif __MACH__

    inline timestamp_t now()
    {
        struct timespec ts;

        clock_serv_t cclock;
        mach_timespec_t mts;
        host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);
        ts.tv_sec = mts.tv_sec;
        ts.tv_nsec = mts.tv_nsec;

        return uint64_t(1e9*ts.tv_sec+ts.tv_nsec);
    }

#else
  inline timestamp_t now()
  {
    struct timespec ts;
    if(0!=clock_gettime(CLOCK_REALTIME, &ts))
      throw std::runtime_error("puzzle::now() - Couldn't read time.");
    return uint64_t(1e9*ts.tv_sec+ts.tv_nsec);
  }
#endif

  template<class TOut, class TIn>
  static TOut *As(TIn *in)
  {
    TOut *tmp=dynamic_cast<TOut *>(in);
    if(tmp==NULL)
      throw std::runtime_error("Puzzle::As - Couldn't convert pointer to requested type");
    return tmp;
  }

  template<class TOut, class TIn>
  static const TOut *As(const TIn *in)
  {
    const TOut *tmp=dynamic_cast<const TOut*>(in);
    if(tmp==NULL)
      throw std::runtime_error("Puzzle::As - Couldn't convert pointer to requested type");
    return tmp;
  }


}; // puzzler

#endif
