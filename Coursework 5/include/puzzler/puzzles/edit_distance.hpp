#ifndef puzzler_puzzles_edit_distance_hpp
#define puzzler_puzzles_edit_distance_hpp

#include <random>
#include <sstream>
#include <vector>
#include <climits>
#include <cassert>
#include <algorithm>

#include "puzzler/core/puzzle.hpp"

namespace puzzler
{

  class EditDistancePuzzle;
  class EditDistanceInput;
  class EditDistanceOutput;

  class EditDistanceInput
    : public Puzzle::Input
  {
  public:
    std::vector<uint8_t> s;
    std::vector<uint8_t> t;
    
    EditDistanceInput(const Puzzle *puzzle, int scale)
      : Puzzle::Input(puzzle, scale)
    {}

    EditDistanceInput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Input(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override final
    {
      conn.SendOrRecv(s);
      conn.SendOrRecv(t);
    }
  };

  class EditDistanceOutput
    : public Puzzle::Output
  {
  public:
    unsigned distance;

    EditDistanceOutput(const Puzzle *puzzle, const Puzzle::Input *input)
      : Puzzle::Output(puzzle, input)
    {}

    EditDistanceOutput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Output(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override
    {
      conn.SendOrRecv(distance);
    }

  };


  class EditDistancePuzzle
    : public PuzzleBase<EditDistanceInput,EditDistanceOutput>
  {
  protected:
 
    void ReferenceExecute(
			  ILog *log,
			  const EditDistanceInput *pInput,
			  EditDistanceOutput *pOutput
			  ) const
    {
      int m=pInput->s.size();
      int n=pInput->t.size();
      std::vector<uint8_t> s=pInput->s; // Length m
      std::vector<uint8_t> t=pInput->t; // Length n

      std::vector<unsigned> dStg( size_t(m+1)*(n+1) );

      // Helper to provide 2d accesses
      auto d=[&](int i,int j) -> unsigned &
      {
        assert(i<=m && j<=n);
        return dStg.at( i*size_t(n+1)+j );
      };
      

      for(int i=0; i<=m; i++){
        d(i,0) = i;
      }

      for(int j=0; j<=n; j++){
        d(0,j) = j;
      }

      for(int j=1; j<=n; j++){
        log->LogVerbose("Row %u", j);
        for(int i=1; i<=m; i++){
          if( s[i-1] == t[j-1] ){
            d(i,j) = d(i-1,j-1);
          }else{
            d(i,j) = 1 + std::min(std::min( d(i-1,j), d(i,j-1) ), d(i-1,j-1) );
          }
        }
      }
      
      auto distance=d(m,n);

      log->LogInfo("Distance = %u", distance);
      pOutput->distance=distance;
    }

    virtual void Execute(
      ILog *log,
      const EditDistanceInput *input,
      EditDistanceOutput *output
      ) const =0;

  public:
    virtual std::string Name() const override
    { return "edit_distance"; }

    virtual bool HasBitExactOutput() const override
    { return true; }

    virtual bool CompareOutputs(ILog */*log*/, const EditDistanceInput * /*input*/, const EditDistanceOutput *ref, const EditDistanceOutput *got) const override
    {
      return ref->distance==got->distance;
    }

    virtual std::shared_ptr<Input> CreateInput(
					       ILog * /*log*/,
					       int scale
					       ) const override
    {
      std::mt19937 rnd(time(0));  // Not the best way of seeding...
      std::uniform_real_distribution<> udist;
      
      auto params=std::make_shared<EditDistanceInput>(this, scale);

      params->s.resize(scale);
      for(int i=0; i<scale; i++){
        params->s[i]=rnd()%256;
      }

      params->t=params->s;
      for(int i=0; i<(int)sqrt(scale); i++){
        int len=rnd()%10;
        int start=rnd() % (params->t.size()-1);
        len = std::min( len, int(params->t.size()-start)  );
        assert(start+len <= (int)params->t.size() );
        switch(rnd()%3){
        case 0:
          params->t.insert(params->t.begin()+start, len, 0);
          std::generate_n(params->t.begin()+start, len, [&](){ return rnd()%256; });
          break;
        case 1:
          params->t.erase(params->t.begin()+start, params->t.begin()+start+len);
          break;
        case 2:
          std::generate_n(params->t.begin()+start, len, [&](){ return rnd()%256; });
          break;
        default:
          assert(0);
          
        }
      }

      return params;
    }

  };

};

#endif
