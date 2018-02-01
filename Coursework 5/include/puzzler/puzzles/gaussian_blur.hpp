#ifndef puzzler_puzzles_gaussian_blur_hpp
#define puzzler_puzzles_gaussian_blur_hpp

#include <random>
#include <sstream>
#include <vector>
#include <climits>

#include "puzzler/core/puzzle.hpp"

namespace puzzler
{

  class GaussianBlurPuzzle;
  class GaussianBlurInput;
  class GaussianBlurOutput;

  class GaussianBlurInput
    : public Puzzle::Input
  {
  public:
    double radius;
    unsigned width, height;
    std::vector<uint8_t> pixels;
    
    GaussianBlurInput(const Puzzle *puzzle, int scale)
      : Puzzle::Input(puzzle, scale)
    {}

    GaussianBlurInput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Input(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override final
    {
      conn.SendOrRecv(radius);
      conn.SendOrRecv(width);
      conn.SendOrRecv(height);
      conn.SendOrRecv(pixels);
    }
  };

  /* This is just a Gaussian blur...
    I'm aware it is broken at the edges, but it is well-define.
    */
  class GaussianBlurOutput
    : public Puzzle::Output
  {
  public:
    std::vector<uint8_t> pixels;

    GaussianBlurOutput(const Puzzle *puzzle, const Puzzle::Input *input)
      : Puzzle::Output(puzzle, input)
    {}

    GaussianBlurOutput(std::string format, std::string name, PersistContext &ctxt)
      : Puzzle::Output(format, name, ctxt)
    {
      PersistImpl(ctxt);
    }

    virtual void PersistImpl(PersistContext &conn) override
    {
      conn.SendOrRecv(pixels);
    }

  };


  class GaussianBlurPuzzle
    : public PuzzleBase<GaussianBlurInput,GaussianBlurOutput>
  {
  protected:

    double coeff(int dx, int dy, double r) const
    {
      return exp(- (dx*dx+dy*dy) / (2*r) ) / (2 * 3.1415926535897932384626433832795 * r);
    }

    void ReferenceExecute(
			  ILog *log,
			  const GaussianBlurInput *pInput,
			  GaussianBlurOutput *pOutput
			  ) const
    {
      pOutput->pixels.resize(pInput->width * pInput->height);

      for(int xOut=0; xOut < (int)pInput->width; xOut++){
        log->LogVerbose("column = %u", xOut);
        for(int yOut=0; yOut < (int)pInput->height; yOut++){
          log->LogDebug("row = %u", yOut);

          double acc=0.0;
          for(int xIn=0; xIn < (int)pInput->width; xIn++){
            for(int yIn=0; yIn < (int)pInput->height; yIn++){
              double contrib = coeff(xIn-xOut, yIn-yOut, pInput->radius);
              acc += contrib * pInput->pixels[yIn*pInput->width+xIn];
            }
          }

          if(acc<0){
            acc=0;
          }else if(acc>255){
            acc=255;
          }

          pOutput->pixels[ pInput->width*yOut+xOut ] = (uint8_t)acc;
        }
      }
    }

    virtual void Execute(
      ILog *log,
      const GaussianBlurInput *input,
      GaussianBlurOutput *output
      ) const =0;

  public:
    virtual std::string Name() const override
    { return "gaussian_blur"; }

    virtual bool HasBitExactOutput() const override
    { return false; }

    virtual bool CompareOutputs(ILog */*log*/, const GaussianBlurInput * /*input*/, const GaussianBlurOutput *ref, const GaussianBlurOutput *got) const override
    {
      for(unsigned i=0; i<ref->pixels.size(); i++){
        double r=ref->pixels[i], g=got->pixels[i];
        if( std::abs(r-g) > 2 ){
          return false;
        }
      }
      return true;
    }

    virtual std::shared_ptr<Input> CreateInput(
					       ILog * /*log*/,
					       int scale
					       ) const override
    {
      std::mt19937 rnd(time(0));  // Not the best way of seeding...
      std::uniform_real_distribution<> udist;
      
      auto params=std::make_shared<GaussianBlurInput>(this, scale);

      params->radius=1+udist(rnd)+std::pow(scale, 0.25);
      params->width=scale;
      params->height=scale;
      params->pixels.resize(params->width*params->height);

      float p1=udist(rnd)*6, p2=udist(rnd)*6;
      float f1=0.05+udist(rnd)*0.05, f2=0.05+udist(rnd)*0.05;

      for(int y=0; y< (int)params->height; y++){
        float v=0.2*sinf(y*f1+p1);
        for(int x=0; x< (int)params->height; x++){
          float h=0.2*sinf(x*f2+p2);
          float r=udist(rnd)*0.1;
          float t=v+h+r;
          t=std::max(0.0f, std::min(1.0f, t));
          params->pixels[y*params->height+x]=(uint8_t)(t*255);
        }
      }

      return params;
    }

  };

};

#endif
