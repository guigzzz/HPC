#ifndef puzzler_puzzles_random_projection_hpp
#define puzzler_puzzles_random_projection_hpp

#include <random>
#include <sstream>
#include <vector>
#include <climits>

#include "puzzler/core/puzzle.hpp"

namespace puzzler
{

  class RandomProjectionPuzzle;
  class RandomProjectionInput;
  class RandomProjectionOutput;

  class RandomProjectionInput
    : public Puzzle::Input
  {
  public:
    uint32_t n;
    uint32_t seed;
    
    RandomProjectionInput(const Puzzle *puzzle, int scale)
      : Puzzle::Input(puzzle, scale)
    {}

    RandomProjectionInput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Input(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override final
    {
      conn.SendOrRecv(n);
      conn.SendOrRecv(seed);
    }
  };

  class RandomProjectionOutput
    : public Puzzle::Output
  {
  public:
    std::vector<uint64_t> acc;

    RandomProjectionOutput(const Puzzle *puzzle, const Puzzle::Input *input)
      : Puzzle::Output(puzzle, input)
    {}

    RandomProjectionOutput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Output(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override
    {
      conn.SendOrRecv(acc);
    }

  };

  /* Apply a set of random projections to a single input, in order to
     look at the statistics of the output. Something that occurs in
     number theory and design of communication systems.
  */
  class RandomProjectionPuzzle
    : public PuzzleBase<RandomProjectionInput,RandomProjectionOutput>
  {
  protected:
    
    static uint32_t lcg(uint32_t s)
    {
      return s*1664525+1013904223;
    }

    // Make sparse random projection matrix M
    static std::vector<uint32_t> MakeProjection(unsigned n, uint32_t seed, double p)
    {
      std::vector<uint32_t> m(n*n,0);
      
      for(unsigned y=0; y<n; y++){
        for(unsigned x=0; x<n; x++){
          uint32_t r=lcg(seed);
          uint32_t rHi=r>>8, rLo=r&0xFF;
          m[y*n+x] = ( (rHi*std::pow(2.0, -24)) < p) ? rLo : 0;
          seed+=19937;
        }
      }
      
      return m;
    }
    
    // a += m * v
    static void ApplyAndAccProjection(unsigned n, std::vector<uint64_t> &a, const std::vector<uint32_t> &m, const std::vector<uint32_t> &v)
    {
      for(unsigned i=0; i<n; i++){
        for(unsigned j=0; j<n; j++){
          a[i] += m[i*n+j] * v[j];
        }
      }
    }

    void ReferenceExecute(
			  ILog *log,
			  const RandomProjectionInput *pInput,
			  RandomProjectionOutput *pOutput
			  ) const
    {
      unsigned n=pInput->n;
      double p=16.0/n;
      
      std::vector<uint64_t> acc(n, 0);
      
      std::mt19937 rnd(pInput->seed);
    
      log->LogInfo("Creating input vector of size %u", n);
      std::vector<uint32_t> v(n);
      for(unsigned i=0; i<n; i++){
        v[i]=rnd()%2;
      }
      
      log->LogInfo("Beginning projections");
      for(unsigned i=0; i<n; i++){
        log->LogVerbose("Projection %u of %u", i, n);
        auto proj = MakeProjection(n, rnd(), p);
        ApplyAndAccProjection(n, acc, proj, v);
      }
      
      pOutput->acc=acc;
      
      log->LogInfo("Finished");
    }

    virtual void Execute(
      ILog *log,
      const RandomProjectionInput *input,
      RandomProjectionOutput *output
      ) const =0;

  public:
    virtual std::string Name() const override
    { return "random_projection"; }

    virtual bool HasBitExactOutput() const override
    { return true; }

    virtual bool CompareOutputs(ILog */*log*/, const RandomProjectionInput * /*input*/, const RandomProjectionOutput *ref, const RandomProjectionOutput *got) const override
    {
      return ref->acc==got->acc;
    }

    virtual std::shared_ptr<Input> CreateInput(
					       ILog * /*log*/,
					       int scale
					       ) const override
    {
      std::mt19937 rnd(time(0));  // Not the best way of seeding...
      std::uniform_real_distribution<> udist;
      
      auto params=std::make_shared<RandomProjectionInput>(this, scale);

      params->n=scale;
      params->seed=rnd();

      return params;
    }

  };

};

#endif
