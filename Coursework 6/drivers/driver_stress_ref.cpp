#include "unif01.h"
#include "bbattery.h"
#include "swrite.h"

#include "workload.hpp"

#include <iostream>
#include <random>
#include <chrono>

double score(double n, double g, double t)
{
   double s=1.0;
   
   double r=g / t;
   if(r>1.0){
      s *= exp(-r);
   }
   
   return log(n)/log(2.0) * s;
}

int main (int argc, char *argv[])
{
   auto begin = std::chrono::system_clock::now();
   
   std::mt19937 rng(time(0));
   
   double t=2+rng()%28;
   if(argc > 1){
      t=strtod(argv[1], 0);
   }
   
   // Turn off all printing to stdout from TestU01
   // You may want to try flipping this to 1 to see what it is actually doing.
   swrite_Basic=0;
    
   unif01_Gen *gen=workload_Create();
   std::string name=workload_Name(gen);
   
   // TODO : Choose the n that you think is achievable in timeBudget seconds
   double n = t*100000; // This is just a throwaway example of how to choose
   
   // Make sure this happens _before_ starting the battery, so that the
   // client knows what you are trying.
   fprintf(stdout, "%s, -1, TimeBudget, 0, %g\n", name.c_str(), t);
   fprintf(stdout, "%s, -1, ChosenN, 0, %g\n", name.c_str(), n);
   fflush(stdout);
   
   // The slow part
   auto results=bbattery_Rabbit(gen, n);
   
   for(auto & r : results){
     fprintf(stdout, "%s, %d, %s, %d, %.16g\n", name.c_str(), r.TestIndex, r.TestName.c_str(), r.SubIndex, r.pVal);
   }
   
   auto end = std::chrono::system_clock::now();
   std::chrono::duration<double> span=end-begin;
   double g=span.count();
   
   fprintf(stdout, "%s, -1, TimeUsed, 0, %.16g\n", name.c_str(), g);
   fprintf(stdout, "%s, -1, Score, 0, %.16g\n", name.c_str(), score(n, g, t));
    
   workload_Destroy(gen); 
   return 0;
}
