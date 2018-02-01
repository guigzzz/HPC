#ifndef puzzler_puzzles_hold_time_hpp
#define puzzler_puzzles_hold_time_hpp

#include <random>
#include <sstream>
#include <vector>
#include <climits>

#include "puzzler/core/puzzle.hpp"

namespace puzzler
{

  class HoldTimePuzzle;
  class HoldTimeInput;
  class HoldTimeOutput;

  class HoldTimeInput
    : public Puzzle::Input
  {
  public:
    // All nodes with indices less than this have registered outputs. Those with higher indices are combinational
    unsigned flipFlopCount; 

    // There is one vector per ff/gate.
    // The first element for each node is the delay. The following are the source node indices.
    std::vector<std::vector<unsigned> > nodes;

    HoldTimeInput(const Puzzle *puzzle, int scale)
      : Puzzle::Input(puzzle, scale)
    {}

    HoldTimeInput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Input(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override final
    {
      conn.SendOrRecv(flipFlopCount);
      conn.SendOrRecv(nodes);
    }
  };

  class HoldTimeOutput
    : public Puzzle::Output
  {
  public:
    unsigned minDelay;

    HoldTimeOutput(const Puzzle *puzzle, const Puzzle::Input *input)
      : Puzzle::Output(puzzle, input)
    {}

    HoldTimeOutput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Output(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override
    {
      conn.SendOrRecv(minDelay);
    }

  };


  /* This puzzle finds the fastest possible path between any pair of flip-flops.
    This will be part of the timing analysis, and is used to check for
    hold-time violations, i.e. is the data-path too fast.
    
    Usually we worry more about setup-time violations, which is what limits
    the maximum clock rate, but setup-time violations also need to be considered.
  */
  class HoldTimePuzzle
    : public PuzzleBase<HoldTimeInput,HoldTimeOutput>
  {
  protected:

    unsigned path_delay(unsigned ffCount, const std::vector<std::vector<unsigned> > &nodes, unsigned targetFF, unsigned currNode) const
    {
      // We use UINT_MAX as a sentinal value to indicate that no path was found
      unsigned minDist=UINT_MAX;

      // Don't look at the first element, as that is the delay
      for(unsigned i=1; i < nodes[currNode].size(); i++){
        unsigned localDist;

        // Follow each incoming wire
        unsigned src=nodes[currNode][i];
        if(targetFF==src){
          localDist=0; // Found it!
        }else if(src < ffCount){
          localDist=UINT_MAX; // Didn't find it, but still hit a flip-flop
        }else{
          localDist=path_delay(ffCount, nodes, targetFF, src);
        }

        if(localDist!=UINT_MAX){
          minDist=std::min(minDist, localDist);
        }
      }

      if(minDist==UINT_MAX){
        return minDist;
      }else{
        unsigned localDelay=nodes[currNode][0];
        return localDelay+minDist; 
      }
    }
  
    void ReferenceExecute(
			  ILog *log,
			  const HoldTimeInput *pInput,
			  HoldTimeOutput *pOutput
			  ) const
    {
      unsigned minDelay=UINT_MAX;

      for(unsigned srcFF=0; srcFF < pInput->flipFlopCount; srcFF++){
        for(unsigned dstFF=0; dstFF < pInput->flipFlopCount; dstFF++){

          log->LogVerbose("Checking path %u <- %u", dstFF, srcFF);
          unsigned localDistance=path_delay( pInput->flipFlopCount, pInput->nodes, dstFF, srcFF);

          if(localDistance < minDelay){
            log->LogInfo("New min distance = %u", localDistance);
            minDelay=localDistance;
          }
          
        }
      }

      log->LogInfo("Min delay = %u", minDelay);
      pOutput->minDelay=minDelay;
    }

    virtual void Execute(
      ILog *log,
      const HoldTimeInput *input,
      HoldTimeOutput *output
      ) const =0;

  public:
    virtual std::string Name() const override
    { return "hold_time"; }

    virtual bool HasBitExactOutput() const override
    { return true; }

    virtual bool CompareOutputs(ILog */*log*/, const HoldTimeInput * /*input*/, const HoldTimeOutput *ref, const HoldTimeOutput *got) const override
    {
      return (ref->minDelay==got->minDelay);
    }

    virtual std::shared_ptr<Input> CreateInput(
					       ILog *log,
					       int scale
					       ) const override
    {
      std::mt19937 rnd(time(0));  // Not the best way of seeding...
      std::uniform_real_distribution<> udist;
      
      auto params=std::make_shared<HoldTimeInput>(this, scale);

      int ffCount=std::max(8, (int)(2+std::pow(scale,0.8)));
      int gateCount=std::max(8, scale);
      int total=ffCount+gateCount;

      params->nodes.resize(total);
      params->flipFlopCount=ffCount;

      for(int i=0; i<ffCount; i++){
        params->nodes[i].push_back(1+(rnd()%4)); // Random gate delay from 1..4

        double u=udist(rnd);
        int src=ffCount + (int)( u*0.25* total );
        params->nodes[i].push_back( src );

        log->Log(Log_Debug, [&]( std::ostream &dst){
          dst<<" FF "<<i<<", delay="<<params->nodes[i][0]<<", src="<<params->nodes[i][1];
        });
      }
      for(int i=ffCount; i<total; i++){
        params->nodes[i].push_back(1+(rnd()%4)); // Random gate delay from 1..4

        int degree=1+(rnd()%3); // Degree from 1..3
        for(int j=0; j<degree; j++){
          double u=udist(rnd);
          int src=i + 1+ (int)( u*0.15* total );
          src=std::min(src, total+ffCount-1); // Avoid combinational loops...
          params->nodes[i].push_back( src % total );
        }

        log->Log(Log_Debug, [&]( std::ostream &dst){
          dst<<" Gate "<<i<<", delay="<<params->nodes[i][0]<<", srcs : ";
          for(unsigned j=1; j<params->nodes[i].size(); j++){
            dst<<" "<<params->nodes[i][j];
          }
        });
      }

      return params;
    }

  };

};

#endif
