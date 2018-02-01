#ifndef puzzler_core_puzzle_hpp
#define puzzler_core_puzzle_hpp

#include "puzzler/core/persist.hpp"
#include "puzzler/core/log.hpp"
#include <map>
#include <iostream>

namespace puzzler
{

  class Puzzle
  {
  public:

    class Input
      : public virtual Persistable
    {
    private:
      std::string m_format;
      std::string m_puzzleName;
      uint32_t m_scale;
      uint32_t m_serial;
    protected:
      Input(const Puzzle *puzzle, int scale)
        : m_format("puzzle.input.v0")
        , m_puzzleName(puzzle->Name())
        , m_scale(scale)
        , m_serial( now() )
      {}

      Input(std::string format, std::string puzzleName, PersistContext &ctxt)
        : m_format(format)
        , m_puzzleName(puzzleName)
      {
        if(format!="puzzle.input.v0")
          throw std::runtime_error("Puzzle::Input - Invalid format string.");
        ctxt.SendOrRecv(m_scale);
        ctxt.SendOrRecv(m_serial);
      }

      virtual void PersistImpl(PersistContext &ctxt) =0;
    public:
      virtual void Persist(PersistContext &ctxt) override final
      {
        ctxt.SendOrRecv(m_format,"puzzle.input.v0");
        ctxt.SendOrRecv(m_puzzleName);
        ctxt.SendOrRecv(m_scale);
        ctxt.SendOrRecv(m_serial);
        PersistImpl(ctxt);
      }


    public:
      std::string PuzzleName() const
      { return m_puzzleName; }

      uint64_t Serial() const
      { return m_serial; }
    };

    class Output
      : public virtual Persistable
    {
    private:
      std::string m_format;
      std::string m_puzzleName;
      uint64_t m_serial;
    protected:
      Output(const Puzzle *puzzle, const Input *input)
        : m_format("puzzle.output.v0")
        , m_puzzleName(puzzle->Name())
        , m_serial(input->Serial())
      {
      }

      Output(std::string format, std::string puzzleName, PersistContext &ctxt)
        : m_format(format)
        , m_puzzleName(puzzleName)
      {
        if(format!="puzzle.output.v0")
          throw std::runtime_error("Puzzle::Output - Invalid format string.");
        ctxt.SendOrRecv(m_serial);
      }

      virtual void PersistImpl(PersistContext &ctxt) =0;
    public:
      virtual void Persist(PersistContext &ctxt) override final
      {
        ctxt.SendOrRecv(m_format, "puzzle.output.v0");
        ctxt.SendOrRecv(m_puzzleName);
        ctxt.SendOrRecv(m_serial);
        PersistImpl(ctxt);
      }

      std::string PuzzleName() const
      { return m_puzzleName; }

      uint64_t Serial() const
      { return m_serial; }
    };

  public:
    //! Unique name for the puzzle
    virtual std::string Name() const=0;

    //! Create input with difficulty scale
    virtual std::shared_ptr<Input> CreateInput(ILog *log, int scale) const=0;

    //! Load a previously generated input
    virtual std::shared_ptr<Input> LoadInput(PersistContext &ctxt) const=0;

    virtual std::shared_ptr<Input> LoadInput(std::string format, std::string name, PersistContext &ctxt) const=0;

    //! Create a class that can hold instances of output
    virtual std::shared_ptr<Output> MakeEmptyOutput(const Input *input) const=0;

    //! Load a previously created output
    virtual std::shared_ptr<Output> LoadOutput(PersistContext &ctxt) const=0;

    virtual std::shared_ptr<Output> LoadOutput(std::string format, std::string name, PersistContext &ctxt) const=0;

    //! "True" version of the solution. It may be slow, but will be correct
    virtual void ReferenceExecute(ILog *log, const Input *pInput, Output *pOutput) const=0;

    //! "Fast" version of the solution, provided by the user
    virtual void Execute(ILog *log, const Input *pInput, Output *pOutput) const=0;

    //! Some puzzles produce bit-exact outputs. Others allow for variation, so there is a space of correct answers
    virtual bool HasBitExactOutput() const=0;

    //! This is used to compare two solutions for the same input
    virtual bool CompareOutputs(ILog *log, const Input *input, const Output *ref, const Output *got) const=0;
  };

  class PuzzleRegistrar
  {
  private:
    static std::map<std::string,std::shared_ptr<Puzzle> > &Registry()
    {
      static std::map<std::string,std::shared_ptr<Puzzle> > registry;
      return registry;
    }
  public:
    static void Register(std::shared_ptr<Puzzle> puzzle)
    {
      if(Registry().find(puzzle->Name())!=Registry().end())
	      throw std::runtime_error("PuzzleRegistrar::Register - There is already a handler for '"+puzzle->Name()+"'");

      Registry()[puzzle->Name()]=puzzle;
    }

    static std::shared_ptr<Puzzle::Input> LoadInput(PersistContext &ctxt)
    {
      std::string format, name;
      ctxt.SendOrRecv(format).SendOrRecv(name);

      auto puzzle=Lookup(name);
      if(!puzzle){
	      throw std::runtime_error("PuzzleRegistrar::LoadInput - No handler for type '"+name+"'");
      }

      return puzzle->LoadInput(format, name, ctxt);
    }

    static std::shared_ptr<Puzzle::Output> LoadOutput(PersistContext &ctxt)
    {
      std::string format, name;
      ctxt.SendOrRecv(format).SendOrRecv(name);

      auto puzzle=Lookup(name);
      if(!puzzle){
	      throw std::runtime_error("PuzzleRegistrar::LoadOutput - No handler for type '"+name+"'");
      }

      return puzzle->LoadOutput(format, name, ctxt);
    }

    static std::shared_ptr<Puzzle> Lookup(std::string name)
    {
      auto it=Registry().find(name);
      if(it==Registry().end())
	      return std::shared_ptr<Puzzle>();
      return it->second;
    }

    static void ListPuzzles()
    {
      auto it=Registry().begin();
      while(it!=Registry().end()){
        std::cout<<it->second->Name()<<"\n";
        ++it;
      }
    }
    
    // ! Provided by the user
    static void UserRegisterPuzzles();
  }; 
  

  template<class TInput,class TOutput>
  class PuzzleBase
    : public Puzzle
  {
  protected:
    virtual void Execute(
			 ILog *log,
			 const TInput *pInput,
			 TOutput *pOutput
			 ) const=0;

    virtual void ReferenceExecute(
        ILog *log,
        const TInput *pInput,
        TOutput *pOutput
      ) const=0;

    virtual bool CompareOutputs(
      ILog *log, 
      const TInput *input,
      const TOutput *ref,
      const TOutput *got
    ) const=0;

  public:
    virtual void Execute(
			 ILog *log,
			 const Input *pInput,
			 Output *pOutput
			 ) const override final
    {
      Execute(log, As<TInput>(pInput), As<TOutput>(pOutput));
    }

    virtual void ReferenceExecute(
				  ILog *log,
				  const Input *pInput,
				  Output *pOutput
				  ) const override final
    {
      ReferenceExecute(log, As<TInput>(pInput), As<TOutput>(pOutput));
    }

    //! Load a previously generated input
    virtual std::shared_ptr<Input> LoadInput(PersistContext &ctxt) const override final
    {
      std::string format, name;
      ctxt.SendOrRecv(format).SendOrRecv(name);
      return std::make_shared<TInput>(format, name, ctxt);
    }

    virtual std::shared_ptr<Input> LoadInput(std::string format, std::string name, PersistContext &ctxt) const override final
    {
      return std::make_shared<TInput>(format, name, ctxt);
    }

    //! Create a class that can hold instances of output
    virtual std::shared_ptr<Output> MakeEmptyOutput(const Input *input) const override final
    {
      return std::make_shared<TOutput>(this, input);
    }

    //! Load a previously created output
    virtual std::shared_ptr<Output> LoadOutput(PersistContext &ctxt) const override final
    {
      std::string format, name;
      ctxt.SendOrRecv(format).SendOrRecv(name);
      return std::make_shared<TOutput>(format, name, ctxt);
    }

    virtual std::shared_ptr<Output> LoadOutput(std::string format, std::string name, PersistContext &ctxt) const override final
    {
      return std::make_shared<TOutput>(format, name, ctxt);
    }

    //! This is used to compare two solutions for the same input
    virtual bool CompareOutputs(ILog *log, const Input *input, const Output *ref, const Output *got) const override final
    {
      return CompareOutputs(log, As<TInput>(input), As<TOutput>(ref), As<TOutput>(got) );
    }

  };
};

#endif
