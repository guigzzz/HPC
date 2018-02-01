#include "unif01.h"
#include "bbattery.h"
#include "swrite.h"

#include "workload.hpp"

#include <iostream>
#include <random>
#include <chrono>

int main (int argc, char *argv[])
{  
   // Turn off all printing to stdout from TestU01
   // You may want to try flipping this to 1 to see what it is actually doing.
   swrite_Basic=0;
   
   auto begin = std::chrono::system_clock::now();
   // Loop forever - the user will kill the process
   while(1){
      begin = std::chrono::system_clock::now();
      unif01_Gen *gen=workload_Create();
      std::string name=workload_Name(gen);
      
      auto results=bbattery_SmallCrush(gen);
      
      for(auto & r : results){
        fprintf(stdout, "%s, %d, %s, %d, %.16g\n", name.c_str(), r.TestIndex, r.TestName.c_str(), r.SubIndex, r.pVal);
      }
      fflush(stdout);
       auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> span=end-begin;
    double g=span.count();
    
    fprintf(stdout, "%s, -1, TimeUsed, 0, %.16g\n", name.c_str(), g);
      workload_Next();
   }
    
   return 0;
}
