#ifndef puzzler_puzzles_mining_hpp
#define puzzler_puzzles_mining_hpp

#include <random>
#include <sstream>
#include <vector>
#include <climits>

#include "puzzler/core/puzzle.hpp"

namespace puzzler
{

  class MiningPuzzle;
  class MiningInput;
  class MiningOutput;

  class MiningInput
    : public Puzzle::Input
  {
  public:
    std::vector<uint32_t> key;
    uint64_t threshold;
    unsigned rounds;

    MiningInput(const Puzzle *puzzle, int scale)
      : Puzzle::Input(puzzle, scale)
    {}

    MiningInput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Input(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override final
    {
      conn.SendOrRecv(threshold);
      conn.SendOrRecv(key);
      conn.SendOrRecv(rounds);
    }
  };

  class MiningOutput
    : public Puzzle::Output
  {
  public:
    uint64_t input;

    MiningOutput(const Puzzle *puzzle, const Puzzle::Input *input)
      : Puzzle::Output(puzzle, input)
    {}

    MiningOutput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Output(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override
    {
      conn.SendOrRecv(input);
    }

  };


  class MiningPuzzle
    : public PuzzleBase<MiningInput,MiningOutput>
  {
  protected:
    static uint64_t TEA_hash (uint64_t v, const uint32_t *k, unsigned rounds) {
      uint32_t v0=v&0xFFFFFFFFull, v1=v>>32, sum=0, i;           /* set up */
      uint32_t delta=0x9e3779b9;                     /* a key schedule constant */
      uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */

      uint64_t res=1234567801234567ull;
      for (i=0; i < rounds; i++) {                       /* basic cycle start */
          sum += delta;
          v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
          v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
          res=((res << 7) ^ (res>>57)) + (v0&v1);
      }     /* end cycle */
      return res;
    }

    void ReferenceExecute(
			  ILog *log,
			  const MiningInput *pInput,
			  MiningOutput *pOutput
			  ) const
    {
      std::mt19937_64 iter(time(0));

      uint64_t best=0xFFFFFFFFFFFFFFFFull;
      while(1){
        uint64_t i=iter();
        uint64_t got=TEA_hash(i, &pInput->key[0], pInput->rounds);
        if(got < pInput->threshold){
          pOutput->input=i;
          break;
        }
        if(got < best){
          log->LogVerbose("Found new best of %llu, ratio=%g", got, pInput->threshold / (double)got);
          best=got;
        }
      }
    }

    virtual void Execute(
      ILog *log,
      const MiningInput *input,
      MiningOutput *output
      ) const =0;

  public:
    virtual std::string Name() const override
    { return "mining"; }

    virtual bool HasBitExactOutput() const override
    { return true; }

    virtual bool CompareOutputs(ILog */*log*/, const MiningInput * input, const MiningOutput */*ref*/, const MiningOutput *got) const override
    {
      uint64_t val=TEA_hash(got->input, &input->key[0], input->rounds);
      return val < input->threshold;
    }

    virtual std::shared_ptr<Input> CreateInput(
					       ILog */*log*/,
					       int scale
					       ) const override
    {
      std::mt19937 rnd(time(0));  // Not the best way of seeding...
      std::uniform_real_distribution<> udist;

      auto params=std::make_shared<MiningInput>(this, scale);

      params->rounds=(unsigned)(4+sqrt(scale));

      params->key.resize(0);
      for(unsigned i=0; i<4; i++){
        params->key.push_back(rnd());
      }

      if(scale <=4){
        params->threshold=0xFFFFFFFFFFFFFFFFull;
      }else{
        params->threshold=(uint64_t)(pow(2.0,64) / (scale*(double)scale));
      }

      return params;
    }

  };

};

#endif
