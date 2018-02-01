#ifndef puzzle_log_hpp
#define puzzle_log_hpp

#include "puzzler/core/util.hpp"

#include <sstream>

namespace puzzler
{

  enum{
    Log_Fatal,
    Log_Error,
    Log_Info,
    Log_Verbose,
    Log_Debug
  };

  class ILog
  {
  protected:
    int m_logLevel;

    ILog(int logLevel)
      : m_logLevel(logLevel)
    {}

    ILog()
      : m_logLevel(4)
    {}
  public:
    virtual ~ILog()
    {}
    
    int Level() const
    { return m_logLevel; }

    void Log(int level, const char *str, ...)
    {
      if(level <= m_logLevel){
        va_list args;
        va_start(args,str);
        vLog(level, str, args);
        va_end(args);
      }
    }


    /*!
      Low-cost logging - we only evaluate the function if the log
      is enabled, so cost is very low in the no-log case.
      It is also templatised, so the functor should be inlined into
      this function at the call-site, again reducing cost.
    */
    template<class TFunctor>
    void Log(int level, TFunctor f)
    {
      if(level <= m_logLevel){
        std::stringstream tmp;
        f(tmp);
        Log(level, tmp.str().c_str());
      }
    }

    void LogFatal(const char *str, ...)
    {
      if(Log_Fatal <= m_logLevel){
        va_list args;
        va_start(args,str);
        vLog(Log_Fatal, str, args);
        va_end(args);
      }
    }

    void LogError(const char *str, ...)
    {
      if(Log_Error <= m_logLevel){
        va_list args;
        va_start(args,str);
        vLog(Log_Error, str, args);
        va_end(args);
      }
    }

    void LogInfo(const char *str, ...)
    {
      if(Log_Info <= m_logLevel){
        va_list args;
        va_start(args,str);
        vLog(Log_Info, str, args);
        va_end(args);
      }
    }

    void LogVerbose(const char *str, ...)
    {
      if(Log_Verbose <= m_logLevel){
        va_list args;
        va_start(args,str);
        vLog(Log_Verbose, str, args);
        va_end(args);
      }
    }

    void LogDebug(const char *str, ...)
    {
      #ifndef NDEBUG
      if(Log_Debug <= m_logLevel){
        va_list args;
        va_start(args,str);
        vLog(Log_Debug, str, args);
        va_end(args);
      }
      #endif
    }


    virtual void vLog(int level, const char *str, va_list args)=0;
  };

  class LogDest
    : public ILog
  {
  private:
    std::string m_prefix;

    std::string render(const char *str, va_list args)
    {
      std::vector<char> tmp(2000, 0);

      unsigned n=vsnprintf(&tmp[0], tmp.size(), str, args);
      if(n>tmp.size()){
        tmp.resize(n);
        vsnprintf(&tmp[0], tmp.size(), str, args);
      }

      return std::string(&tmp[0]);
    }
  public:
    LogDest(std::string prefix, int logLevel)
      : ILog(logLevel)
      , m_prefix(prefix)
    {}

    virtual void vLog(int level, const char *str, va_list args) override
    {
      if(level<=m_logLevel){
        double t=now()*1e-9;
        std::string msg=render(str, args);
        fprintf(stderr, "[%s], %.2f, %u, %s\n", m_prefix.c_str(), t, level, msg.c_str());
      }
    }
  };

}; // puzzler

#endif
