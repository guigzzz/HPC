
#include "puzzler/puzzler.hpp"

#include <iostream>
#include <chrono>
using namespace std::chrono;


int main(int argc, char *argv[])
{
   puzzler::PuzzleRegistrar::UserRegisterPuzzles();

   if(argc<3){
      fprintf(stderr, "run_puzzle reference new scale logLevel\n");
      fprintf(stderr, "INFO: Used to compare two custom puzzle solutions together\n");
      std::cout<<"Puzzles:\n";
      puzzler::PuzzleRegistrar::ListPuzzles();
      exit(1);
   }

   try{
      std::string refname=argv[1];
      std::string newname=argv[2];
      int scale=atoi(argv[3]);

      // Control how much is being output.
      // Higher numbers give you more info
      int logLevel=atoi(argv[4]);
      fprintf(stderr, "LogLevel = %s -> %d\n", argv[4], logLevel);

      std::shared_ptr<puzzler::ILog> logDest=std::make_shared<puzzler::LogDest>("run_puzzle", logLevel);
      logDest->Log(puzzler::Log_Info, "Created log.");

    auto refpuzzle=puzzler::PuzzleRegistrar::Lookup(refname);
    if(!refpuzzle)
	    throw std::runtime_error("No puzzle registered with name "+refname);

    auto newpuzzle=puzzler::PuzzleRegistrar::Lookup(newname);
    if(!newpuzzle)
	    throw std::runtime_error("No puzzle registered with name "+refname);

      logDest->LogInfo("Creating random input");
      auto input=refpuzzle->CreateInput(logDest.get(), scale);

      logDest->LogInfo("Executing new");
      auto got=newpuzzle->MakeEmptyOutput(input.get());
      high_resolution_clock::time_point t1 = high_resolution_clock::now();
      newpuzzle->Execute(logDest.get(), input.get(), got.get());
      high_resolution_clock::time_point t2 = high_resolution_clock::now();
    microseconds time_span = duration_cast<microseconds>(t2 - t1);
    logDest->LogInfo("Execute 1 duration: %d microseconds.", time_span.count());

      logDest->LogInfo("Executing reference");
      auto ref=refpuzzle->MakeEmptyOutput(input.get());
      t1 = high_resolution_clock::now();
      refpuzzle->Execute(logDest.get(), input.get(), ref.get());
      t2 = high_resolution_clock::now();

      time_span = duration_cast<microseconds>(t2 - t1);
    logDest->LogInfo("Execute 2 duration: %d microseconds.", time_span.count());

      logDest->LogInfo("Checking output");
      if(!newpuzzle->CompareOutputs(logDest.get(), input.get(), ref.get(), got.get())){
         logDest->LogFatal("Output is not correct.");
         exit(1);
      }
      logDest->LogInfo("Output is correct");


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

