#include "unif01.h"
#include "bbattery.h"
#include "swrite.h"

#include "workload.hpp"

#include <iostream>
#include <random>
#include <chrono>

#include "tbb/parallel_do.h"
#include "tbb/task_scheduler_init.h"

int main (int argc, char *argv[])
{  
   // Turn off all printing to stdout from TestU01
   // You may want to try flipping this to 1 to see what it is actually doing.
   swrite_Basic=0;

   unsigned trueP=tbb::task_scheduler_init::default_num_threads();

   auto work=[&](int i, tbb::parallel_do_feeder <int> &feeder){
    unif01_Gen *gen=workload_Create();
    
    
    if(i==0){
      workload_Next();
    }

    std::string name=workload_Name(gen);


    auto results=bbattery_SmallCrush(gen);
    
    for(auto & r : results){
      fprintf(stdout, "%s, %d, %s, %d, %.16g\n", name.c_str(), r.TestIndex, r.TestName.c_str(), r.SubIndex, r.pVal);
    }

    fflush(stdout); 
    feeder.add(0);
   };

   std::vector<int>workVec(trueP*4,0);
   workVec[0]=1;

   tbb::parallel_do(workVec.begin(),workVec.end(),work);
   
   // Loop forever - the user will kill the process
    
  
   return 0;
}
