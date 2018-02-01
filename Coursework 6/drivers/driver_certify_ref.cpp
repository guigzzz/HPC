#include "unif01.h"
#include "bbattery.h"
#include "swrite.h"

#include "workload.hpp"

#include <iostream>

int main (int argc, char *argv[])
{
   // Turn off all printing to stdout from TestU01
   // You may want to try flipping this to 1 to see what it is actually doing.
   swrite_Basic=0;
    
   unif01_Gen *gen=workload_Create();
   std::string name=workload_Name(gen);
   
   auto results=bbattery_Crush(gen);
   
   for(auto & r : results){
     fprintf(stdout, "%s, %d, %s, %d, %.16g\n", name.c_str(), r.TestIndex, r.TestName.c_str(), r.SubIndex, r.pVal);
   }
    
   return 0;
}
