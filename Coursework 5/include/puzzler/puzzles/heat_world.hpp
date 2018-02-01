#ifndef puzzler_puzzles_heat_world_hpp
#define puzzler_puzzles_heat_world_hpp

#include <random>
#include <sstream>
#include <vector>
#include <climits>
#include <iomanip>

#include "puzzler/core/puzzle.hpp"

namespace puzzler
{

  class HeatWorldPuzzle;
  class HeatWorldInput;
  class HeatWorldOutput;

  class HeatWorldInput
    : public Puzzle::Input
  {
  public:
    unsigned n; // width, height. Do n timesteps
    float alpha;
    std::vector<uint32_t> properties;

    std::vector<float> state;

    HeatWorldInput(const Puzzle *puzzle, int scale)
      : Puzzle::Input(puzzle, scale)
    {}

    HeatWorldInput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Input(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override final
    {
      conn.SendOrRecv(n);
      conn.SendOrRecv(alpha);
      conn.SendOrRecv(properties);
      conn.SendOrRecv(state);
    }
  };

  class HeatWorldOutput
    : public Puzzle::Output
  {
  public:
    std::vector<float> state;

    HeatWorldOutput(const Puzzle *puzzle, const Puzzle::Input *input)
      : Puzzle::Output(puzzle, input)
    {}

    HeatWorldOutput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Output(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override
    {
      conn.SendOrRecv(state);
    }

  };


  class HeatWorldPuzzle
    : public PuzzleBase<HeatWorldInput,HeatWorldOutput>
  {
  protected:

   	typedef enum : uint32_t{
      Cell_Fixed			=0x1,
      Cell_Insulator	=0x2
    }cell_flags_t;

    void ReferenceExecute(
			  ILog *log,
			  const HeatWorldInput *pInput,
			  HeatWorldOutput *pOutput
		) const
    {
      unsigned n=pInput->n;

      float outer=pInput->alpha;
      float inner=1-outer/4;

      const auto &properties=pInput->properties;
      auto state=pInput->state;
      std::vector<float> buffer(n*n);

      log->Log(Log_Verbose, [&](std::ostream &dst){
        for(unsigned y=0; y<n; y++){
          for(unsigned x=0; x<n; x++){
            dst<<properties[y*n+x];
          }
          dst<<"\n";
        }
      });

      for(unsigned t=0;t<n;t++){
        log->LogDebug("Time step %d", t);

        for(unsigned y=0;y<n;y++){
          for(unsigned x=0;x<n;x++){
            unsigned index=y*n + x;

            if((properties[index] & Cell_Fixed) || (properties[index] & Cell_Insulator)){
              // Do nothing, this cell never changes (e.g. a boundary, or an interior fixed-value heat-source)
              buffer[index]=state[index];
            }else{
              float contrib=inner;
              float acc=inner*state[index];

              // Cell above
              if(! (properties[index-n] & Cell_Insulator)) {
                contrib += outer;
                acc += outer * state[index-n];
              }

              // Cell below
              if(! (properties[index+n] & Cell_Insulator)) {
                contrib += outer;
                acc += outer * state[index+n];
              }

              // Cell left
              if(! (properties[index-1] & Cell_Insulator)) {
                contrib += outer;
                acc += outer * state[index-1];
              }

              // Cell right
              if(! (properties[index+1] & Cell_Insulator)) {
                contrib += outer;
                acc += outer * state[index+1];
              }

              float res=acc/contrib;

              res=std::min(1.0f, std::max(-1.0f, res));
              buffer[index] = res;
            }
          }
        }

        std::swap(state, buffer);
      }

      log->Log(Log_Verbose, [&](std::ostream &dst){
        dst<<std::fixed<<std::setprecision(2);
        for(unsigned y=0; y<n; y++){
          for(unsigned x=0; x<n; x++){
            dst<<" "<<std::setw(6)<<state[y*n+x];
          }
          dst<<"\n";
        }
      });

      pOutput->state=state;
    }

    virtual void Execute(
      ILog *log,
      const HeatWorldInput *input,
      HeatWorldOutput *output
      ) const =0;

  public:
    virtual std::string Name() const override
    { return "heat_world"; }

    virtual bool HasBitExactOutput() const override
    { return false; }

    virtual bool CompareOutputs(ILog *log, const HeatWorldInput * input, const HeatWorldOutput *ref, const HeatWorldOutput *got) const override
    {
      double acc=0;
      unsigned nn=input->n*input->n;
      for(unsigned i=0; i<nn; i++){
        float err=ref->state[i]-got->state[i];
        acc += err*err;
      }
      double rmse= sqrt(acc / nn);

      // NOTE : I am willing to discuss this in an issue, as long as it
      // is not the night before the deadline. Based on theory and a
      // number of experiments this is quite reasonable (actionally quite generous);
      double expected = 16 * pow(2.0,-24) * sqrt((double)nn);

      log->LogInfo("rmse = %g, expected = %g", rmse, expected);

      return rmse < expected;
    }

    virtual std::shared_ptr<Input> CreateInput(
					       ILog */*log*/,
					       int scale
					       ) const override
    {
      std::mt19937 rnd(time(0));  // Not the best way of seeding...
      std::uniform_real_distribution<> udist;

      auto params=std::make_shared<HeatWorldInput>(this, scale);

      unsigned n=scale;
      params->n=n;
      params->alpha=0.1;

      std::vector<uint32_t> properties(n*n, 0);
      std::vector<float> state(n*n, 0.f);

      auto merge=[&](unsigned x,unsigned y, uint32_t flag=0) {
        assert((x<n) && (y<n));
        properties[y*n+x] |= flag;
        return properties[y*n+x];
      };

      // Four walls
      for(unsigned i=0; i<n ;i++){
        merge(i,  0,   Cell_Insulator);
        merge(i,  n-1, Cell_Insulator);
        merge(0,  i,   Cell_Insulator);
        merge(n-1,i,   Cell_Insulator);
      }

      unsigned h=n*(0.25+udist(rnd)/8);
      unsigned d=n/4-h/2;

      // Two bars
      for(unsigned x=0; x<n-h; x++){
        for(unsigned y=d; y<d+h; y++){
          merge(x,  y, Cell_Insulator);
          merge(n-x-1,  n-y-1, Cell_Insulator);
        }
      }

      // random point sources
      for(unsigned i=0; i<10; i++){
        unsigned x=0, y=0;
        while(merge(x,y)!=0){
          x=rnd()%n;
          y=rnd()%n;
        }
        merge(x,y, Cell_Fixed);
        state[y*n+x]=udist(rnd)*2-1;
      }

      params->properties=properties;
      params->state=state;

      return params;
    }

  };

};

#endif
