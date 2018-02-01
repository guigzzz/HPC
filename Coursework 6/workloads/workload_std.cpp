#include "workload.hpp"

#include <string>
#include <random>
#include <cassert>
#include <mutex>

static int n=0;
static int Count=0;

class RNGImpl
{
public:
    virtual ~RNGImpl()
    {}
    
    unif01_Gen m_gen;
    
    virtual std::string name() const=0;
    
    virtual RNGImpl *clone() =0;
};

template<class TRng>
class StdRNG
    : public RNGImpl
{
private:
    TRng m_rng;
    std::string m_name;

public: 
    StdRNG(const std::string &name, const TRng &rng)
        : m_rng(rng)
        , m_name(name)
    {
        m_gen.state=(void*)this;
        m_gen.param=(void*)this;
        m_gen.name=(char *)m_name.c_str();
        m_gen.GetU01=GetU01;
        m_gen.GetBits=GetBits;
        m_gen.Write=Write;
    }
    
    virtual std::string name() const
    {
        return m_name;
    }
    
    virtual void  rename(const std::string &name)
    {
        m_name=name;
        m_gen.name=(char *)m_name.c_str();
    }
    
    virtual void reseed(int i)
    {
        m_rng.seed(i);
    }

    virtual RNGImpl *clone()
    {
        return new StdRNG(m_name, m_rng);
    }
    
    static double GetU01(void *param, void *state)
    {
        static std::uniform_real_distribution<double> u;
        return u(static_cast<StdRNG*>(state)->m_rng);
    }
    
    static unsigned long GetBits(void *param, void *state)
    {
        return (static_cast<StdRNG*>(state))->m_rng() &0xFFFFFFFFul;
    }
    
    static void Write(void *state)
    {
        // Not supported
    }
};

static int g_sel=0;

static RNGImpl *createNext()
{
    ++g_sel;
    std::string num=std::to_string(g_sel);
    RNGImpl *n=0;
    switch(g_sel%3){
    case 0:
    case 1:
        n=new StdRNG<std::mt19937>("mt19937_"+num, std::mt19937(g_sel));
        break;
    case 2:
        n=new StdRNG<std::mt19937_64>("mt19937_64"+num, std::mt19937_64(g_sel));
        break;
    }
    return n;
}

static std::mutex g_mutex;

static RNGImpl *g_curr=0;


void workload_Next()
{
    // Stop potentially racing access to g_curr versus target of c_curr.
    std::lock_guard<std::mutex> lock(g_mutex);

    if(g_curr){
        delete g_curr;
    }
    g_curr=createNext();
}

std::string workload_Name(unif01_Gen *gen)
{
    assert(gen);
    return static_cast<RNGImpl*>(gen->state)->name();
}

unif01_Gen *workload_Create()
{
    // Complete paranoia versus static initialisation of g_curr
    std::lock_guard<std::mutex> lock(g_mutex);

    if(g_curr==0){
        g_curr=createNext();
    }

    RNGImpl *c=g_curr->clone();
    return &c->m_gen;
}

unif01_Gen *workload_Clone(unif01_Gen *gen)
{
    assert(gen);
    RNGImpl *c=static_cast<RNGImpl*>(gen->state)->clone();
    return &c->m_gen;
}

//! Destroy (free) the given RNG instance
void workload_Destroy(unif01_Gen *gen)
{
    assert(gen);
    delete static_cast<RNGImpl*>(gen->state);
}
