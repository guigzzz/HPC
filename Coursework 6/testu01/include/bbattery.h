 
/* bbattery.h for ANSI C */
#ifndef BBATTERY_H
#define BBATTERY_H
 
#include "unif01.h"

#include <string>
#include <vector>

struct bbattery_Result
{
  bbattery_Result(int ti, std::string name, int sub, double p)
    : TestIndex(ti)
    , TestName(name)
    , SubIndex(sub)
    , pVal(p)
  {}
  
  int TestIndex;
  std::string TestName;
  int SubIndex; // Index of the p-value within the test
  double pVal;
};

std::vector<bbattery_Result> bbattery_SmallCrush (unif01_Gen *gen);


std::vector<bbattery_Result> bbattery_RepeatSmallCrush (unif01_Gen *gen, int rep[]);



std::vector<bbattery_Result> bbattery_Crush (unif01_Gen *gen);



std::vector<bbattery_Result> bbattery_RepeatCrush (unif01_Gen *gen, int rep[]);



std::vector<bbattery_Result> bbattery_BigCrush (unif01_Gen *gen);



std::vector<bbattery_Result> bbattery_RepeatBigCrush (unif01_Gen *gen, int rep[]);



std::vector<bbattery_Result> bbattery_Rabbit (unif01_Gen *gen, double nb);
std::vector<bbattery_Result> bbattery_RabbitPar (unif01_Gen * gen, double nb);

std::vector<bbattery_Result> bbattery_RabbitFile (char *filename, double nb);


std::vector<bbattery_Result> bbattery_RepeatRabbit (unif01_Gen *gen, double nb, int rep[]);



std::vector<bbattery_Result> bbattery_Alphabit (unif01_Gen *gen, double nb, int r, int s);



std::vector<bbattery_Result> bbattery_AlphabitFile (char *filename, double nb);


std::vector<bbattery_Result> bbattery_RepeatAlphabit (unif01_Gen *gen, double nb, int r, int s,
                              int rep[]);



std::vector<bbattery_Result> bbattery_BlockAlphabit (unif01_Gen *gen, double nb, int r, int s);
std::vector<bbattery_Result> bbattery_BlockAlphabitFile (char *filename, double nb);



std::vector<bbattery_Result> bbattery_RepeatBlockAlphabit (unif01_Gen *gen, double nb, int r, int s,
                                   int rep[], int w);



std::vector<bbattery_Result> bbattery_pseudoDIEHARD (unif01_Gen *gen);



std::vector<bbattery_Result> bbattery_FIPS_140_2 (unif01_Gen *gen);
std::vector<bbattery_Result> bbattery_FIPS_140_2File (char *filename);
std::vector<bbattery_Result> bbattery_CrushTBB (unif01_Gen * gen);
 
#endif
 

