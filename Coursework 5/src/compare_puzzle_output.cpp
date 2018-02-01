
#include "puzzler/puzzler.hpp"

#include <iostream>


int main(int argc, char *argv[])
{
   puzzler::PuzzleRegistrar::UserRegisterPuzzles();

   if(argc<3){
      fprintf(stderr, "compare_puzzle_output input ref got logLevel\n");
      std::cout<<"Puzzles:\n";
      puzzler::PuzzleRegistrar::ListPuzzles();
      exit(1);
   }

   try{
      std::string inputName=argv[1];
      std::string refName=argv[2];
      std::string gotName=argv[3];

      // Control how much is being output.
      // Higher numbers give you more info
      int logLevel=2;
      if(argc>3){
          logLevel = atoi(argv[4]);
          fprintf(stderr, "LogLevel = %s -> %d\n", argv[4], logLevel);
      }
      
      std::shared_ptr<puzzler::ILog> logDest=std::make_shared<puzzler::LogDest>("execute_puzzle", logLevel);
      logDest->Log(puzzler::Log_Info, "Created log.");

      logDest->LogInfo("Loading input %s", inputName.c_str());
      std::shared_ptr<puzzler::Puzzle::Input> input;
      {
        puzzler::FileInStream src(inputName);
        puzzler::PersistContext ctxt(&src, false);

        input=puzzler::PuzzleRegistrar().LoadInput(ctxt);
     }

     std::string puzzleName=input->PuzzleName();

     logDest->LogInfo("Creating puzzle %s to match input", puzzleName.c_str());
     auto puzzle=puzzler::PuzzleRegistrar().Lookup(puzzleName);

      logDest->LogInfo("Loading reference %s", refName.c_str());
      std::shared_ptr<puzzler::Puzzle::Output> ref;
      {
         puzzler::FileInStream src(refName);
         puzzler::PersistContext ctxt(&src, false);

         ref=puzzler::PuzzleRegistrar().LoadOutput(ctxt);
      }
      if(ref->PuzzleName() != puzzleName){
        logDest->LogFatal("Reference output is for a different puzzle type."); 
        exit(1);
      }
      if(ref->Serial() != input->Serial()){
        logDest->LogFatal("Reference output is for a different input."); 
        exit(1);
      }
      
      logDest->LogInfo("Loading got %s", gotName.c_str());
      std::shared_ptr<puzzler::Puzzle::Output> got;
      {
         puzzler::FileInStream src(gotName);
         puzzler::PersistContext ctxt(&src, false);

         got=puzzler::PuzzleRegistrar().LoadOutput(ctxt);
      }
      if(got->PuzzleName() != puzzleName){
        logDest->LogFatal("Got output is for a different puzzle type."); 
        exit(1);
      }
      if(ref->Serial() != input->Serial()){
        logDest->LogFatal("Got output is for a different input."); 
        exit(1);
      }
      
      if(!puzzle->CompareOutputs(logDest.get(), input.get(), ref.get(), got.get())){
         logDest->LogFatal("Outputs are different.");
         exit(1);
      }
      logDest->LogInfo("Outputs are equal.");
   }catch(std::string &msg){
      std::cerr<<"Caught error string : "<<msg<<std::endl;
      return 1;
   }catch(std::exception &e){
      std::cerr<<"Caught exception : "<<e.what()<<std::endl;
      return 1;
   }catch(...){
      std::cerr<<"Caught unknown exception."<<std::endl;
      return 1;
   }

   return 0;
}

