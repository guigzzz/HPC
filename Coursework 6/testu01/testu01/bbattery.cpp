/*************************************************************************\
 *
 * Package:        TestU01
 * File:           bbattery.c
 * Environment:    ANSI C
 *
 * Copyright (c) 2002 Pierre L'Ecuyer, DIRO, Université de Montréal.
 * e-mail: lecuyer@iro.umontreal.ca
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted without a fee for private, research,
 * academic, or other non-commercial purposes.
 * Any use of this software in a commercial environment requires a
 * written licence from the copyright owner.
 *
 * Any changes made to this package must be clearly identified as such.
 *
 * In scientific publications which used this software, a reference to it
 * would be appreciated.
 *
 * Redistributions of source code must retain this copyright notice
 * and the following disclaimer.
 *
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
\*************************************************************************/


#include <iostream>
#include "util.h"
#include "config.h"
#include "bbattery.h"
#include "smultin.h"
#include "sknuth.h"
#include "smarsa.h"
#include "snpair.h"
#include "svaria.h"
#include "sstring.h"
#include "swalk.h"
#include "scomp.h"
#include "sspectral.h"
#include "swrite.h"
#include "sres.h"
#include "unif01.h"
#include "ufile.h"
#include <chrono>
#include "workload.hpp"

#include "gofs.h"
#include "gofw.h"
#include "fdist.h"
#include "fbar.h"
#include "num.h"
#include "chrono.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include <map>
#include <functional>

#include <vector>
#include "workload.hpp"
#include "tbb/parallel_for.h"
#include "tbb/task_group.h"
#include <iostream>
#include <chrono>


#define LEN 120
#define NAMELEN 30
#define NDIM 200                  /* Dimension of extern arrays */
#define THOUSAND 1000
#define MILLION (THOUSAND * THOUSAND)
#define BILLION (THOUSAND * MILLION)

/* The number of tests in each battery */
#define SMALLCRUSH_NUM 10
#define CRUSH_NUM 96
#define BIGCRUSH_NUM 106
#define RABBIT_NUM 26
#define ALPHABIT_NUM 9

using namespace std::chrono;

/*
DT10 : Removing globals from API

-- double bbattery_pVal[1 + NDIM] = { 0 };
-- char *bbattery_TestNames[1 + NDIM] = { 0 };
-- int bbattery_NTests;
*/

/* Gives the test number as enumerated in bbattery.tex. Some test applies
   more than one test, so the array of p-values does not correspond with 
   the test number in the doc. */
/*
--  static int TestNumber[1 + NDIM] = { 0 };
*/

/* DT10 : This captures what used to be mutable globals. */
struct TestGlobals
{
  double bbattery_pVal[1 + NDIM];
  char *bbattery_TestNames[1 + NDIM];
  int bbattery_NTests;
  int TestNumber[1 + NDIM];
};


struct TestRes
{
  std::vector<double> pVal;
  std::vector<std::string> name;
};

/*
  DT10 : Injects from globals stuct back into globals.

    auto &bbattery_pVal = globals.bbattery_pVal;
    auto &bbattery_TestNames = globals.bbattery_TestNames;
    auto &bbattery_NTests = globals.bbattery_NTests;
    auto &TestNumber = globals.TestNumber;
 */


/*-------------------------------- Functions ------------------------------*/


static void GetName (unif01_Gen * gen, char *genName)
{
   char *p;
   int len1, len2;

   if (NULL == gen) {
      genName[0] = '\0';
      return;
   }

   /* Print only the generator name, without the parameters or seeds. */
   /* The parameters start after the first blank; name ends with ':' */
   genName[LEN] = '\0';
   len1 = strcspn (gen->name, ":");
   len1 = util_Min (LEN, len1);
   strncpy (genName, gen->name, (size_t) len1);
   genName[len1] = '\0';
   /* For Filters or Combined generators */
   p = strstr (&gen->name[1 + len1], "unif01");
   while (p != NULL) {
      len1 += 2;
      if (len1 >= LEN)
         return;
      strcat (genName, ", ");
      len2 = strcspn (p, " \0");
      len2 = util_Min (LEN - len1, len2);
      if (len2 <= 0)
         return;
      strncat (genName, p, (size_t) len2);
      len1 = strlen (genName);
      genName[len1] = '\0';
      p += len2;
      p = strstr (p, "unif01");
   }
}


/*=========================================================================*/

static void WritepVal (double p)
/*
 * Write a p-value with a nice format.
 */
{
   if (p < gofw_Suspectp) {
      gofw_Writep0 (p);

   } else if (p > 1.0 - gofw_Suspectp) {
      if (p >= 1.0 - gofw_Epsilonp1) {
         printf (" 1 - eps1");
      } else if (p >= 1.0 - 1.0e-4) {
         printf (" 1 - ");
         num_WriteD (1.0 - p, 7, 2, 2);
         /* printf (" 1 - %.2g ", 1.0 - p); */
      } else if (p >= 1.0 - 1.0e-2)
         printf ("  %.4f ", p);
      else
         printf ("   %.2f", p);
   }
}


/*=========================================================================*/

static void WriteReport (
   const char *genName,                 /* Generator or file name */
   const char *batName,                 /* Battery name */
   int N,                         /* Max. number of tests */
   double pVal[],                 /* p-values of the tests */
   chrono_Chrono * Timer,         /* Timer */
   lebool Flag,                  /* = TRUE for a file, FALSE for a gen */
   lebool VersionFlag,           /* = TRUE: write the version number */
   double nb,                      /* Number of bits in the random file */
   TestGlobals &globals
   )
{
  // inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;

  
   int j, co;

   printf ("\n========= Summary results of ");
   printf ("%s", batName);
   printf (" =========\n\n");
   if (VersionFlag)
      printf (" Version:          %s\n", PACKAGE_STRING);
   if (Flag)
      printf (" File:             ");
   else
      printf (" Generator:        ");
   printf ("%s", genName);
   if (nb > 0)
      printf ("\n Number of bits:   %.0f", nb);
   co = 0;
   /* Some of the tests have not been done: their pVal[j] < 0. */
   for (j = 0; j < N; j++) {
      if (pVal[j] >= 0.0)
         co++;
   }
   printf ("\n Number of statistics:  %1d\n", co);
   printf (" Total CPU time:   ");
   chrono_Write (Timer, chrono_hms);

   co = 0;
   for (j = 0; j < N; j++) {
      if (pVal[j] < 0.0)          /* That test was not done: pVal = -1 */
         continue;
      if ((pVal[j] < gofw_Suspectp) || (pVal[j] > 1.0 - gofw_Suspectp)) {
         co++;
         break;
      }
   }
   if (co == 0) {
      printf ("\n\n All tests were passed\n\n\n\n");
      return;
   }

   if (gofw_Suspectp >= 0.01)
      printf ("\n The following tests gave p-values outside [%.4g, %.2f]",
         gofw_Suspectp, 1.0 - gofw_Suspectp);
   else if (gofw_Suspectp >= 0.0001)
      printf ("\n The following tests gave p-values outside [%.4g, %.4f]",
         gofw_Suspectp, 1.0 - gofw_Suspectp);
   else if (gofw_Suspectp >= 0.000001)
      printf ("\n The following tests gave p-values outside [%.4g, %.6f]",
         gofw_Suspectp, 1.0 - gofw_Suspectp);
   else
      printf ("\n The following tests gave p-values outside [%.4g, %.14f]",
         gofw_Suspectp, 1.0 - gofw_Suspectp);
   printf (":\n (eps  means a value < %6.1e)", gofw_Epsilonp);
   printf (":\n (eps1 means a value < %6.1e)", gofw_Epsilonp1);
   printf (":\n\n       Test                          p-value\n");
   printf (" ----------------------------------------------\n");

   co = 0;
   for (j = 0; j < N; j++) {
      if (pVal[j] < 0.0)          /* That test was not done: pVal = -1 */
         continue;
      if ((pVal[j] >= gofw_Suspectp) && (pVal[j] <= 1.0 - gofw_Suspectp))
         continue;                /* That test was passed */
      printf (" %2d ", TestNumber[j]);
      printf (" %-30s", bbattery_TestNames[j]);
      WritepVal (pVal[j]);
      printf ("\n");
      co++;
   }

   printf (" ----------------------------------------------\n");
   if (co < N - 1) {
      printf (" All other tests were passed\n");
   }
   printf ("\n\n\n");
}


/*=========================================================================*/

static void GetPVal_Walk (long N, swalk_Res * res, int *pj, const char *mess, int j2, TestGlobals &globals)
/*
 * Get the p-values in a swalk_RandomWalk1 test
 */
{
  // Inject globals
    auto &bbattery_pVal = globals.bbattery_pVal;
    auto &bbattery_TestNames = globals.bbattery_TestNames;
    auto &bbattery_NTests = globals.bbattery_NTests;
    auto &TestNumber = globals.TestNumber;

  
   int j = *pj;
   const unsigned int len = 20;

   char CharTemp[LEN + 1];

   if (N == 1) {
      bbattery_pVal[++j] = res->H[0]->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 H");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->M[0]->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 M");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->J[0]->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 J");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->R[0]->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 R");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->C[0]->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 C");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

   } else {
      bbattery_pVal[++j] = res->H[0]->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 H");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->M[0]->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 M");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->J[0]->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 J");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->R[0]->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 R");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      bbattery_pVal[++j] = res->C[0]->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (CharTemp, "RandomWalk1 C");
      strncat (CharTemp, mess, (size_t) len);
      strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);
   }

   *pj = j;
}



/*=========================================================================*/

static void GetPVal_CPairs (long N, snpair_Res * res, int *pj, const char *mess,
			    int j2, TestGlobals &globals)
/*
 * Get the p-values in a snpair_ClosePairs test
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;

  char CharTemp[LEN + 1];
  
   int j = *pj;
   const unsigned int len = 20;

   if (N == 1) {
      bbattery_pVal[++j] = res->pVal[snpair_NP];
      TestNumber[j] = j2;
      strcpy (CharTemp, "ClosePairs NP");
      strncat (CharTemp, mess, (size_t) len);
      strcpy (bbattery_TestNames[j], CharTemp);

      bbattery_pVal[++j] = res->pVal[snpair_mNP];
      TestNumber[j] = j2;
      strcpy (CharTemp, "ClosePairs mNP");
      strncat (CharTemp, mess, (size_t) len);
      strcpy (bbattery_TestNames[j], CharTemp);

   } else {
      bbattery_pVal[++j] = res->pVal[snpair_NP];
      TestNumber[j] = j2;
      strcpy (CharTemp, "ClosePairs NP");
      strncat (CharTemp, mess, (size_t) len);
      strcpy (bbattery_TestNames[j], CharTemp);

      bbattery_pVal[++j] = res->pVal[snpair_mNP];
      TestNumber[j] = j2;
      strcpy (CharTemp, "ClosePairs mNP");
      strncat (CharTemp, mess, (size_t) len);
      strcpy (bbattery_TestNames[j], CharTemp);

      bbattery_pVal[++j] = res->pVal[snpair_mNP1];
      TestNumber[j] = j2;
      strcpy (CharTemp, "ClosePairs mNP1");
      strncat (CharTemp, mess, (size_t) len);
      strcpy (bbattery_TestNames[j], CharTemp);

      bbattery_pVal[++j] = res->pVal[snpair_mNP2];
      TestNumber[j] = j2;
      strcpy (CharTemp, "ClosePairs mNP2");
      strncat (CharTemp, mess, (size_t) len);
      strcpy (bbattery_TestNames[j], CharTemp);

      bbattery_pVal[++j] = res->pVal[snpair_NJumps];
      TestNumber[j] = j2;
      strcpy (CharTemp, "ClosePairs NJumps");
      strncat (CharTemp, mess, (size_t) len);
      strcpy (bbattery_TestNames[j], CharTemp);

      if (snpair_mNP2S_Flag) {
         bbattery_pVal[++j] = res->pVal[snpair_mNP2S];
         TestNumber[j] = j2;
         strcpy (CharTemp, "ClosePairs mNP2S");
         strncat (CharTemp, mess, (size_t) len);
         strcpy (bbattery_TestNames[j], CharTemp);
      }
   }

   *pj = j;
}


static std::vector<bbattery_Result> makeResults(TestGlobals &globals)
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
  std::map<int,int> seen;
  
  std::vector<bbattery_Result> res;
  for(int i=0; i<NDIM; i++){
    if(bbattery_pVal[i]>=0){
      int subindex=seen[TestNumber[i]]++;
      res.emplace_back(TestNumber[i], bbattery_TestNames[i], subindex, bbattery_pVal[i]);
    }
    util_Free(bbattery_TestNames[i]);
    bbattery_TestNames[i]=0;
  }
  return res;
}


/*=========================================================================*/

static void InitBat (TestGlobals &globals)
/*
 * Initializes the battery of tests: sets all p-values to -1.
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
   int j;
   for (j = 0; j < NDIM; j++)
      bbattery_pVal[j] = -1.0;

   // These will now be deallocaed in makeResults
   //static int flag = 0;
   //if (0 == flag) {
   //   flag++;
      for (j = 0; j < NDIM; j++)
	bbattery_TestNames[j] = (char *)util_Calloc (LEN + 1, sizeof (char));
      //}
}


/*=========================================================================*/

static void SmallCrush (unif01_Gen * gen, char *filename, int Rep[], TestGlobals &globals)
/*
 * A small battery of statistical tests for Random Number Generators 
 * used in simulation.
 * Rep[i] gives the number of times that test i will be done. The default
 * values are Rep[i] = 1 for all i.
 */
{

  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;

  
   const int r = 0;
   int i;
   int j = -1;
   int j2 = 0;
   char genName[LEN + 1] = "";
   chrono_Chrono *Timer;
   sres_Poisson *res1;
   sres_Chi2 *res2;
   sknuth_Res2 *res3;
   swalk_Res *res4;
   sknuth_Res1 *res5;
   sstring_Res *res6;
   lebool fileFlag;

   Timer = chrono_Create ();
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "                 Starting SmallCrush\n"
         "                 Version: %s\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n",
         PACKAGE_STRING);
   }

   if (NULL == gen) {
      gen = ufile_CreateReadText (filename, 10 * MILLION);
      fileFlag = TRUE;
   } else
      fileFlag = FALSE;

   ++j2;
   if (fileFlag)
      ufile_InitReadText ();
   res1 = sres_CreatePoisson ();
   for (i = 0; i < Rep[j2]; ++i) {
#ifdef USE_LONGLONG
      smarsa_BirthdaySpacings (gen, res1, 1, 5 * MILLION, r, 1073741824,
         2, 1);
#else
      smarsa_BirthdaySpacings (gen, res1, 10, MILLION / 2, r, 67108864, 2, 1);
#endif
      bbattery_pVal[++j] = res1->pVal2;
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "BirthdaySpacings");
   }
   sres_DeletePoisson (res1);

   if (fileFlag)
      ufile_InitReadText ();
   ++j2;
   res3 = sknuth_CreateRes2 ();
   for (i = 0; i < Rep[j2]; ++i) {
      sknuth_Collision (gen, res3, 1, 5 * MILLION, 0, 65536, 2);
      bbattery_pVal[++j] = res3->Pois->pVal2;
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "Collision");
   }
   sknuth_DeleteRes2 (res3);

   if (fileFlag)
      ufile_InitReadText ();
   ++j2;
   res2 = sres_CreateChi2 ();
   for (i = 0; i < Rep[j2]; ++i) {
      sknuth_Gap (gen, res2, 1, MILLION / 5, 22, 0.0, .00390625);
      bbattery_pVal[++j] = res2->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "Gap");
   }

   ++j2;
   if (fileFlag)
      ufile_InitReadText ();
   for (i = 0; i < Rep[j2]; ++i) {
      sknuth_SimpPoker (gen, res2, 1, 2 * MILLION / 5, 24, 64, 64);
      bbattery_pVal[++j] = res2->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "SimpPoker");
   }

   ++j2;
   if (fileFlag)
      ufile_InitReadText ();
   for (i = 0; i < Rep[j2]; ++i) {
      sknuth_CouponCollector (gen, res2, 1, MILLION / 2, 26, 16);
      bbattery_pVal[++j] = res2->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "CouponCollector");
   }

   if (fileFlag)
      ufile_InitReadText ();
   ++j2;
   res5 = sknuth_CreateRes1 ();
   for (i = 0; i < Rep[j2]; ++i) {
      sknuth_MaxOft (gen, res5, 1, 2 * MILLION, 0, MILLION / 10, 6);
      bbattery_pVal[++j] = res5->Chi->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "MaxOft");
      bbattery_pVal[++j] = res5->Bas->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "MaxOft AD");
   }
   sknuth_DeleteRes1 (res5);

   ++j2;
   if (fileFlag)
      ufile_InitReadText ();
   for (i = 0; i < Rep[j2]; ++i) {
      svaria_WeightDistrib (gen, res2, 1, MILLION / 5, 27, 256, 0.0, 0.125);
      bbattery_pVal[++j] = res2->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "WeightDistrib");
   }

   ++j2;
   if (fileFlag)
      ufile_InitReadText ();
   for (i = 0; i < Rep[j2]; ++i) {
      smarsa_MatrixRank (gen, res2, 1, 20 * THOUSAND, 20, 10, 60, 60);
      bbattery_pVal[++j] = res2->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "MatrixRank");
   }
   sres_DeleteChi2 (res2);

   if (fileFlag)
      ufile_InitReadText ();
   ++j2;
   res6 = sstring_CreateRes ();
   for (i = 0; i < Rep[j2]; ++i) {
      sstring_HammingIndep (gen, res6, 1, MILLION/2, 20, 10, 300, 0);
      bbattery_pVal[++j] = res6->Bas->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "HammingIndep");
   }
   sstring_DeleteRes (res6);

   if (fileFlag)
      ufile_InitReadText ();
   ++j2;
   util_Assert (j2 <= SMALLCRUSH_NUM, "SmallCrush:   j2 > SMALLCRUSH_NUM");
   res4 = swalk_CreateRes ();
   for (i = 0; i < Rep[j2]; ++i) {
      swalk_RandomWalk1 (gen, res4, 1, MILLION, r, 30, 150, 150);
      GetPVal_Walk (1, res4, &j, "", j2, globals);
   }
   swalk_DeleteRes (res4);

   bbattery_NTests = ++j;
   if (fileFlag) {
      if(swrite_Basic){
         WriteReport (filename, "SmallCrush", bbattery_NTests, bbattery_pVal,
            Timer, TRUE, TRUE, 0.0, globals);
      }
      ufile_DeleteReadBin (gen);
   } else {
      GetName (gen, genName);
      if(swrite_Basic){
         WriteReport (genName, "SmallCrush", bbattery_NTests, bbattery_pVal,
            Timer, FALSE, TRUE, 0.0, globals);
      }
   }
   chrono_Delete (Timer);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_SmallCrush (unif01_Gen * gen)
{
  TestGlobals globals;
   int i;
   int Rep[1 + NDIM] = {0};
   for (i = 1; i <= SMALLCRUSH_NUM; ++i)
      Rep[i] = 1;
   SmallCrush (gen, NULL, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_SmallCrushFile (char *filename)
{
  TestGlobals globals;
   int i;
   int Rep[1 + NDIM] = {0};
   for (i = 1; i <= SMALLCRUSH_NUM; ++i)
      Rep[i] = 1;
   SmallCrush (NULL, filename, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_RepeatSmallCrush (unif01_Gen * gen, int Rep[])
{
  TestGlobals globals;
  SmallCrush (gen, NULL, Rep, globals);
  return makeResults(globals);
}


/*=========================================================================*/

static void Crush (unif01_Gen * gen, int Rep[], TestGlobals &globals)
/*
 * A battery of stringent statistical tests for Random Number Generators
 * used in simulation.
 * Rep[i] gives the number of times that test i will be done. The default
 * values are Rep[i] = 1 for all i.
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
   const int s = 30;
   const int r = 0;
   int i;
   chrono_Chrono *Timer;
   char genName[LEN + 1] = "";
   int j = -1;
   int j2 = 0;

   Timer = chrono_Create ();
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "                 Starting Crush\n"
         "                 Version: %s\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n",
         PACKAGE_STRING);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_SerialOver (workload_Clone(gen), res, 1, 500 * MILLION, 0, 4096, 2);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SerialOver, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_SerialOver (workload_Clone(gen), res, 1, 300 * MILLION, 0, 64, 4);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SerialOver, t = 4");
      }
      sres_DeleteBasic (res);
   }
   {
      smarsa_Res *res;
      res = smarsa_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 1024 * 1024, 2);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 10, 1024 * 1024, 2);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 1024, 4);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 20, 1024, 4);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 32, 8);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 25, 32, 8);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 4, 20);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 20");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 28, 4, 20);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 20");
      }
      smarsa_DeleteRes (res);
   }
   {
      sres_Poisson *res;
      res = sres_CreatePoisson ();

#ifdef USE_LONGLONG
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         long d;
#if LONG_MAX <= 2147483647L
         d = 1073741824L;
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 10, 10 * MILLION, 0, d, 2, 1);
#else
         d = 2*1073741824L;
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 5, 20 * MILLION, 0, d, 2, 1);
#endif
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 5, 20 * MILLION, 0, 2097152, 3,
            1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 3");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 5, 20 * MILLION, 0, 65536, 4, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 0, 512, 7, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 7");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 7, 512, 7, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 7");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 14, 256, 8, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 22, 256, 8, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 8");
      }

#else
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 200, 4 * MILLION / 10, 0,
            67108864, 2, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 100, 4 * MILLION / 10, 0, 131072,
            3, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 3");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 200, 4 * MILLION / 10, 0,
            1024 * 8, 4, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 100, 4 * MILLION / 10, 0, 16, 13,
            1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 100, 4 * MILLION / 10, 10, 16,
            13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 100, 4 * MILLION / 10, 20, 16,
            13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (workload_Clone(gen), res, 100, 4 * MILLION / 10, 26, 16,
            13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }
#endif

      sres_DeletePoisson (res);
   }
   {
      lebool flag = snpair_mNP2S_Flag;
      snpair_Res *res;
      res = snpair_CreateRes ();

      snpair_mNP2S_Flag = FALSE;
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairs (workload_Clone(gen), res, 10, 2 * MILLION, 0, 2, 0, 30);
         GetPVal_CPairs (10, res, &j, ", t = 2", j2, globals);
      }

      snpair_mNP2S_Flag = TRUE;
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairs (workload_Clone(gen), res, 10, 2 * MILLION, 0, 3, 0, 30);
         GetPVal_CPairs (10, res, &j, ", t = 3", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairs (workload_Clone(gen), res, 5, 2 * MILLION, 0, 7, 0, 30);
         GetPVal_CPairs (10, res, &j, ", t = 7", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairsBitMatch (workload_Clone(gen), res, 4, 4 * MILLION, 0, 2);
         bbattery_pVal[++j] = res->pVal[snpair_BM];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairsBitMatch (workload_Clone(gen), res, 2, 4 * MILLION, 0, 4);
         bbattery_pVal[++j] = res->pVal[snpair_BM];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 4");
      }
      snpair_DeleteRes (res);
      snpair_mNP2S_Flag = flag;
   }
   {
      sres_Chi2 *res;
      res = sres_CreateChi2 ();

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (workload_Clone(gen), res, 1, 40 * MILLION, 0, 16, 16);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, d = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (workload_Clone(gen), res, 1, 40 * MILLION, 26, 16, 16);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, d = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (workload_Clone(gen), res, 1, 10 * MILLION, 0, 64, 64);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, d = 64");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (workload_Clone(gen), res, 1, 10 * MILLION, 24, 64, 64);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, d = 64");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (workload_Clone(gen), res, 1, 40 * MILLION, 0, 4);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, d = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (workload_Clone(gen), res, 1, 40 * MILLION, 28, 4);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, d = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (workload_Clone(gen), res, 1, 10 * MILLION, 0, 16);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, d = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (workload_Clone(gen), res, 1, 10 * MILLION, 26, 16);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, d = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (workload_Clone(gen), res, 1, 100 * MILLION, 0, 0.0, 0.125);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (workload_Clone(gen), res, 1, 100 * MILLION, 27, 0.0, 0.125);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 27");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (workload_Clone(gen), res, 1, 5 * MILLION, 0, 0.0, 1.0/256.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (workload_Clone(gen), res, 1, 5 * MILLION, 22, 0.0, 1.0/256.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 22");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Run (workload_Clone(gen), res, 1, 500 * MILLION, 0, TRUE);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of U01, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Run (workload_Clone(gen), res, 1, 500 * MILLION, 15, FALSE);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of U01, r = 15");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Permutation (workload_Clone(gen), res, 1, 50 * MILLION, 0, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Permutation, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Permutation (workload_Clone(gen), res, 1, 50 * MILLION, 15, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Permutation, r = 15");
      }
      sres_DeleteChi2 (res);
   }
   {
      sknuth_Res2 *res;
      res = sknuth_CreateRes2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CollisionPermut (workload_Clone(gen), res, 5, 10 * MILLION, 0, 13);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionPermut, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CollisionPermut (workload_Clone(gen), res, 5, 10 * MILLION, 15, 13);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionPermut, r = 15");
      }
      sknuth_DeleteRes2 (res);
   }
   {
      sknuth_Res1 *res;
      res = sknuth_CreateRes1 ();

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (workload_Clone(gen), res, 10, 10 * MILLION, 0, MILLION / 10, 5);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 5");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 5");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (workload_Clone(gen), res, 5, 10 * MILLION, 0, MILLION / 10, 10);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 10");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 10");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (workload_Clone(gen), res, 1, 10 * MILLION, 0, MILLION / 10, 20);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 20");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 20");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (workload_Clone(gen), res, 1, 10 * MILLION, 0, MILLION / 10, 30);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 30");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 30");
      }
      sknuth_DeleteRes1 (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleProd (workload_Clone(gen), res, 1, 10 * MILLION, 0, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleProd, t = 10");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleProd (workload_Clone(gen), res, 1, 10 * MILLION, 0, 30);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleProd, t = 30");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleMean (workload_Clone(gen), res, 10*MILLION, 20, 0);
         bbattery_pVal[++j] = res->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleMean");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleCorr (workload_Clone(gen), res, 1, 500 * MILLION, 0, 1);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleCorr");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_AppearanceSpacings (workload_Clone(gen), res, 1, 10 * MILLION, 400 * MILLION,
            r, 30, 15);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AppearanceSpacings, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_AppearanceSpacings (workload_Clone(gen), res, 1, 10 * MILLION, 100 * MILLION,
            20, 10, 15);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AppearanceSpacings, r = 20");
      }
      sres_DeleteBasic (res);
   }
   {
      smarsa_Res2 *res2;
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 0, 256, 0.0, 0.125);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 8, 256, 0.0, 0.125);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 16, 256, 0.0, 0.125);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 24, 256, 0.0, 0.125);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 24");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SumCollector (workload_Clone(gen), res, 1, 20 * MILLION, 0, 10.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SumCollector");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (workload_Clone(gen), res, 1, MILLION, r, s, 2 * s, 2 * s);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, 60 x 60");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (workload_Clone(gen), res, 1, MILLION, 20, 10, 2 * s, 2 * s);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, 60 x 60");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (workload_Clone(gen), res, 1, 50 * THOUSAND, r, s, 10 * s, 10 * s);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, 300 x 300");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (workload_Clone(gen), res, 1, 50 * THOUSAND, 20, 10, 10 * s,
            10 * s);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, 300 x 300");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (workload_Clone(gen), res, 1, 2 * THOUSAND, r, s, 40 * s, 40 * s);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, 1200 x 1200");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (workload_Clone(gen), res, 1, 2 * THOUSAND, 20, 10, 40 * s, 40 * s);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, 1200 x 1200");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_Savir2 (workload_Clone(gen), res, 1, 20 * MILLION, 0, 1024*1024, 30);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Savir2");
      }
      sres_DeleteChi2 (res);

      res2 = smarsa_CreateRes2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_GCD (workload_Clone(gen), res2, 1, 100 * MILLION, 0, 30);
         bbattery_pVal[++j] = res2->GCD->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "GCD, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_GCD (workload_Clone(gen), res2, 1, 40 * MILLION, 10, 20);
         bbattery_pVal[++j] = res2->GCD->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "GCD, r = 10");
      }
      smarsa_DeleteRes2 (res2);
   }
   {
      swalk_Res *res;
      res = swalk_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (workload_Clone(gen), res, 1, 50 * MILLION, r, s, 90, 90);
         GetPVal_Walk (1, res, &j, " (L = 90)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (workload_Clone(gen), res, 1, 10 * MILLION, 20, 10, 90, 90);
         GetPVal_Walk (1, res, &j, " (L = 90)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (workload_Clone(gen), res, 1, 5 * MILLION, r, s, 1000, 1000);
         GetPVal_Walk (1, res, &j, " (L = 1000)", j2, globals);
      }
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (workload_Clone(gen), res, 1, MILLION, 20, 10, 1000, 1000);
         GetPVal_Walk (1, res, &j, " (L = 1000)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (workload_Clone(gen), res, 1, MILLION / 2, r, s, 10000, 10000);
         GetPVal_Walk (1, res, &j, " (L = 10000)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (workload_Clone(gen), res, 1, MILLION / 10, 20, 10, 10000, 10000);
         GetPVal_Walk (1, res, &j, " (L = 10000)", j2, globals);
      }
      swalk_DeleteRes (res);
   }
   {
      scomp_Res *res;
      res = scomp_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LinearComp (workload_Clone(gen), res, 1, 120 * THOUSAND, r, 1);
         bbattery_pVal[++j] = res->JumpNum->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 0");
         bbattery_pVal[++j] = res->JumpSize->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LinearComp (workload_Clone(gen), res, 1, 120 * THOUSAND, 29, 1);
         bbattery_pVal[++j] = res->JumpNum->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 29");
         bbattery_pVal[++j] = res->JumpSize->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 29");
      }
      scomp_DeleteRes (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LempelZiv (workload_Clone(gen), res, 10, 25, r, s);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LempelZiv");
      }
      sres_DeleteBasic (res);
   }
   {
      sspectral_Res *res;
      res = sspectral_CreateRes ();

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier3 (workload_Clone(gen), res, 50 * THOUSAND, 14, r, s);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier3, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier3 (workload_Clone(gen), res, 50 * THOUSAND, 14, 20, 10);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier3, r = 20");
      }
      sspectral_DeleteRes (res);
   }
   {
      sstring_Res2 *res;
      res = sstring_CreateRes2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_LongestHeadRun (workload_Clone(gen), res, 1, 1000, r, s, 20 + 10 * MILLION);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 0");
         bbattery_pVal[++j] = res->Disc->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_LongestHeadRun (workload_Clone(gen), res, 1, 300, 20, 10, 20 + 10 * MILLION);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 20");
         bbattery_pVal[++j] = res->Disc->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 20");
      }
      sstring_DeleteRes2 (res);
   }
   {
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_PeriodsInStrings (workload_Clone(gen), res, 1, 300 * MILLION, r, s);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "PeriodsInStrings, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_PeriodsInStrings (workload_Clone(gen), res, 1, 300 * MILLION, 15, 15);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "PeriodsInStrings, r = 15");
      }
      sres_DeleteChi2 (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingWeight2 (workload_Clone(gen), res, 100, 100 * MILLION, r, s, MILLION);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingWeight2, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingWeight2 (workload_Clone(gen), res, 30, 100 * MILLION, 20, 10, MILLION);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingWeight2, r = 20");
      }
      sres_DeleteBasic (res);
   }
   {
      sstring_Res *res;
      res = sstring_CreateRes ();
      /* sstring_HammingCorr will probably be removed: less sensitive than
         svaria_HammingIndep */
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingCorr (workload_Clone(gen), res, 1, 500 * MILLION, r, s, s);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingCorr, L = 30");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingCorr (workload_Clone(gen), res, 1, 50 * MILLION, r, s, 10 * s);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingCorr, L = 300");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingCorr (workload_Clone(gen), res, 1, 10 * MILLION, r, s, 40 * s);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingCorr, L = 1200");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (workload_Clone(gen), res, 1, 300 * MILLION, r, s, s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L = 30");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (workload_Clone(gen), res, 1, 100 * MILLION, 20, 10, s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L = 30");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (workload_Clone(gen), res, 1, 30 * MILLION, r, s, 10 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L = 300");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (workload_Clone(gen), res, 1, 10 * MILLION, 20, 10, 10 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L = 300");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (workload_Clone(gen), res, 1, 10 * MILLION, r, s, 40 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L = 1200");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (workload_Clone(gen), res, 1, MILLION, 20, 10, 40 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L = 1200");
      }
      sstring_DeleteRes (res);
   }
   {
      sstring_Res3 *res;
      res = sstring_CreateRes3 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_Run (workload_Clone(gen), res, 1, 1 * BILLION, r, s);
         bbattery_pVal[++j] = res->NRuns->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 0");
         bbattery_pVal[++j] = res->NBits->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_Run (workload_Clone(gen), res, 1, 1 * BILLION, 20, 10);
         bbattery_pVal[++j] = res->NRuns->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 20");
         bbattery_pVal[++j] = res->NBits->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 20");
      }
      sstring_DeleteRes3 (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (workload_Clone(gen), res, 10, 30 + BILLION, r, s, 1);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d = 1");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (workload_Clone(gen), res, 5, 1 + BILLION, 20, 10, 1);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d = 1");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (workload_Clone(gen), res, 10, 31 + BILLION, r, s, s);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d = 30");
      }

      ++j2;
 /*     util_Assert (j2 <= CRUSH_NUM, "Crush:   j2 > CRUSH_NUM");  */
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (workload_Clone(gen), res, 5, 11 + BILLION, 20, 10, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d = 10");
      }
      sres_DeleteBasic (res);
   }

   bbattery_NTests = ++j;
   GetName (gen, genName);
   if(swrite_Basic){
      WriteReport (genName, "Crush", bbattery_NTests,
         bbattery_pVal, Timer, FALSE, TRUE, 0.0, globals);
   }
   chrono_Delete (Timer);
}

static void CrushTBB (unif01_Gen * gen, int Rep[], TestGlobals &globals)
/*
 * A battery of stringent statistical tests for Random Number Generators
 * used in simulation.
 * Rep[i] gives the number of times that test i will be done. The default
 * values are Rep[i] = 1 for all i.
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;

  // Within some function, create a group
  tbb::task_group group;
  
  
   const int s = 30;
   const int r = 0;
   int i;
   chrono_Chrono *Timer;
   char genName[LEN + 1] = "";

   int j2 = 0;

   Timer = chrono_Create ();
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "                 Starting Crush\n"
         "                 Version: %s\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n",
         PACKAGE_STRING);
   }
      // sres_Basic *res;
      // res = sres_CreateBasic ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_SerialOver (gen, res, 1, 500 * MILLION, 0, 4096, 2);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SerialOver, t = 2");
      // }

        auto SerialOver1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl; 
        sres_Basic *res = sres_CreateBasic ();
          smarsa_SerialOver (workload_Clone(gen), res, 1, 500 * MILLION, 0, 4096, 2);
          tr.pVal.push_back(res->pVal2[gofw_Mean]); // EXAMPLE OF HOW TO SETUP PUSH_BACK
          tr.name.push_back("SerialOver, t = 2");
          sres_DeleteBasic (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_SerialOver (gen, res, 1, 300 * MILLION, 0, 64, 4);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SerialOver, t = 4");
      // }
      // sres_DeleteBasic (res);

      auto SerialOver2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
          smarsa_SerialOver (workload_Clone(gen), res, 1, 300 * MILLION, 0, 64, 4);
          tr.pVal.push_back(res->pVal2[gofw_Mean]);
          tr.name.push_back("SerialOver, t = 4");
          sres_DeleteBasic (res);

          return tr;
      };

      // smarsa_Res *res;
      // res = smarsa_CreateRes ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 0, 1024 * 1024, 2);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 2");
      // }

      auto CollisionOver1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 1024 * 1024, 2);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 2");
          smarsa_DeleteRes (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 10, 1024 * 1024, 2);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 2");
      // }
      
      auto CollisionOver2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 10, 1024 * 1024, 2);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 2");
          smarsa_DeleteRes (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 0, 1024, 4);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 4");
      // }

      auto CollisionOver3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 1024, 4);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 4");
          smarsa_DeleteRes (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 20, 1024, 4);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 4");
      // }

      auto CollisionOver4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 20, 1024, 4);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 4");
          smarsa_DeleteRes (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 0, 32, 8);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 8");
      // }

      auto CollisionOver5 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 32, 8);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 8");
          smarsa_DeleteRes (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 25, 32, 8);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 8");
      // }

      auto CollisionOver6 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 25, 32, 8);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 8");
          smarsa_DeleteRes (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 0, 4, 20);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 20");
      // }

      auto CollisionOver7 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 0, 4, 20);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 20");
          smarsa_DeleteRes (res);

          return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_CollisionOver (gen, res, 10, 10 * MILLION, 28, 4, 20);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionOver, t = 20");
      // }
      // smarsa_DeleteRes (res);

      auto CollisionOver8 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res *res = smarsa_CreateRes ();
          smarsa_CollisionOver (workload_Clone(gen), res, 10, 10 * MILLION, 28, 4, 20);
          tr.pVal.push_back(res->Pois->pVal2);
          tr.name.push_back("CollisionOver, t = 20");
          smarsa_DeleteRes (res);

          return tr;
      };

      // sres_Poisson *res;
      // res = sres_CreatePoisson ();

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    long d;
      //    d = 1073741824L;
      //    smarsa_BirthdaySpacings (gen, res, 10, 10 * MILLION, 0, d, 2, 1);
      //    bbattery_pVal[++j] = res->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 2");
      // }

      auto BirthdaySpacings1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Poisson *res = sres_CreatePoisson ();
        // smarsa_BirthdaySpacings (workload_Clone(gen), res, 10, 10 * MILLION, 0, 1073741824L, 2, 1);
        smarsa_BirthdaySpacings (workload_Clone(gen), res, 5, 20 * MILLION, 0, 2*1073741824L, 2, 1);
        tr.pVal.push_back(res->pVal2);
        tr.name.push_back("BirthdaySpacings, t = 2");
        sres_DeletePoisson (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_BirthdaySpacings (gen, res, 5, 20 * MILLION, 0, 2097152, 3,
      //       1);
      //    bbattery_pVal[++j] = res->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 3");
      // }

      auto BirthdaySpacings2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Poisson *res = sres_CreatePoisson ();
        smarsa_BirthdaySpacings (workload_Clone(gen), res, 5, 20 * MILLION, 0, 2097152, 3,
            1);
        tr.pVal.push_back(res->pVal2);
        tr.name.push_back("BirthdaySpacings, t = 3");
        sres_DeletePoisson (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_BirthdaySpacings (gen, res, 5, 20 * MILLION, 0, 65536, 4, 1);
      //    bbattery_pVal[++j] = res->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 4");
      // }

      auto BirthdaySpacings3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Poisson *res = sres_CreatePoisson ();
        smarsa_BirthdaySpacings (workload_Clone(gen), res, 5, 20 * MILLION, 0, 65536, 4, 1);
        tr.pVal.push_back(res->pVal2);
        tr.name.push_back("BirthdaySpacings, t = 4");
        sres_DeletePoisson (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_BirthdaySpacings (gen, res, 3, 20 * MILLION, 0, 512, 7, 1);
      //    bbattery_pVal[++j] = res->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 7");
      // }

      auto BirthdaySpacings4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Poisson *res = sres_CreatePoisson ();
        smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 0, 512, 7, 1);
        tr.pVal.push_back(res->pVal2);
        tr.name.push_back("BirthdaySpacings, t = 7");
        sres_DeletePoisson (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_BirthdaySpacings (gen, res, 3, 20 * MILLION, 7, 512, 7, 1);
      //    bbattery_pVal[++j] = res->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 7");
      // }

      auto BirthdaySpacings5 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Poisson *res = sres_CreatePoisson ();
        smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 7, 512, 7, 1);
        tr.pVal.push_back(res->pVal2);
        tr.name.push_back("BirthdaySpacings, t = 7");
        sres_DeletePoisson (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_BirthdaySpacings (gen, res, 3, 20 * MILLION, 14, 256, 8, 1);
      //    bbattery_pVal[++j] = res->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 8");
      // }

      auto BirthdaySpacings6 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Poisson *res = sres_CreatePoisson ();
        smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 14, 256, 8, 1);
        tr.pVal.push_back(res->pVal2);
        tr.name.push_back("BirthdaySpacings, t = 8");
        sres_DeletePoisson (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_BirthdaySpacings (gen, res, 3, 20 * MILLION, 22, 256, 8, 1);
      //    bbattery_pVal[++j] = res->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 8");
      // }

      auto BirthdaySpacings7 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Poisson *res = sres_CreatePoisson ();
        smarsa_BirthdaySpacings (workload_Clone(gen), res, 3, 20 * MILLION, 22, 256, 8, 1);
        tr.pVal.push_back(res->pVal2);
        tr.name.push_back("BirthdaySpacings, t = 8");
        sres_DeletePoisson (res);
        return tr;
      };


  //      STILL NEED TO TRNAFER THESE TO LAMBDAS

      // lebool flag = snpair_mNP2S_Flag;
      // snpair_Res *res;
      // res = snpair_CreateRes ();

      //snpair_mNP2S_Flag = FALSE;
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    snpair_ClosePairs (gen, res, 10, 2 * MILLION, 0, 2, 0, 30);
      //    GetPVal_CPairs (10, res, &j, ", t = 2", j2, globals);
      //    const char *mess = ", t = 2";
      //     char CharTemp[LEN + 1];
      //     const unsigned int len = 20;

      //     bbattery_pVal[++j] = res->pVal[snpair_NP];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "ClosePairs NP");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strcpy (bbattery_TestNames[j], CharTemp);

      //     bbattery_pVal[++j] = res->pVal[snpair_mNP];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "ClosePairs mNP");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strcpy (bbattery_TestNames[j], CharTemp);

      //     bbattery_pVal[++j] = res->pVal[snpair_mNP1];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "ClosePairs mNP1");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strcpy (bbattery_TestNames[j], CharTemp);

      //     bbattery_pVal[++j] = res->pVal[snpair_mNP2];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "ClosePairs mNP2");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strcpy (bbattery_TestNames[j], CharTemp);

      //     bbattery_pVal[++j] = res->pVal[snpair_NJumps];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "ClosePairs NJumps");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strcpy (bbattery_TestNames[j], CharTemp);
      // }

      auto ClosePairs1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        snpair_Res *res = snpair_CreateRes ();
        snpair_ClosePairs (workload_Clone(gen), res, 10, 2 * MILLION, 0, 2, 0, 30);
        const char *mess = ", t = 2";
        char CharTemp[LEN + 1];
        const unsigned int len = 20;

        tr.pVal.push_back(res->pVal[snpair_NP]);
        strcpy (CharTemp, "ClosePairs NP");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP]);
        strcpy (CharTemp, "ClosePairs mNP");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP1]);
        strcpy (CharTemp, "ClosePairs mNP1");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP2]);
        strcpy (CharTemp, "ClosePairs mNP2");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_NJumps]);
        strcpy (CharTemp, "ClosePairs NJumps");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);
        snpair_DeleteRes (res);
        return tr;
      };




      //snpair_mNP2S_Flag = TRUE;
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    snpair_ClosePairs (gen, res, 10, 2 * MILLION, 0, 3, 0, 30);
      //    GetPVal_CPairs (10, res, &j, ", t = 3", j2, globals);
      // }

      auto ClosePairs2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        snpair_Res *res = snpair_CreateRes ();
        snpair_ClosePairs (workload_Clone(gen), res, 10, 2 * MILLION, 0, 3, 0, 30);
        const char *mess = ", t = 3";
        char CharTemp[LEN + 1];
        const unsigned int len = 20;

        tr.pVal.push_back(res->pVal[snpair_NP]);
        strcpy (CharTemp, "ClosePairs NP");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP]);
        strcpy (CharTemp, "ClosePairs mNP");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP1]);
        strcpy (CharTemp, "ClosePairs mNP1");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP2]);
        strcpy (CharTemp, "ClosePairs mNP2");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_NJumps]);
        strcpy (CharTemp, "ClosePairs NJumps");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP2S]);
        strcpy (CharTemp, "ClosePairs mNP2S");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);
        snpair_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    snpair_ClosePairs (gen, res, 5, 2 * MILLION, 0, 7, 0, 30);
      //    GetPVal_CPairs (10, res, &j, ", t = 7", j2, globals);
      // }

      auto ClosePairs3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        snpair_Res *res = snpair_CreateRes ();
        snpair_ClosePairs (workload_Clone(gen), res, 5, 2 * MILLION, 0, 7, 0, 30);
        const char *mess = ", t = 7";
        char CharTemp[LEN + 1];
        const unsigned int len = 20;

        tr.pVal.push_back(res->pVal[snpair_NP]);
        strcpy (CharTemp, "ClosePairs NP");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP]);
        strcpy (CharTemp, "ClosePairs mNP");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP1]);
        strcpy (CharTemp, "ClosePairs mNP1");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP2]);
        strcpy (CharTemp, "ClosePairs mNP2");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_NJumps]);
        strcpy (CharTemp, "ClosePairs NJumps");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);

        tr.pVal.push_back(res->pVal[snpair_mNP2S]);
        strcpy (CharTemp, "ClosePairs mNP2S");
        strncat (CharTemp, mess, (size_t) len);
        tr.name.push_back(CharTemp);
        snpair_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    snpair_ClosePairsBitMatch (gen, res, 4, 4 * MILLION, 0, 2);
      //    bbattery_pVal[++j] = res->pVal[snpair_BM];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 2");
      // }

      auto ClosePairsBitMatch1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        snpair_Res *res = snpair_CreateRes ();
        snpair_ClosePairsBitMatch (workload_Clone(gen), res, 4, 4 * MILLION, 0, 2);
        tr.pVal.push_back(res->pVal[snpair_BM]);
        tr.name.push_back("ClosePairsBitMatch, t = 2");
        snpair_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    snpair_ClosePairsBitMatch (gen, res, 2, 4 * MILLION, 0, 4);
      //    bbattery_pVal[++j] = res->pVal[snpair_BM];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 4");
      // }

      auto ClosePairsBitMatch2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        snpair_Res *res = snpair_CreateRes ();
        snpair_ClosePairsBitMatch (workload_Clone(gen), res, 2, 4 * MILLION, 0, 4);
        tr.pVal.push_back(res->pVal[snpair_BM]);
        tr.name.push_back("ClosePairsBitMatch, t = 4");
        snpair_DeleteRes (res);
        return tr;
      };

      // snpair_DeleteRes (res);
      // snpair_mNP2S_Flag = flag;

///////////////////////////////////////////////////////////////////////////

      // sres_Chi2 *res;
      // res = sres_CreateChi2 ();

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_SimpPoker (gen, res, 1, 40 * MILLION, 0, 16, 16);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SimpPoker, d = 16");
      // }

      auto SimPoker1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_SimpPoker (workload_Clone(gen), res, 1, 40 * MILLION, 0, 16, 16);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SimpPoker, d = 16");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_SimpPoker (gen, res, 1, 40 * MILLION, 26, 16, 16);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SimpPoker, d = 16");
      // }

      auto SimPoker2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_SimpPoker (workload_Clone(gen), res, 1, 40 * MILLION, 26, 16, 16);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SimpPoker, d = 16");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_SimpPoker (gen, res, 1, 10 * MILLION, 0, 64, 64);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SimpPoker, d = 64");
      // }

      auto SimPoker3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_SimpPoker (workload_Clone(gen), res, 1, 10 * MILLION, 0, 64, 64);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SimpPoker, d = 64");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_SimpPoker (gen, res, 1, 10 * MILLION, 24, 64, 64);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SimpPoker, d = 64");
      // }

      auto SimPoker4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_SimpPoker (workload_Clone(gen), res, 1, 10 * MILLION, 24, 64, 64);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SimpPoker, d = 64");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_CouponCollector (gen, res, 1, 40 * MILLION, 0, 4);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CouponCollector, d = 4");
      // }

      auto CouponCollector1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_CouponCollector (workload_Clone(gen), res, 1, 40 * MILLION, 0, 4);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("CouponCollector, d = 4");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_CouponCollector (gen, res, 1, 40 * MILLION, 28, 4);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CouponCollector, d = 4");
      // }

      auto CouponCollector2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_CouponCollector (workload_Clone(gen), res, 1, 40 * MILLION, 28, 4);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("CouponCollector, d = 4");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_CouponCollector (gen, res, 1, 10 * MILLION, 0, 16);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CouponCollector, d = 16");
      // }

      auto CouponCollector3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_CouponCollector (workload_Clone(gen), res, 1, 10 * MILLION, 0, 16);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("CouponCollector, d = 16");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_CouponCollector (gen, res, 1, 10 * MILLION, 26, 16);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CouponCollector, d = 16");
      // }

      auto CouponCollector4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_CouponCollector (workload_Clone(gen), res, 1, 10 * MILLION, 26, 16);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("CouponCollector, d = 16");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Gap (gen, res, 1, 100 * MILLION, 0, 0.0, 0.125);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Gap, r = 0");
      // }

      auto Gap1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Gap (workload_Clone(gen), res, 1, 100 * MILLION, 0, 0.0, 0.125);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Gap, r = 0");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Gap (gen, res, 1, 100 * MILLION, 27, 0.0, 0.125);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Gap, r = 27");
      // }

      auto Gap2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Gap (workload_Clone(gen), res, 1, 100 * MILLION, 27, 0.0, 0.125);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Gap, r = 27");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Gap (gen, res, 1, 5 * MILLION, 0, 0.0, 1.0/256.0);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Gap, r = 0");
      // }

      auto Gap3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Gap (workload_Clone(gen), res, 1, 5 * MILLION, 0, 0.0, 1.0/256.0);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Gap, r = 0");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Gap (gen, res, 1, 5 * MILLION, 22, 0.0, 1.0/256.0);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Gap, r = 22");
      // }

      auto Gap4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Gap (workload_Clone(gen), res, 1, 5 * MILLION, 22, 0.0, 1.0/256.0);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Gap, r = 22");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Run (gen, res, 1, 500 * MILLION, 0, TRUE);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Run of U01, r = 0");
      // }

      auto Run1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Run (workload_Clone(gen), res, 1, 500 * MILLION, 0, TRUE);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Run of U01, r = 0");
        sres_DeleteChi2 (res);
        return tr;
      };


      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Run (gen, res, 1, 500 * MILLION, 15, FALSE);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Run of U01, r = 15");
      // }

      auto Run2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Run (workload_Clone(gen), res, 1, 500 * MILLION, 15, FALSE);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Run of U01, r = 15");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Permutation (gen, res, 1, 50 * MILLION, 0, 10);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Permutation, r = 0");
      // }

      auto Permutation1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Permutation (workload_Clone(gen), res, 1, 50 * MILLION, 0, 10);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Permutation, r = 0");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_Permutation (gen, res, 1, 50 * MILLION, 15, 10);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Permutation, r = 15");
      // }

      auto Permutation2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sknuth_Permutation (workload_Clone(gen), res, 1, 50 * MILLION, 15, 10);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Permutation, r = 15");
        sres_DeleteChi2 (res);
        return tr;
      };

      // sknuth_Res2 *res3;
      // res3 = sknuth_CreateRes2 ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_CollisionPermut (gen, res3, 5, 10 * MILLION, 0, 13);
      //    bbattery_pVal[++j] = res3->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionPermut, r = 0");
      // }

      auto CollisionPermut1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sknuth_Res2 *res = sknuth_CreateRes2 ();
        sknuth_CollisionPermut (workload_Clone(gen), res, 5, 10 * MILLION, 0, 13);
        tr.pVal.push_back(res->Pois->pVal2);
        tr.name.push_back("CollisionPermut, r = 0");
        sknuth_DeleteRes2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_CollisionPermut (gen, res, 5, 10 * MILLION, 15, 13);
      //    bbattery_pVal[++j] = res->Pois->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "CollisionPermut, r = 15");
      // }

      auto CollisionPermut2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sknuth_Res2 *res = sknuth_CreateRes2 ();
        sknuth_CollisionPermut (workload_Clone(gen), res, 5, 10 * MILLION, 15, 13);
        tr.pVal.push_back(res->Pois->pVal2);
        tr.name.push_back("CollisionPermut, r = 15");
        sknuth_DeleteRes2 (res);
        return tr;
      };
      

      // sknuth_Res1 *res;
      // res = sknuth_CreateRes1 ();

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_MaxOft (gen, res, 10, 10 * MILLION, 0, MILLION / 10, 5);
      //    bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft, t = 5");
      //    bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft AD, t = 5");
      // }

      auto MaxOft1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sknuth_Res1 *res = sknuth_CreateRes1 ();
        sknuth_MaxOft (workload_Clone(gen), res, 10, 10 * MILLION, 0, MILLION / 10, 5);
        tr.pVal.push_back(res->Chi->pVal2[gofw_Sum]);
        tr.name.push_back("MaxOft, t = 5");
        tr.pVal.push_back(res->Bas->pVal2[gofw_AD]);
        tr.name.push_back("MaxOft AD, t = 5");
        sknuth_DeleteRes1 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_MaxOft (gen, res, 5, 10 * MILLION, 0, MILLION / 10, 10);
      //    bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft, t = 10");
      //    bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft AD, t = 10");
      // }

      auto MaxOft2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sknuth_Res1 *res = sknuth_CreateRes1 ();
        sknuth_MaxOft (workload_Clone(gen), res, 5, 10 * MILLION, 0, MILLION / 10, 10);
        tr.pVal.push_back(res->Chi->pVal2[gofw_Sum]);
        tr.name.push_back("MaxOft, t = 10");
        tr.pVal.push_back(res->Bas->pVal2[gofw_AD]);
        tr.name.push_back("MaxOft AD, t = 10");
        sknuth_DeleteRes1 (res);
        return tr;
      };


      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_MaxOft (gen, res, 1, 10 * MILLION, 0, MILLION / 10, 20);
      //    bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft, t = 20");
      //    bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft AD, t = 20");
      // }

      auto MaxOft3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sknuth_Res1 *res = sknuth_CreateRes1 ();
        sknuth_MaxOft (workload_Clone(gen), res, 1, 10 * MILLION, 0, MILLION / 10, 20);
        tr.pVal.push_back(res->Chi->pVal2[gofw_Mean]);
        tr.name.push_back("MaxOft, t = 20");
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("MaxOft AD, t = 20");
        sknuth_DeleteRes1 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sknuth_MaxOft (gen, res, 1, 10 * MILLION, 0, MILLION / 10, 30);
      //    bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft, t = 30");
      //    bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MaxOft AD, t = 30");
      // }

      auto MaxOft4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sknuth_Res1 *res = sknuth_CreateRes1 ();
        sknuth_MaxOft (workload_Clone(gen), res, 1, 10 * MILLION, 0, MILLION / 10, 30);
        tr.pVal.push_back(res->Chi->pVal2[gofw_Mean]);
        tr.name.push_back("MaxOft, t = 30");
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("MaxOft AD, t = 30");
        sknuth_DeleteRes1 (res);
        return tr;
      };

      // sres_Basic *res;
      // res = sres_CreateBasic ();

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_SampleProd (gen, res, 1, 10 * MILLION, 0, 10);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SampleProd, t = 10");
      // }

      auto SampleProd1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        svaria_SampleProd (workload_Clone(gen), res, 1, 10 * MILLION, 0, 10);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SampleProd, t = 10");
        sres_DeleteBasic (res);
        return tr;      
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_SampleProd (gen, res, 1, 10 * MILLION, 0, 30);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SampleProd, t = 30");
      // }

      auto SampleProd2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        svaria_SampleProd (workload_Clone(gen), res, 1, 10 * MILLION, 0, 30);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SampleProd, t = 30");
        sres_DeleteBasic (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_SampleMean (gen, res, 10*MILLION, 20, 0);
      //    bbattery_pVal[++j] = res->pVal2[gofw_AD];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SampleMean");
      // }

      auto SampleMean1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        svaria_SampleMean (workload_Clone(gen), res, 10*MILLION, 20, 0);
        tr.pVal.push_back(res->pVal2[gofw_AD]);
        tr.name.push_back("SampleMean");
        sres_DeleteBasic (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_SampleCorr (gen, res, 1, 500 * MILLION, 0, 1);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SampleCorr");
      // }

      auto SampleCorr1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        svaria_SampleCorr (workload_Clone(gen), res, 1, 500 * MILLION, 0, 1);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SampleCorr");
        sres_DeleteBasic (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_AppearanceSpacings (gen, res, 1, 10 * MILLION, 400 * MILLION,
      //       r, 30, 15);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "AppearanceSpacings, r = 0");
      // }

      auto AppearanceSpacings1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        svaria_AppearanceSpacings (workload_Clone(gen), res, 1, 10 * MILLION, 400 * MILLION,
            r, 30, 15);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("AppearanceSpacings, r = 0");
        sres_DeleteBasic (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_AppearanceSpacings (gen, res, 1, 10 * MILLION, 100 * MILLION,
      //       20, 10, 15);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "AppearanceSpacings, r = 20");
      // }

      auto AppearanceSpacings2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        svaria_AppearanceSpacings (workload_Clone(gen), res, 1, 10 * MILLION, 100 * MILLION,
            20, 10, 15);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("AppearanceSpacings, r = 20");
        sres_DeleteBasic (res);
        return tr; 
      };



      // smarsa_Res2 *res6;
      // sres_Chi2 *res;
      // res = sres_CreateChi2 ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_WeightDistrib (gen, res, 1, 2 * MILLION, 0, 256, 0.0, 0.125);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "WeightDistrib, r = 0");
      // }

      auto WeightDistrib1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 0, 256, 0.0, 0.125);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("WeightDistrib, r = 0");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_WeightDistrib (gen, res, 1, 2 * MILLION, 8, 256, 0.0, 0.125);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "WeightDistrib, r = 8");
      // }

      auto WeightDistrib2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 8, 256, 0.0, 0.125);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("WeightDistrib, r = 8");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_WeightDistrib (gen, res, 1, 2 * MILLION, 16, 256, 0.0, 0.125);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "WeightDistrib, r = 16");
      // }

      auto WeightDistrib3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 16, 256, 0.0, 0.125);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("WeightDistrib, r = 16");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_WeightDistrib (gen, res, 1, 2 * MILLION, 24, 256, 0.0, 0.125);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "WeightDistrib, r = 24");
      // }

      auto WeightDistrib4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        svaria_WeightDistrib (workload_Clone(gen), res, 1, 2 * MILLION, 24, 256, 0.0, 0.125);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("WeightDistrib, r = 24");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    svaria_SumCollector (gen, res, 1, 20 * MILLION, 0, 10.0);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "SumCollector");
      // }

      auto SumCollector1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        svaria_SumCollector (workload_Clone(gen), res, 1, 20 * MILLION, 0, 10.0);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("SumCollector");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_MatrixRank (gen, res, 1, MILLION, r, s, 2 * s, 2 * s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MatrixRank, 60 x 60");
      // }

      auto MatrixRank1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        smarsa_MatrixRank (workload_Clone(gen), res, 1, MILLION, r, s, 2 * s, 2 * s);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("MatrixRank, 60 x 60");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_MatrixRank (gen, res, 1, MILLION, 20, 10, 2 * s, 2 * s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MatrixRank, 60 x 60");
      // }

      auto MatrixRank2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        smarsa_MatrixRank (workload_Clone(gen), res, 1, MILLION, 20, 10, 2 * s, 2 * s);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("MatrixRank, 60 x 60");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_MatrixRank (gen, res, 1, 50 * THOUSAND, r, s, 10 * s, 10 * s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MatrixRank, 300 x 300");
      // }

      auto MatrixRank3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        smarsa_MatrixRank (workload_Clone(gen), res, 1, 50 * THOUSAND, r, s, 10 * s, 10 * s);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("MatrixRank, 300 x 300");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_MatrixRank (gen, res, 1, 50 * THOUSAND, 20, 10, 10 * s,
      //       10 * s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MatrixRank, 300 x 300");
      // }

      auto MatrixRank4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        smarsa_MatrixRank (workload_Clone(gen), res, 1, 50 * THOUSAND, 20, 10, 10 * s,
            10 * s);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("MatrixRank, 300 x 300");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_MatrixRank (gen, res, 1, 2 * THOUSAND, r, s, 40 * s, 40 * s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MatrixRank, 1200 x 1200");
      // }

      auto MatrixRank5 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        smarsa_MatrixRank (workload_Clone(gen), res, 1, 2 * THOUSAND, r, s, 40 * s, 40 * s);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("MatrixRank, 1200 x 1200");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_MatrixRank (gen, res, 1, 2 * THOUSAND, 20, 10, 40 * s, 40 * s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "MatrixRank, 1200 x 1200");
      // }

      auto MatrixRank6 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        smarsa_MatrixRank (workload_Clone(gen), res, 1, 2 * THOUSAND, 20, 10, 40 * s, 40 * s);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("MatrixRank, 1200 x 1200");
        sres_DeleteChi2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_Savir2 (gen, res, 1, 20 * MILLION, 0, 1024*1024, 30);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Savir2");
      // }

      auto Savir2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        smarsa_Savir2 (workload_Clone(gen), res, 1, 20 * MILLION, 0, 1024*1024, 30);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("Savir2");
        sres_DeleteChi2 (res);
        return tr; 
      };

      //res = smarsa_CreateRes2 ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_GCD (gen, res, 1, 100 * MILLION, 0, 30);
      //    bbattery_pVal[++j] = res->GCD->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "GCD, r = 0");
      // }

      auto GCD1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res2 *res = smarsa_CreateRes2 ();
        smarsa_GCD (workload_Clone(gen), res, 1, 100 * MILLION, 0, 30);
        tr.pVal.push_back(res->GCD->pVal2[gofw_Mean]);
        tr.name.push_back("GCD, r = 0");
        smarsa_DeleteRes2 (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    smarsa_GCD (gen, res, 1, 40 * MILLION, 10, 20);
      //    bbattery_pVal[++j] = res->GCD->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "GCD, r = 10");
      // }

      auto GCD2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        smarsa_Res2 *res = smarsa_CreateRes2 ();
        smarsa_GCD (workload_Clone(gen), res, 1, 40 * MILLION, 10, 20);
        tr.pVal.push_back(res->GCD->pVal2[gofw_Mean]);
        tr.name.push_back("GCD, r = 10");
        smarsa_DeleteRes2 (res);
        return tr; 
      };

      // TO DO
      // swalk_Res *res;
      // res = swalk_CreateRes ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    swalk_RandomWalk1 (gen, res, 1, 50 * MILLION, r, s, 90, 90);
      //    GetPVal_Walk (1, res, &j, " (L = 90)", j2, globals);
      // const unsigned int len = 20;
      // char CharTemp[LEN + 1];
      // const char *mess = " (L = 90)";

      //     bbattery_pVal[++j] = res->H[0]->pVal2[gofw_Mean];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "RandomWalk1 H");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      //     bbattery_pVal[++j] = res->M[0]->pVal2[gofw_Mean];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "RandomWalk1 M");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      //     bbattery_pVal[++j] = res->J[0]->pVal2[gofw_Mean];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "RandomWalk1 J");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      //     bbattery_pVal[++j] = res->R[0]->pVal2[gofw_Mean];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "RandomWalk1 R");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);

      //     bbattery_pVal[++j] = res->C[0]->pVal2[gofw_Mean];
      //     TestNumber[j] = j2;
      //     strcpy (CharTemp, "RandomWalk1 C");
      //     strncat (CharTemp, mess, (size_t) len);
      //     strncpy (bbattery_TestNames[j], CharTemp, (size_t) LEN);
      // }

      auto RandomWalk11 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        swalk_Res *res = swalk_CreateRes ();
        swalk_RandomWalk1 (workload_Clone(gen), res, 1, 50 * MILLION, r, s, 90, 90);
        const unsigned int len = 20;
        char CharTemp[LEN + 1];
        const char *mess = " (L = 90)";
        char tmpName[LEN + 1];

        tr.pVal.push_back(res->H[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 H");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->M[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 M");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->J[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 J");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->R[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 R");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName); 

        tr.pVal.push_back(res->C[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 C");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        swalk_DeleteRes (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    swalk_RandomWalk1 (gen, res, 1, 10 * MILLION, 20, 10, 90, 90);
      //    GetPVal_Walk (1, res, &j, " (L = 90)", j2, globals);
      // }

      auto RandomWalk12 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        swalk_Res *res = swalk_CreateRes ();
        swalk_RandomWalk1 (workload_Clone(gen), res, 1, 10 * MILLION, 20, 10, 90, 90);
        const unsigned int len = 20;
        char CharTemp[LEN + 1];
        const char *mess = " (L = 90)";
        char tmpName[LEN + 1];

        tr.pVal.push_back(res->H[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 H");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->M[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 M");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->J[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 J");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->R[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 R");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName); 

        tr.pVal.push_back(res->C[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 C");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        swalk_DeleteRes (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    swalk_RandomWalk1 (gen, res, 1, 5 * MILLION, r, s, 1000, 1000);
      //    GetPVal_Walk (1, res, &j, " (L = 1000)", j2, globals);
      // }

        auto RandomWalk13 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        swalk_Res *res = swalk_CreateRes ();
        swalk_RandomWalk1 (workload_Clone(gen), res, 1, 5 * MILLION, r, s, 1000, 1000);
        const unsigned int len = 20;
        char CharTemp[LEN + 1];
        const char *mess = " (L = 1000)";
        char tmpName[LEN + 1];

        tr.pVal.push_back(res->H[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 H");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->M[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 M");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->J[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 J");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->R[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 R");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName); 

        tr.pVal.push_back(res->C[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 C");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        swalk_DeleteRes (res);
        return tr; 
      };


      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    swalk_RandomWalk1 (gen, res, 1, MILLION, 20, 10, 1000, 1000);
      //    GetPVal_Walk (1, res, &j, " (L = 1000)", j2, globals);
      // }

      auto RandomWalk14 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        swalk_Res *res = swalk_CreateRes ();
        swalk_RandomWalk1 (workload_Clone(gen), res, 1, MILLION, 20, 10, 1000, 1000);
        const unsigned int len = 20;
        char CharTemp[LEN + 1];
        const char *mess = " (L = 1000)";
        char tmpName[LEN + 1];

        tr.pVal.push_back(res->H[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 H");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->M[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 M");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->J[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 J");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->R[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 R");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName); 

        tr.pVal.push_back(res->C[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 C");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        swalk_DeleteRes (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    swalk_RandomWalk1 (gen, res, 1, MILLION / 2, r, s, 10000, 10000);
      //    GetPVal_Walk (1, res, &j, " (L = 10000)", j2, globals);
      // }

        auto RandomWalk15 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        swalk_Res *res = swalk_CreateRes ();
        swalk_RandomWalk1 (workload_Clone(gen), res, 1, MILLION / 2, r, s, 10000, 10000);
        const unsigned int len = 20;
        char CharTemp[LEN + 1];
        const char *mess = " (L = 10000)";
        char tmpName[LEN + 1];

        tr.pVal.push_back(res->H[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 H");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->M[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 M");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->J[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 J");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->R[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 R");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName); 

        tr.pVal.push_back(res->C[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 C");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        swalk_DeleteRes (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    swalk_RandomWalk1 (gen, res, 1, MILLION / 10, 20, 10, 10000, 10000);
      //    GetPVal_Walk (1, res, &j, " (L = 10000)", j2, globals);
      // }

      auto RandomWalk16 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        swalk_Res *res = swalk_CreateRes ();
        swalk_RandomWalk1 (workload_Clone(gen), res, 1, MILLION / 10, 20, 10, 10000, 10000);
        const unsigned int len = 20;
        char CharTemp[LEN + 1];
        const char *mess = " (L = 10000)";
        char tmpName[LEN + 1];

        tr.pVal.push_back(res->H[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 H");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->M[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 M");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->J[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 J");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        tr.pVal.push_back(res->R[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 R");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName); 

        tr.pVal.push_back(res->C[0]->pVal2[gofw_Mean]);
        strcpy (CharTemp, "RandomWalk1 C");
        strncat (CharTemp, mess, (size_t) len);
        strncpy (tmpName, CharTemp, (size_t) LEN);
        tr.name.push_back(tmpName);

        swalk_DeleteRes (res);
        return tr; 
      };
      
      

      // scomp_Res *res;
      // res = scomp_CreateRes ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    scomp_LinearComp (gen, res, 1, 120 * THOUSAND, r, 1);
      //    bbattery_pVal[++j] = res->JumpNum->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LinearComp, r = 0");
      //    bbattery_pVal[++j] = res->JumpSize->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LinearComp, r = 0");
      // }

      auto LinearComp1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        scomp_Res *res = scomp_CreateRes ();
        scomp_LinearComp (workload_Clone(gen), res, 1, 120 * THOUSAND, r, 1);
        tr.pVal.push_back(res->JumpNum->pVal2[gofw_Mean]);
        tr.name.push_back("LinearComp, r = 0");
        tr.pVal.push_back(res->JumpSize->pVal2[gofw_Mean]);
        tr.name.push_back("LinearComp, r = 0");
        scomp_DeleteRes (res);
        return tr; 
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    scomp_LinearComp (gen, res, 1, 120 * THOUSAND, 29, 1);
      //    bbattery_pVal[++j] = res->JumpNum->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LinearComp, r = 29");
      //    bbattery_pVal[++j] = res->JumpSize->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LinearComp, r = 29");
      // }

      auto LinearComp2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        scomp_Res *res = scomp_CreateRes ();
        scomp_LinearComp (workload_Clone(gen), res, 1, 120 * THOUSAND, 29, 1);
        tr.pVal.push_back(res->JumpNum->pVal2[gofw_Mean]);
        tr.name.push_back("LinearComp, r = 29");
        tr.pVal.push_back(res->JumpSize->pVal2[gofw_Mean]);
        tr.name.push_back("LinearComp, r = 29");
        scomp_DeleteRes (res);
        return tr; 
      };

      // PICKED UP HERE ON FRIDAY
      
      // sres_Basic *res;
      // res = sres_CreateBasic ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    scomp_LempelZiv (gen, res, 10, 25, r, s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LempelZiv");
      // }

      auto LempelZiv = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        scomp_LempelZiv (workload_Clone(gen), res, 10, 25, r, s);
        tr.pVal.push_back(res->pVal2[gofw_Sum]);
        tr.name.push_back("LempelZiv");
        sres_DeleteBasic (res);
        return tr; 
      };
      

      // sspectral_Res *res11;
      // res11 = sspectral_CreateRes ();

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sspectral_Fourier3 (gen, res11, 50 * THOUSAND, 14, r, s);
      //    bbattery_pVal[++j] = res11->Bas->pVal2[gofw_AD];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Fourier3, r = 0");
      // }

      auto Fourier31 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sspectral_Res *res = sspectral_CreateRes ();
        sspectral_Fourier3 (workload_Clone(gen), res, 50 * THOUSAND, 14, r, s);
        tr.pVal.push_back(res->Bas->pVal2[gofw_AD]);
        tr.name.push_back("Fourier3, r = 0");
        sspectral_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sspectral_Fourier3 (gen, res11, 50 * THOUSAND, 14, 20, 10);
      //    bbattery_pVal[++j] = res11->Bas->pVal2[gofw_AD];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Fourier3, r = 20");
      // }

      auto Fourier32 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sspectral_Res *res = sspectral_CreateRes ();
        sspectral_Fourier3 (workload_Clone(gen), res, 50 * THOUSAND, 14, 20, 10);
        tr.pVal.push_back(res->Bas->pVal2[gofw_AD]);
        tr.name.push_back("Fourier3, r = 20");
        sspectral_DeleteRes (res);
        return tr;
      };

      // sstring_Res2 *res12;
      // res12 = sstring_CreateRes2 ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_LongestHeadRun (gen, res12, 1, 1000, r, s, 20 + 10 * MILLION);
      //    bbattery_pVal[++j] = res12->Chi->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 0");
      //    bbattery_pVal[++j] = res12->Disc->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 0");
      // }

      auto LongestHeadRun1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res2 *res = sstring_CreateRes2 ();
        sstring_LongestHeadRun (workload_Clone(gen), res, 1, 1000, r, s, 20 + 10 * MILLION);
        tr.pVal.push_back(res->Chi->pVal2[gofw_Mean]);
        tr.name.push_back("LongestHeadRun, r = 0");
        tr.pVal.push_back(res->Disc->pVal2);
        tr.name.push_back("LongestHeadRun, r = 0");
        sstring_DeleteRes2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_LongestHeadRun (gen, res12, 1, 300, 20, 10, 20 + 10 * MILLION);
      //    bbattery_pVal[++j] = res12->Chi->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 20");
      //    bbattery_pVal[++j] = res12->Disc->pVal2;
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 20");
      // }
      
      auto LongestHeadRun2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res2 *res = sstring_CreateRes2 ();
        sstring_LongestHeadRun (workload_Clone(gen), res, 1, 300, 20, 10, 20 + 10 * MILLION);
        tr.pVal.push_back(res->Chi->pVal2[gofw_Mean]);
        tr.name.push_back("LongestHeadRun, r = 20");
        tr.pVal.push_back(res->Disc->pVal2);
        tr.name.push_back("LongestHeadRun, r = 20");
        sstring_DeleteRes2 (res);
        return tr;
      };

      // sres_Chi2 *res13;
      // res13 = sres_CreateChi2 ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_PeriodsInStrings (gen, res13, 1, 300 * MILLION, r, s);
      //    bbattery_pVal[++j] = res13->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "PeriodsInStrings, r = 0");
      // }

      auto PeriodsInStrings1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sstring_PeriodsInStrings (workload_Clone(gen), res, 1, 300 * MILLION, r, s);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("PeriodsInStrings, r = 0");
        sres_DeleteChi2 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_PeriodsInStrings (gen, res13, 1, 300 * MILLION, 15, 15);
      //    bbattery_pVal[++j] = res13->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "PeriodsInStrings, r = 15");
      // }

      auto PeriodsInStrings2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Chi2 *res = sres_CreateChi2 ();
        sstring_PeriodsInStrings (workload_Clone(gen), res, 1, 300 * MILLION, 15, 15);
        tr.pVal.push_back(res->pVal2[gofw_Mean]);
        tr.name.push_back("PeriodsInStrings, r = 15");
        sres_DeleteChi2 (res);
        return tr;
      };
   
      // sres_Basic *res14;
      // res14 = sres_CreateBasic ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingWeight2 (gen, res14, 100, 100 * MILLION, r, s, MILLION);
      //    bbattery_pVal[++j] = res14->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingWeight2, r = 0");
      // }

      auto HammingWeight21 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        sstring_HammingWeight2 (workload_Clone(gen), res, 100, 100 * MILLION, r, s, MILLION);
        tr.pVal.push_back(res->pVal2[gofw_Sum]);
        tr.name.push_back("HammingWeight2, r = 0");
        sres_DeleteBasic (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingWeight2 (gen, res14, 30, 100 * MILLION, 20, 10, MILLION);
      //    bbattery_pVal[++j] = res14->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingWeight2, r = 20");
      // }

      auto HammingWeight22 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        sstring_HammingWeight2 (workload_Clone(gen), res, 30, 100 * MILLION, 20, 10, MILLION);
        tr.pVal.push_back(res->pVal2[gofw_Sum]);
        tr.name.push_back("HammingWeight2, r = 20");
        sres_DeleteBasic (res);
        return tr;
      };

      // sstring_Res *res15;
      // res15 = sstring_CreateRes ();
      // /* sstring_HammingCorr will probably be removed: less sensitive than
      //    svaria_HammingIndep */
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingCorr (gen, res15, 1, 500 * MILLION, r, s, s);
      //    bbattery_pVal[++j] = res15->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingCorr, L = 30");
      // }

      auto HammingCorr1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingCorr (workload_Clone(gen), res, 1, 500 * MILLION, r, s, s);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingCorr, L = 30");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingCorr (gen, res15, 1, 50 * MILLION, r, s, 10 * s);
      //    bbattery_pVal[++j] = res15->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingCorr, L = 300");
      // }

      auto HammingCorr2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingCorr (workload_Clone(gen), res, 1, 50 * MILLION, r, s, 10 * s);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingCorr, L = 300");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingCorr (gen, res15, 1, 10 * MILLION, r, s, 40 * s);
      //    bbattery_pVal[++j] = res15->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingCorr, L = 1200");
      // }

      auto HammingCorr3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingCorr (workload_Clone(gen), res, 1, 10 * MILLION, r, s, 40 * s);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingCorr, L = 1200");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingIndep (gen, res15, 1, 300 * MILLION, r, s, s, 0);
      //    bbattery_pVal[++j] = res15->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingIndep, L = 30");
      // }

      auto HammingIndep1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingIndep (workload_Clone(gen), res, 1, 300 * MILLION, r, s, s, 0);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingIndep, L = 30");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingIndep (gen, res15, 1, 100 * MILLION, 20, 10, s, 0);
      //    bbattery_pVal[++j] = res15->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingIndep, L = 30");
      // }

      auto HammingIndep2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingIndep (workload_Clone(gen), res, 1, 100 * MILLION, 20, 10, s, 0);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingIndep, L = 30");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingIndep (gen, res15, 1, 30 * MILLION, r, s, 10 * s, 0);
      //    bbattery_pVal[++j] = res15->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingIndep, L = 300");
      // }

      auto HammingIndep3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingIndep (workload_Clone(gen), res, 1, 30 * MILLION, r, s, 10 * s, 0);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingIndep, L = 300");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingIndep (gen, res15, 1, 10 * MILLION, 20, 10, 10 * s, 0);
      //    bbattery_pVal[++j] = res15->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingIndep, L = 300");
      // }

      auto HammingIndep4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingIndep (workload_Clone(gen), res, 1, 10 * MILLION, 20, 10, 10 * s, 0);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingIndep, L = 300");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingIndep (gen, res, 1, 10 * MILLION, r, s, 40 * s, 0);
      //    bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingIndep, L = 1200");
      // }

      auto HammingIndep5 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingIndep (workload_Clone(gen), res, 1, 10 * MILLION, r, s, 40 * s, 0);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingIndep, L = 1200");
        sstring_DeleteRes (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_HammingIndep (gen, res, 1, MILLION, 20, 10, 40 * s, 0);
      //    bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "HammingIndep, L = 1200");
      // }

      auto HammingIndep6 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res *res = sstring_CreateRes ();
        sstring_HammingIndep (workload_Clone(gen), res, 1, MILLION, 20, 10, 40 * s, 0);
        tr.pVal.push_back(res->Bas->pVal2[gofw_Mean]);
        tr.name.push_back("HammingIndep, L = 1200");
        sstring_DeleteRes (res);
        return tr;
      };



      // sstring_Res3 *res;
      // res = sstring_CreateRes3 ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_Run (gen, res, 1, 1 * BILLION, r, s);
      //    bbattery_pVal[++j] = res->NRuns->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Run of bits, r = 0");
      //    bbattery_pVal[++j] = res->NBits->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Run of bits, r = 0");
      // }

      auto Run11 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res3 *res = sstring_CreateRes3 ();
        sstring_Run (workload_Clone(gen), res, 1, 1 * BILLION, r, s);
        tr.pVal.push_back(res->NRuns->pVal2[gofw_Mean]);
        tr.name.push_back("Run of bits, r = 0");
        tr.pVal.push_back(res->NBits->pVal2[gofw_Mean]);
        tr.name.push_back("Run of bits, r = 0");
        sstring_DeleteRes3 (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_Run (gen, res, 1, 1 * BILLION, 20, 10);
      //    bbattery_pVal[++j] = res->NRuns->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Run of bits, r = 20");
      //    bbattery_pVal[++j] = res->NBits->pVal2[gofw_Mean];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "Run of bits, r = 20");
      // }

      auto Run22 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sstring_Res3 *res = sstring_CreateRes3 ();
        sstring_Run (workload_Clone(gen), res, 1, 1 * BILLION, 20, 10);
        tr.pVal.push_back(res->NRuns->pVal2[gofw_Mean]);
        tr.name.push_back("Run of bits, r = 20");
        tr.pVal.push_back(res->NBits->pVal2[gofw_Mean]);
        tr.name.push_back("Run of bits, r = 20");
        sstring_DeleteRes3 (res);
        return tr;
      };

      
      // sres_Basic *res;
      // res = sres_CreateBasic ();
      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_AutoCor (gen, res, 10, 30 + BILLION, r, s, 1);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "AutoCor, d = 1");
      // }

      auto AutoCor1 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        sstring_AutoCor (workload_Clone(gen), res, 10, 30 + BILLION, r, s, 1);
        tr.pVal.push_back(res->pVal2[gofw_Sum]);
        tr.name.push_back("AutoCor, d = 1");
        sres_DeleteBasic (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_AutoCor (gen, res, 5, 1 + BILLION, 20, 10, 1);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "AutoCor, d = 1");
      // }

      auto AutoCor2 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        sstring_AutoCor (workload_Clone(gen), res, 5, 1 + BILLION, 20, 10, 1);
        tr.pVal.push_back(res->pVal2[gofw_Sum]);
        tr.name.push_back("AutoCor, d = 1");
        sres_DeleteBasic (res);
        return tr;
      };

      // ++j2;
      // for (i = 0; i < Rep[j2]; ++i) {
      //    sstring_AutoCor (gen, res, 10, 31 + BILLION, r, s, s);
      //    bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      //    TestNumber[j] = j2;
      //    strcpy (bbattery_TestNames[j], "AutoCor, d = 30");
      // }

      auto AutoCor3 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        sstring_AutoCor (workload_Clone(gen), res, 10, 31 + BILLION, r, s, s);
        tr.pVal.push_back(res->pVal2[gofw_Sum]);
        tr.name.push_back("AutoCor, d = 30");
        sres_DeleteBasic (res);
        return tr;
      };

//       ++j2;
//  /*     util_Assert (j2 <= CRUSH_NUM, "Crush:   j2 > CRUSH_NUM");  */
//       for (i = 0; i < Rep[j2]; ++i) {
//          sstring_AutoCor (gen, res, 5, 11 + BILLION, 20, 10, 10);
//          bbattery_pVal[++j] = res->pVal2[gofw_Sum];
//          TestNumber[j] = j2;
//          strcpy (bbattery_TestNames[j], "AutoCor, d = 10");
//       }

      auto AutoCor4 = [&](int testnum){
        TestRes tr; 
        // std::cout << "test: " << testnum << std::endl;
        sres_Basic *res = sres_CreateBasic ();
        sstring_AutoCor (workload_Clone(gen), res, 5, 11 + BILLION, 20, 10, 10);
        tr.pVal.push_back(res->pVal2[gofw_Sum]);
        tr.name.push_back("AutoCor, d = 10");
        sres_DeleteBasic (res);
        return tr;
      };

      std::vector<std::function<TestRes(int)>> testVec = {SerialOver1, SerialOver2, 
       CollisionOver1, CollisionOver2, CollisionOver3,CollisionOver4,CollisionOver5,CollisionOver6,CollisionOver7,CollisionOver8,
       BirthdaySpacings1, BirthdaySpacings2, BirthdaySpacings3, BirthdaySpacings4, BirthdaySpacings5, BirthdaySpacings6, BirthdaySpacings7,
       ClosePairs1, ClosePairs2, ClosePairs3,
       ClosePairsBitMatch1, ClosePairsBitMatch2,
       SimPoker1, SimPoker2, SimPoker3, SimPoker4,
       CouponCollector1, CouponCollector2, CouponCollector3, CouponCollector4,
       Gap1, Gap2, Gap3, Gap4,
       Run1, Run2,
       Permutation1, Permutation2,
       CollisionPermut1, CollisionPermut2,
       MaxOft1, MaxOft2, MaxOft3, MaxOft4,
       SampleProd1, SampleProd2,
       SampleMean1,
       SampleCorr1,
       AppearanceSpacings1, AppearanceSpacings2,
       WeightDistrib1, WeightDistrib2, WeightDistrib3, WeightDistrib4,
       SumCollector1, 
       MatrixRank1, MatrixRank2, MatrixRank3, MatrixRank4, MatrixRank5, MatrixRank6,
       Savir2,
       GCD1, GCD2,
       RandomWalk11, RandomWalk12, RandomWalk13, RandomWalk14, RandomWalk15, RandomWalk16,
       LinearComp1, LinearComp2,
       LempelZiv,
       Fourier31, Fourier32,
       LongestHeadRun1, LongestHeadRun2,
       PeriodsInStrings1, PeriodsInStrings2,
       HammingWeight21, HammingWeight22,
       HammingCorr1, HammingCorr2, HammingCorr3,
       HammingIndep1, HammingIndep2, HammingIndep3, HammingIndep4, HammingIndep5, HammingIndep6,
       Run11, Run22, 
       AutoCor1, AutoCor2, AutoCor3, AutoCor4 };

      std::vector<int> TestIndices = {
        8, 5, 6, 7, 4, 9, 60, 40, 3, 75, 1, 58, 59, 2, 12, 19, 39, 57, 11, 38, 10, 33, 41, 16, 15, 14, 64, 48, 56, 13, 93, 
        66, 32, 18, 95, 68, 91, 37, 23, 25, 55, 31, 88, 76, 74, 20, 62, 94, 36, 73, 92, 22, 17, 90, 84, 24, 86, 30, 21, 81, 77, 35, 
        72, 85, 29, 82, 51, 52, 87, 28, 53, 46, 61, 78, 34, 47, 43, 83, 50, 27, 45, 26, 79, 54, 42, 49, 80, 67, 
        65, 69, 63, 89, 44, 70, 71, 0 //Put test 0 last in order to be first in the taskgroup queue 
        };

      std::vector<TestRes> OutVec(testVec.size());
      
      for (int i=0; i<testVec.size(); i++){
        group.run([i, &TestIndices, &OutVec, &testVec](){
          OutVec[TestIndices[i]] = testVec[TestIndices[i]](TestIndices[i]);
          });
      }
      group.wait();

      /*
      tbb::parallel_for(size_t(0),testVec.size(),[&](int testnum){
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        std::cout << "Executing..." << testnum << std::endl;
        
        TestRes tr = testVec[testnum](testnum);
        // bbattery_pVal[testnum] = tr.pVal;
        //   TestNumber[testnum] = testnum+1;
        //   strcpy (bbattery_TestNames[testnum], &tr.name[0]);
        OutVec[testnum] = tr;
        // std::cout << "Test number: " << testnum <<  " test name: " << tr.name[0] << "\t\t" << " p value:  " << tr.pVal[0];
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        milliseconds time_span = duration_cast<milliseconds>(t2 - t1);
        // fprintf(stdout, "%i TimeUsed, 0, %i\n", testnum , time_span);
      });
      */

   int j = -1;

   for (int i=0; i<OutVec.size(); i++){
     for (int k=0; k<OutVec[i].name.size(); k++){
        bbattery_pVal[++j] = OutVec[i].pVal[k];
        TestNumber[j] = i+1;
        strcpy (bbattery_TestNames[j], OutVec[i].name[k].c_str());
     }
   }

   bbattery_NTests = ++j;
   GetName (gen, genName);
   if(swrite_Basic){
      WriteReport (genName, "Crush", bbattery_NTests,
         bbattery_pVal, Timer, FALSE, TRUE, 0.0, globals);
   }
   chrono_Delete (Timer);
}

/*=========================================================================*/

std::vector<bbattery_Result> bbattery_Crush (unif01_Gen * gen)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= CRUSH_NUM; ++i)
      Rep[i] = 1;
   Crush (gen, Rep, globals);
   return makeResults(globals);
}

std::vector<bbattery_Result> bbattery_CrushTBB (unif01_Gen * gen)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= CRUSH_NUM; ++i)
      Rep[i] = 1;
   CrushTBB (gen, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_RepeatCrush (unif01_Gen * gen, int Rep[])
{
  TestGlobals globals;
  Crush (gen, Rep, globals);
  return makeResults(globals);
}


/*=========================================================================*/

void BigCrush (unif01_Gen * gen, int Rep[], TestGlobals &globals)
/*
 * A battery of very stringent statistical tests for Random Number Generators
 * used in simulation.
 * Rep[i] gives the number of times that test i will be done. The default
 * values are Rep[i] = 1 for all i.
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
   const int s = 30;
   const int r = 0;
   int i;
   chrono_Chrono *Timer;
   char genName[LEN + 1] = "";
   int j = -1;
   int j2 = 0;

   Timer = chrono_Create ();
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "                 Starting BigCrush\n"
         "                 Version: %s\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n",
         PACKAGE_STRING);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_SerialOver (gen, res, 1, BILLION, 0, 256, 3);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SerialOver, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_SerialOver (gen, res, 1, BILLION, 22, 256, 3);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SerialOver, r = 22");
      }
      sres_DeleteBasic (res);
   }
   {
      smarsa_Res *resm;
      resm = smarsa_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 0, 1024*1024*2, 2);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 9, 1024*1024*2, 2);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 0, 1024*16, 3);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 3");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 16, 1024*16, 3);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 3");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 0, 64, 7);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 7");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 24, 64, 7);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 7");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 0, 8, 14);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 14");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 27, 8, 14);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 14");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 0, 4, 21);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 21");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_CollisionOver (gen, resm, 30, 20 * MILLION, 28, 4, 21);
         bbattery_pVal[++j] = resm->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionOver, t = 21");
      }
      smarsa_DeleteRes (resm);
   }
   {
      sres_Poisson *res;
      res = sres_CreatePoisson ();
#ifdef USE_LONGLONG
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         long d;
#if LONG_MAX <= 2147483647L
         d = 1073741824L;
         smarsa_BirthdaySpacings (gen, res, 250, 4 * MILLION, 0, d, 2, 1);
#else
         d = 2147483648L;
         smarsa_BirthdaySpacings (gen, res, 100, 10 * MILLION, 0, d, 2, 1);
#endif
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 20 * MILLION, 0, 2097152, 3,
            1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 3");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 30 * MILLION, 14, 65536, 4, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 20 * MILLION, 0, 512, 7, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 7");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 20 * MILLION, 7, 512, 7, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 7");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 30 * MILLION, 14, 256, 8, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 30 * MILLION, 22, 256, 8, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 30 * MILLION, 0, 16, 16, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 20, 30 * MILLION, 26, 16, 16, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 16");
      }

#else
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 0,
            67108864, 2, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 0,
            1024 * 8, 4, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 16,
            1024 * 8, 4, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 4");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 0, 16,
            13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 5, 16,
            13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 10,
            16, 13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 15,
            16, 13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 20,
            16, 13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_BirthdaySpacings (gen, res, 10 * THOUSAND, MILLION / 10, 26,
            16, 13, 1);
         bbattery_pVal[++j] = res->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings, t = 13");
      }
#endif
      sres_DeletePoisson (res);
   }
   {
      lebool flag = snpair_mNP2S_Flag;
      snpair_Res *res;
      res = snpair_CreateRes ();

      snpair_mNP2S_Flag = TRUE;
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairs (gen, res, 30, 6 * MILLION, 0, 3, 0, 30);
         GetPVal_CPairs (40, res, &j, ", t = 3", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairs (gen, res, 20, 4 * MILLION, 0, 5, 0, 30);
         GetPVal_CPairs (40, res, &j, ", t = 5", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairs (gen, res, 10, 3 * MILLION, 0, 9, 0, 30);
         GetPVal_CPairs (20, res, &j, ", t = 9", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairs (gen, res, 5, 2*MILLION, 0, 16, 0, 30);
         GetPVal_CPairs (10, res, &j, ", t = 16", j2, globals);
      }
      snpair_DeleteRes (res);
      snpair_mNP2S_Flag =flag;
   }
   {
      sres_Chi2 *res;
      res = sres_CreateChi2 ();

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (gen, res, 1, 400 * MILLION, 0, 8, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (gen, res, 1, 400 * MILLION, 27, 8, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, r = 27");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (gen, res, 1, 100 * MILLION, 0, 32, 32);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_SimpPoker (gen, res, 1, 100 * MILLION, 25, 32, 32);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SimpPoker, r = 25");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (gen, res, 1, 200 * MILLION, 0, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (gen, res, 1, 200 * MILLION, 10, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, r = 10");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (gen, res, 1, 200 * MILLION, 20, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, r = 20");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CouponCollector (gen, res, 1, 200 * MILLION, 27, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CouponCollector, r = 27");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (gen, res, 1, BILLION/2, 0, 0.0, 1.0/16.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (gen, res, 1, 300*MILLION, 25, 0.0, 1.0/32.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 25");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (gen, res, 1, BILLION/10, 0, 0.0, 1.0/128.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Gap (gen, res, 1, 10*MILLION, 20, 0.0, 1.0/1024.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Gap, r = 20");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Run (gen, res, 5, BILLION, 0, FALSE);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Run (gen, res, 10, BILLION, 15, TRUE);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run, r = 15");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Permutation (gen, res, 1, BILLION, 5, 3);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Permutation, t = 3" );
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Permutation (gen, res, 1, BILLION, 5, 5);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Permutation, t = 5");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Permutation (gen, res, 1, BILLION/2, 5, 7);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Permutation, t = 7");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_Permutation (gen, res, 1, BILLION/2, 10, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Permutation, t = 10");
      }
      sres_DeleteChi2 (res);
   }
   {
      sknuth_Res2 *res;
      res = sknuth_CreateRes2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CollisionPermut (gen, res, 20, 20 * MILLION, 0, 14);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionPermut, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_CollisionPermut (gen, res, 20, 20 * MILLION, 10, 14);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "CollisionPermut, r = 10");
      }
      sknuth_DeleteRes2 (res);
   }
   {
      sknuth_Res1 *res;
      res = sknuth_CreateRes1 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (gen, res, 40, 10 * MILLION, 0, MILLION / 10, 8);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 8");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (gen, res, 30, 10 * MILLION, 0, MILLION / 10, 16);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 16");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (gen, res, 20, 10 * MILLION, 0, MILLION / 10, 24);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 24");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 24");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sknuth_MaxOft (gen, res, 20, 10 * MILLION, 0, MILLION / 10, 32);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft, t = 32");
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MaxOft AD, t = 32");
      }
      sknuth_DeleteRes1 (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleProd (gen, res, 40, 10 * MILLION, 0, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleProd, t = 8");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleProd (gen, res, 20, 10*MILLION, 0, 16);
         bbattery_pVal[++j] = res->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleProd, t = 16");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleProd (gen, res, 20, 10*MILLION, 0, 24);
         bbattery_pVal[++j] = res->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleProd, t = 24");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleMean (gen, res, 20*MILLION, 30, 0);
         bbattery_pVal[++j] = res->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleMean, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleMean (gen, res, 20*MILLION, 30, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleMean, r = 10");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleCorr (gen, res, 1, 2*BILLION, 0, 1);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleCorr, k = 1");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SampleCorr (gen, res, 1, 2*BILLION, 0, 2);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SampleCorr, k = 2");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_AppearanceSpacings (gen, res, 1, 10 * MILLION, BILLION,
            r, 3, 15);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AppearanceSpacings, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_AppearanceSpacings (gen, res, 1, 10 * MILLION, BILLION,
            27, 3, 15);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AppearanceSpacings, r = 27");
      }
      sres_DeleteBasic (res);
   }
   {
      smarsa_Res2 *res2;
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (gen, res, 1, 20 * MILLION, 0, 256, 0.0, 0.25);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (gen, res, 1, 20 * MILLION, 20, 256, 0.0, 0.25);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 20");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (gen, res, 1, 20 * MILLION, 28, 256, 0.0, 0.25);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 28");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (gen, res, 1, 20 * MILLION, 0, 256, 0.0, 0.0625);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (gen, res, 1, 20 * MILLION, 10, 256, 0.0, 0.0625);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 10");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_WeightDistrib (gen, res, 1, 20 * MILLION, 26, 256, 0.0, 0.0625);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "WeightDistrib, r = 26");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         svaria_SumCollector (gen, res, 1, 500 * MILLION, 0, 10.0);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "SumCollector");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (gen, res, 10, MILLION, r, 5, 30, 30);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, L=30, r=0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (gen, res, 10, MILLION, 25, 5, 30, 30);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, L=30, r=26");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (gen, res, 1, 5 * THOUSAND, r, 4, 1000, 1000);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, L=1000, r=0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (gen, res, 1, 5 * THOUSAND, 26, 4, 1000, 1000);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, L=1000, r=26");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (gen, res, 1, 80, 15, 15, 5000, 5000);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, L=5000");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_MatrixRank (gen, res, 1, 80, 0, 30, 5000, 5000);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank, L=5000");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_Savir2 (gen, res, 10, 10 * MILLION, 10, 1024*1024, 30);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Savir2");
      }
      sres_DeleteChi2 (res);

      res2 = smarsa_CreateRes2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smarsa_GCD (gen, res2, 10, 50 * MILLION, 0, 30);
         bbattery_pVal[++j] = res2->GCD->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "GCD");
      }
      smarsa_DeleteRes2 (res2);
   }
   {
      swalk_Res *res;
      res = swalk_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (gen, res, 1, 100 * MILLION, r, 5, 50, 50);
         GetPVal_Walk (1, res, &j, " (L=50, r=0)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (gen, res, 1, 100 * MILLION, 25, 5, 50, 50);
         GetPVal_Walk (1, res, &j, " (L=50, r=25)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (gen, res, 1, 10 * MILLION, r, 10, 1000, 1000);
         GetPVal_Walk (1, res, &j, " (L=1000, r=0)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (gen, res, 1, 10 * MILLION, 20, 10, 1000, 1000);
         GetPVal_Walk (1, res, &j, " (L=1000, r=20)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (gen, res, 1, 1 * MILLION, r, 15, 10000, 10000);
         GetPVal_Walk (1, res, &j, " (L=10000, r=0)", j2, globals);
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         swalk_RandomWalk1 (gen, res, 1, 1 * MILLION, 15, 15, 10000, 10000);
         GetPVal_Walk (1, res, &j, " (L=10000, r=15)", j2, globals);
      }
      swalk_DeleteRes (res);
   }
   {
      scomp_Res *res;
      res = scomp_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LinearComp (gen, res, 1, 400 * THOUSAND + 20, r, 1);
         bbattery_pVal[++j] = res->JumpNum->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 0");
         bbattery_pVal[++j] = res->JumpSize->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LinearComp (gen, res, 1, 400 * THOUSAND + 20, 29, 1);
         bbattery_pVal[++j] = res->JumpNum->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 29");
         bbattery_pVal[++j] = res->JumpSize->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp, r = 0");
      }
      scomp_DeleteRes (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LempelZiv (gen, res, 10, 27, r, s);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LempelZiv, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LempelZiv (gen, res, 10, 27, 15, 15);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LempelZiv, r = 15");
      }
      sres_DeleteBasic (res);
   }
   {
      sspectral_Res *res;
      res = sspectral_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier3 (gen, res, 100 * THOUSAND, 14, r, 3);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier3, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier3 (gen, res, 100 * THOUSAND, 14, 27, 3);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier3, r = 27");
      }
      sspectral_DeleteRes (res);
   }
   {
      sstring_Res2 *res;
      res = sstring_CreateRes2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_LongestHeadRun (gen, res, 1, 1000, r, 3, 20 + 10 * MILLION);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 0");
         bbattery_pVal[++j] = res->Disc->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_LongestHeadRun (gen, res, 1, 1000, 27, 3, 20 + 10 * MILLION);
         bbattery_pVal[++j] = res->Chi->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 27");
         bbattery_pVal[++j] = res->Disc->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LongestHeadRun, r = 27");
      }
      sstring_DeleteRes2 (res);
   }
   {
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_PeriodsInStrings (gen, res, 10, BILLION/2, r, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "PeriodsInStrings, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_PeriodsInStrings (gen, res, 10, BILLION/2, 20, 10);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "PeriodsInStrings, r = 20");
      }
      sres_DeleteChi2 (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingWeight2 (gen, res, 10, BILLION, r, 3, MILLION);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingWeight2, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingWeight2 (gen, res, 10, BILLION, 27, 3, MILLION);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingWeight2, r = 27");
      }
      sres_DeleteBasic (res);
   }
   {
      sstring_Res *res;
      res = sstring_CreateRes ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingCorr (gen, res, 1, BILLION, 10, 10, s);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingCorr, L = 30");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingCorr (gen, res, 1, 100 * MILLION, 10, 10, 10 * s);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingCorr, L = 300");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingCorr (gen, res, 1, 100 * MILLION, 10, 10, 40 * s);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingCorr, L = 1200");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (gen, res, 10, 30 * MILLION, r, 3, s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L=30, r=0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (gen, res, 10, 30 * MILLION, 27, 3, s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L=30, r=27");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (gen, res, 1, 30 * MILLION, r, 4, 10 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L=300, r=0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (gen, res, 1, 30 * MILLION, 26, 4, 10 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L=300, r=26");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (gen, res, 1, 10 * MILLION, r, 5, 40 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L=1200, r=0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingIndep (gen, res, 1, 10 * MILLION, 25, 5, 40 * s, 0);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingIndep, L=1200, r=25");
      }
      sstring_DeleteRes (res);
   }
   {
      sstring_Res3 *res;
      res = sstring_CreateRes3 ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_Run (gen, res, 1, 2*BILLION, r, 3);
         bbattery_pVal[++j] = res->NRuns->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 0");
         bbattery_pVal[++j] = res->NBits->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_Run (gen, res, 1, 2*BILLION, 27, 3);
         bbattery_pVal[++j] = res->NRuns->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 27");
         bbattery_pVal[++j] = res->NBits->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits, r = 27");
      }
      sstring_DeleteRes3 (res);
   }
   {
      sres_Basic *res;
      res = sres_CreateBasic ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, 10, 30 + BILLION, r, 3, 1);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d=1, r=0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, 10, 30 + BILLION, r, 3, 3);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d=3, r=0");
      }

      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, 10, 30 + BILLION, 27, 3, 1);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d=1, r=27");
      }

      ++j2;
      util_Assert (j2 <= BIGCRUSH_NUM, "BigCrush:   j2 > BIGCRUSH_NUM");
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, 10, 30 + BILLION, 27, 3, 3);
         bbattery_pVal[++j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor, d=3, r=27");
      }
      sres_DeleteBasic (res);
   }

   bbattery_NTests = ++j;
   GetName (gen, genName);
   if(swrite_Basic){
      WriteReport (genName, "BigCrush", bbattery_NTests, bbattery_pVal,
         Timer, FALSE, TRUE, 0.0, globals);
   }
   chrono_Delete (Timer);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_BigCrush (unif01_Gen * gen)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= BIGCRUSH_NUM; ++i)
      Rep[i] = 1;
   BigCrush (gen, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_RepeatBigCrush (unif01_Gen * gen, int Rep[])
{
  TestGlobals globals;
  BigCrush (gen, Rep, globals);
  return makeResults(globals);
}


/*=========================================================================*/
#if 0
static void WriteTime (time_t t0, time_t t1)
{
   int y1;
   double y = 0;

   y = difftime (t1, t0);
   /* printf (" Total time: %.2f sec\n\n", y); */
   printf (" Total time: ");
   y1 = y / 3600;
   printf ("%02d:", y1);
   y -= y1 * 3600.0;
   y1 = y / 60;
   printf ("%02d:", y1);
   y -= y1 * 60.0;
   printf ("%.2f\n\n", y);
}
#endif

/*-------------------------------------------------------------------------*/

static void Alphabit (unif01_Gen * gen, char *fname, double nb, int r, int s,
		      lebool blocFlag, int w, int Rep[], TestGlobals &globals)
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
   chrono_Chrono *Timer;
 /*  time_t t0, t1; */
   int NbDelta = 1;
   double ValDelta[] = { 1 };
   long N = 1;
   long n, L;
   int j = 0;
   int j2 = 0;
   int i;
   lebool fileFlag;
   long bufsiz;
   char genName[LEN + 1] = "";
   double z;
   unif01_Gen *gen0;

   Timer = chrono_Create ();
 /*  t0 = time (NULL); */
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "          Starting Alphabit:   nb = %.0f\n"
         "          Version: %s\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n",
         nb, PACKAGE_STRING);
   }
   util_Assert (nb > 0, "Alphabit:   nb <= 0");
   /* Bits will be read as 32-bit unsigned integers */
   nb -= fmod (nb, 32.0);
   bufsiz = nb / 32.0;

   if (blocFlag) {
      gen0 = ufile_CreateReadBin (fname, bufsiz);
      gen = unif01_CreateBitBlockGen (gen0, r, s, w);
      nb -= fmod (nb, 1024.0 / w);
      fileFlag = TRUE;
   } else if (NULL == gen) {
      gen = ufile_CreateReadBin (fname, bufsiz);
      fileFlag = TRUE;
   } else {
      fileFlag = FALSE;
   }

   {
      smultin_Param *par = NULL;
      smultin_Res *res;
      par = smultin_CreateParam (NbDelta, ValDelta, smultin_GenerCellSerial,
         3);
      res = smultin_CreateRes (par);
      if (fileFlag)
         ufile_InitReadBin ();

      if (nb > BILLION)
         N = 1 + nb / BILLION;
      else
         N = 1;
      n = nb / N;
      /* Set n as a multiple of s = 32 */
      n -= n % 32;
      j = -1;
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smultin_MultinomialBitsOver (gen, par, res, N, n, r, s, 2, FALSE);
         strcpy (bbattery_TestNames[++j], "MultinomialBitsOver, L = 2");
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[0][gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[0][gofw_AD];
         TestNumber[j] = j2;
      }

      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         smultin_MultinomialBitsOver (gen, par, res, N, n, r, s, 4, FALSE);
         strcpy (bbattery_TestNames[++j], "MultinomialBitsOver, L = 4");
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[0][gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[0][gofw_AD];
         TestNumber[j] = j2;
      }

      ++j2;
      if (n > 250) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            smultin_MultinomialBitsOver (gen, par, res, N, n, r, s, 8, FALSE);
            strcpy (bbattery_TestNames[++j], "MultinomialBitsOver, L = 8");
            if (N == 1)
               bbattery_pVal[j] = res->pVal2[0][gofw_Mean];
            else
               bbattery_pVal[j] = res->pVal2[0][gofw_AD];
           TestNumber[j] = j2;
         }
      }

      ++j2;
      if (n > 65000) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            smultin_MultinomialBitsOver (gen, par, res, N, n, r, s, 16, FALSE);
            strcpy (bbattery_TestNames[++j], "MultinomialBitsOver, L = 16");
            if (N == 1)
               bbattery_pVal[j] = res->pVal2[0][gofw_Mean];
            else
               bbattery_pVal[j] = res->pVal2[0][gofw_AD];
           TestNumber[j] = j2;
         }
      }

      smultin_DeleteRes (res);
      smultin_DeleteParam (par);
   }

   {
      sstring_Res *res;
      res = sstring_CreateRes ();

      if (fileFlag)
         ufile_InitReadBin ();
      z = nb / s;
      N = 1 + z / BILLION;
      n = z / N;
      ++j2;
      if (n >= 20) {
         for (i = 0; i < Rep[j2]; ++i) {
            sstring_HammingIndep (gen, res, N, n, r, s, 16, 0);
            j++;
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 16");
            TestNumber[j] = j2;
         }
      }

      if (fileFlag)
         ufile_InitReadBin ();
      n /= 2;
      ++j2;
      if (n >= 20) {
         for (i = 0; i < Rep[j2]; ++i) {
            sstring_HammingIndep (gen, res, N, n, r, s, 32, 0);
            j++;
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 32");
            TestNumber[j] = j2;
         }
      }

      if (fileFlag)
         ufile_InitReadBin ();
      n *= 2;
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_HammingCorr (gen, res, N, n, r, s, 32);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "HammingCorr, L = 32");
      }
   }

   {
      swalk_Res *res;
      res = swalk_CreateRes ();

      if (fileFlag)
         ufile_InitReadBin ();
      L = 64;
      z = nb / L;
      N = 1 + z / BILLION;
      n = z / N;
      j2++;
      if (n >= 30) {
         for (i = 0; i < Rep[j2]; ++i) {
            swalk_RandomWalk1 (gen, res, N, n, r, s, L, L);
            GetPVal_Walk (N, res, &j, " (L = 64)", j2, globals);
         }
      }

      if (fileFlag)
         ufile_InitReadBin ();
      L = 320;
      z = nb / L;
      N = 1 + z / BILLION;
      n = z / N;
      j2++;
      util_Assert (j2 <= ALPHABIT_NUM, "Alphabit:   j2 > ALPHABIT_NUM");
      if (n >= 30) {
         for (i = 0; i < Rep[j2]; ++i) {
            swalk_RandomWalk1 (gen, res, N, n, r, s, L, L);
            GetPVal_Walk (N, res, &j, " (L = 320)", j2, globals);
         }
      }
      swalk_DeleteRes (res);
   }

   bbattery_NTests = ++j;
   if (blocFlag) {
      unif01_DeleteBitBlockGen (gen);
      gen = gen0;
   }
   if (fileFlag) {
      WriteReport (fname, "Alphabit", bbattery_NTests,
		   bbattery_pVal, Timer, TRUE, TRUE, nb, globals);
      ufile_DeleteReadBin (gen);
   } else {
      GetName (gen, genName);
      WriteReport (genName, "Alphabit", bbattery_NTests, bbattery_pVal,
		   Timer, FALSE, TRUE, nb, globals);
   }

   chrono_Delete (Timer);
  /*  t1 = time (NULL);
    WriteTime (t0, t1); */
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_Alphabit (unif01_Gen * gen, double nb, int r, int s)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= ALPHABIT_NUM; ++i)
      Rep[i] = 1;
   Alphabit (gen, NULL, nb, r, s, FALSE, 0, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_AlphabitFile (char *filename, double nb)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= ALPHABIT_NUM; ++i)
      Rep[i] = 1;
   Alphabit (NULL, filename, nb, 0, 32, FALSE, 0, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_RepeatAlphabit (unif01_Gen * gen, double nb, int r, int s,
   int Rep[])
{
  TestGlobals globals;
  Alphabit (gen, NULL, nb, r, s, FALSE, 0, Rep, globals);
  return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_BlockAlphabit (unif01_Gen * gen, double n, int r, int s)
{
  TestGlobals globals;
  
   unif01_Gen *gen2;
   int L = 1;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= ALPHABIT_NUM; ++i)
      Rep[i] = 1;
   while ((L <= 32) && (L <= s)) {
      gen2 = unif01_CreateBitBlockGen (gen, r, s, L);
      Alphabit (gen2, NULL, n, r, s, FALSE, 0, Rep, globals);
      unif01_DeleteBitBlockGen (gen2);
      L *= 2;
   }
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_RepeatBlockAlphabit (unif01_Gen * gen, double nb, int r, int s,
   int Rep[], int L)
{
  TestGlobals globals;
  
   if ((L <= 32) && (L <= s)) {
      unif01_Gen *gen2;
      gen2 = unif01_CreateBitBlockGen (gen, r, s, L);
      Alphabit (gen2, NULL, nb, r, s, FALSE, 0, Rep, globals);
      unif01_DeleteBitBlockGen (gen2);
   }
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_BlockAlphabitFile (char *filename, double nb)
{
  TestGlobals globals;
  
   int w = 1;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= ALPHABIT_NUM; ++i)
      Rep[i] = 1;
   while (w <= 32) {
     Alphabit (NULL, filename, nb, 0, 32, TRUE, w, Rep, globals);
      w *= 2;
   }
   return makeResults(globals);
}


/*=========================================================================*/

static void DoMultinom (lebool fileFlag, /* */
   unif01_Gen * gen,              /* */
   double nb,                     /* Number of bits */
   int *pj,                       /* j */
   int j2,                        /* Test number in the battery */
   int Rep[],                      /* Number of replications */
   TestGlobals &globals
   )
/*
 * Do the smultin_MultinomialBits in Rabbit
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
   const long NLIM = 10000000;
   long n, N;
   int L, t;
   double x;
   int i;
   int j = *pj;
   smultin_Res *res;
   smultin_Param *par = NULL;
   double ValDelta[] = { -1 };

   util_Assert (nb > 0.0, "MultinomialBits:   nb <= 0");
   par = smultin_CreateParam (1, ValDelta, smultin_GenerCellSerial, -3);
   res = smultin_CreateRes (par);
   if (fileFlag)
      ufile_InitReadBin ();

   /* Limit sample size n to NLIM because of memory limitations. */
   /* Determine number of replications N from this. */
   N = 1 + nb / NLIM;
   n = nb / N;
   /* Time limit on test: N = 30 */
   N = util_Min (30, N);
   /* Set n as a multiple of s = 32 */
   n -= n % 32;
   L = num_Log2 (n / 200.0 * n);
   L = util_Max (4, L);
   for (i = 0; i < Rep[j2]; ++i) {
      smultin_MultinomialBitsOver (gen, par, res, N, n, 0, 32, L, TRUE);
      strcpy (bbattery_TestNames[++j], "MultinomialBitsOver");
      bbattery_pVal[j] = res->pColl;
      TestNumber[j] = j2;
   }

   *pj = j;
   smultin_DeleteRes (res);
   smultin_DeleteParam (par);
}

static void DoMultinomPar (lebool fileFlag, /* */
   unif01_Gen * gen,              /* */
   double nb,                     /* Number of bits */
   int *pj,                       /* j */
   int j2,                        /* Test number in the battery */
   int Rep[],                      /* Number of replications */
   TestGlobals &globals
   )
/*
 * Do the smultin_MultinomialBits in Rabbit
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
   const long NLIM = 10000000;
   long n, N;
   int L, t;
   double x;
   int i;
   int j = *pj;
   smultin_Res *res;
   smultin_Param *par = NULL;
   double ValDelta[] = { -1 };

   util_Assert (nb > 0.0, "MultinomialBits:   nb <= 0");
   par = smultin_CreateParam (1, ValDelta, smultin_GenerCellSerial, -3);
   res = smultin_CreateRes (par);
   if (fileFlag)
      ufile_InitReadBin ();

   /* Limit sample size n to NLIM because of memory limitations. */
   /* Determine number of replications N from this. */
   N = 1 + nb / NLIM;
   n = nb / N;
   /* Time limit on test: N = 30 */
   N = util_Min (30, N);
   /* Set n as a multiple of s = 32 */
   n -= n % 32;
   L = num_Log2 (n / 200.0 * n);
   L = util_Max (4, L);
   for (i = 0; i < Rep[j2]; ++i) {
      smultin_MultinomialBitsOverPar (gen, par, res, N, n, 0, 32, L, TRUE);
      strcpy (bbattery_TestNames[++j], "MultinomialBitsOver");
      bbattery_pVal[j] = res->pColl;
      TestNumber[j] = j2;
   }

   *pj = j;
   smultin_DeleteRes (res);
   smultin_DeleteParam (par);
}


/*-------------------------------------------------------------------------*/

static void DoAppear (lebool fileFlag, /* */
   unif01_Gen * gen, double nb,   /* Number of bits to test */
   int *pj,                       /* j */
   int j2,                        /* Test number in the battery */
   int Rep[],
   TestGlobals &globals
   )
/*
 * Do the svaria_AppearanceSpacings test in Rabbit
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
   sres_Basic *res;
   const long NLIM = 2000000000;
   int L;
   long N, Q;
   int i;
   int j = *pj;
   double temp = nb * (30.0 / 32.0) / 20.0;

   res = sres_CreateBasic ();
   if (num_TwoExp[30] < temp / 30.0)
      L = 30;
   else if (num_TwoExp[15] < temp / 15.0)
      L = 15;
   else if (num_TwoExp[10] < temp / 10.0)
      L = 10;
   else if (num_TwoExp[6] < temp / 6.0)
      L = 6;
   else if (num_TwoExp[5] < temp / 5.0)
      L = 5;
   else if (num_TwoExp[3] < temp / 3.0)
      L = 3;
   else
      L = 2;
   temp = nb / 2;
   temp *= 30.0 / 32.0;
   temp /= L;
   N = 1 + temp / NLIM;
   Q = temp / N;
   N = 1;

   if (Q < 50)
      return;
   if (fileFlag)
      ufile_InitReadBin ();
   for (i = 0; i < Rep[j2]; ++i) {
      svaria_AppearanceSpacings (gen, res, N, Q, Q, 0, 30, L);
      j++;
      if (N == 1)
         bbattery_pVal[j] = res->pVal2[gofw_Mean];
      else
         bbattery_pVal[j] = res->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "AppearanceSpacings");
   }
   sres_DeleteBasic (res);
   *pj = j;
}


/*-------------------------------------------------------------------------*/

static void DoWalk (lebool fileFlag, /* */
   unif01_Gen * gen_base,              /* */
   double nb,                     /* Number of bits to test */
   int *pj,                       /* j */
   int j2,                        /* Test number in the battery */
   int Rep[],
   TestGlobals &globals
   )
/*
 * Do 3 swalk_RandomWalk1 tests in Rabbit
 */
{
   swalk_Res *res;
   long n, N, L;
   double z;
   int i;

   L = 128;
   z = nb / L;
   N = 1 + z / BILLION;
   n = z / N;
   N = 1;

   while (n < 100) {
      L /= 2;
      n *= 2;
   }
   if (L < 4)
      return;
   n = nb / (L * N);
   n = util_Min (n, 500 * MILLION);
   if (L < 32) {
      while (32 * n > nb)
         n--;
   }
   if (n < 30)
      return;

   unif01_Gen * gen = workload_Clone(gen_base);
   res = swalk_CreateRes ();
   ++j2;
   if (fileFlag)
      ufile_InitReadBin ();
   for (i = 0; i < Rep[j2]; ++i) {
      swalk_RandomWalk1 (gen, res, N, n, 0, 32, L, L);
      GetPVal_Walk (N, res, pj, "", j2, globals);
   }
   if (L < 96)
      return;

   L = 1024;
   z = nb / L;
   N = 1 + z / BILLION;
   n = z / N;
   n = util_Min (n, 50 * MILLION);
   N = 1;
   while ((double) n * L > nb)
      n--;
   if (n < 30)
      return;

   ++j2;
   gen = workload_Clone(gen_base);
   if (fileFlag)
      ufile_InitReadBin ();
   for (i = 0; i < Rep[j2]; ++i) {
      swalk_RandomWalk1 (gen, res, N, n, 0, 32, L, L);
      GetPVal_Walk (N, res, pj, " (L = 1024)", j2, globals);
   }

   L = 10016;
   z = nb / L;
   N = 1 + z / BILLION;
   n = z / N;
   n = util_Min (n, 5 * MILLION);
   N = 1;
   while ((double) n * L > nb)
      n--;
   if (n < 30)
      return;
   ++j2;
   gen = workload_Clone(gen_base);
   if (fileFlag)
      ufile_InitReadBin ();
   for (i = 0; i < Rep[j2]; ++i) {
      swalk_RandomWalk1 (gen, res, N, n, 0, 32, L, L);
      GetPVal_Walk (N, res, pj, " (L = 10016)", j2, globals);
   }

   swalk_DeleteRes (res);
}


static void Rabbit (unif01_Gen * gen, char *fname, double nb, int Rep[], TestGlobals &globals)
/*
 * A battery of statistical tests for a file of n random bits.
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
   const int s = 32;
   int k, j = 0, j2 = 0;
   int i;
   long n, N, L;
   double nw, x;
   chrono_Chrono *Timer;
   long bufsiz;
   lebool fileFlag;
   char genName[LEN + 1] = "";

   unif01_Gen * gen_base = gen;

   Timer = chrono_Create ();
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "          Starting Rabbit:   nb = %.0f\n"
         "          Version: %s\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n",
         nb, PACKAGE_STRING);
   }
   util_Assert (nb >= 500.0, "bbattery_Rabbit:   nb < 500");

   /* Bits will be read as 32-bit unsigned integers */
   nb -= fmod (nb, 32.0);
   nw = nb / 32.0;
   bufsiz = nw;

   if (NULL == gen) {
      gen = ufile_CreateReadBin (fname, bufsiz);
      fileFlag = TRUE;
   } else
      fileFlag = FALSE;

   j = -1;
   ++j2;
   DoMultinom (fileFlag, workload_Clone(gen_base), nb, &j, j2, Rep, globals);

   {
      unif01_Gen * gen = workload_Clone(gen_base);
      const long NLIM = 4000000;
      snpair_Res *res;
      res = snpair_CreateRes ();
      N = 1 + nw / NLIM;
      n = nw / N;
      N = util_Min (N, 25);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairsBitMatch (gen, res, N, n / 2, 0, 2);
         bbattery_pVal[++j] = res->pVal[snpair_BM];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 2");
      }

      gen = workload_Clone(gen_base);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairsBitMatch (gen, res, N, n / 4, 0, 4);
         bbattery_pVal[++j] = res->pVal[snpair_BM];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 4");
      }
      snpair_DeleteRes (res);
   }

   ++j2;
   DoAppear (fileFlag, workload_Clone(gen), nb, &j, j2, Rep, globals);

   {
     unif01_Gen * gen = workload_Clone(gen_base);
      const long NLIM1 = 300000;
      const long NLIM2 = 10000;
      scomp_Res *res;
      res = scomp_CreateRes ();
      n = NLIM2 + 2.0 * sqrt (nb);
      n = util_Min (n, nb);
      n = util_Min (n, NLIM1);
      N = 1;
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LinearComp (gen, res, N, n, 0, s);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->JumpSize->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->JumpSize->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp");
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->JumpNum->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->JumpNum->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp");
      }
      scomp_DeleteRes (res);
   }

   k = num_Log2 (nb + 0.5);
   if (k > 28)
      k = 28;
   N = 1;
   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sres_Basic *res;
      res = sres_CreateBasic ();
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         scomp_LempelZiv (gen, res, N, k, 0, s);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LempelZiv");
      }
      sres_DeleteBasic (res);
   }
   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sspectral_Res *res;
      k = num_Log2 (nb + 0.5);
      k = util_Min (20, k);
      res = sspectral_CreateRes ();
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier1 (gen, res, 1, k, 0, s);
         j++;
         bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier1");
      }
      gen = workload_Clone(gen_base);
      x = sqrt (2.0 * nb);
      N = x / 2.0;
      if (N < 32) {
         k = 5;
         N = nb / 32.0;
      } else if (N >= 16384) {
         k = 14;
         N = nb / 16384.0;
      } else {
         k = num_Log2 (x / 2.0 + 0.5);
         N = nb / (num_TwoExp[k]);
      }
      N = util_Min (N, 300000);
      while ((num_TwoExp[k] + 32) * N > nb)
         N--;
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier3 (gen, res, N, k, 0, s);
         j++;
         bbattery_pVal[j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier3");
      }
      sspectral_DeleteRes (res);
   }
   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sstring_Res2 *res;
      res = sstring_CreateRes2 ();
      x = util_Min (BILLION * 100.0, nb);
      n = 600;
      L = x / n;
      if (L <= 100000) {
         n /= 10;
         L *= 10;
      }
      if (L <= 10000) {
         n /= 2;
         L *= 2;
      }
      ++j2;
      if ((L >= 1032) && (n >= 30)) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            sstring_LongestHeadRun (gen, res, 1, n, 0, s, L);
            j++;
            bbattery_pVal[j] = res->Chi->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "LongestHeadRun");
         }
      }
      sstring_DeleteRes2 (res);
   }
   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      nw = nb / 32.0;
      nw = util_Min (nw, 4.0 * BILLION);
      N = 1 + nw / BILLION;
      n = nw / N;
      ++j2;
      if (n >= 30) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            sstring_PeriodsInStrings (gen, res, N, n, 0, 31);
            ++j;
            if (N == 1)
               bbattery_pVal[j] = res->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "PeriodsInStrings");
         }
      }

      nw = nb / s;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (10, N);
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            sstring_HammingWeight (gen, res, N, n, 0, s, s);
            ++j;
            if (N == 1)
               bbattery_pVal[j] = res->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingWeight");
         }
      }
      sres_DeleteChi2 (res);
   }
   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sstring_Res *res;
      res = sstring_CreateRes ();
      nw = nb / s;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (10, N);
      ++j2;
      if (n > 2) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingCorr (gen, res, N, n, 0, s, 32);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingCorr, L = 32");
         }
      }

      nw = nb / 64;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 2) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingCorr (gen, res, N, n, 0, s, 2 * s);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingCorr, L = 64");
         }
      }

      nw = nb / (4 * s);
      N = 1 + nw / BILLION * 4;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 2) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            sstring_HammingCorr (gen, res, N, n, 0, s, 4 * s);
            j++;
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingCorr, L = 128");
         }
      }

      nw = nb / s;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (5, N);
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingIndep (gen, res, N, n, 0, s, 16, 0);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 16");
         }
      }

      nw = nb / (2 * s);
      N = 1 + nw / BILLION * 2;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingIndep (gen, res, N, n, 0, s, s, 0);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 32");
         }
      }

      nw = nb / (4 * s);
      N = 1 + nw / BILLION * 10;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingIndep (gen, res, N, n, 0, s, 2 * s, 0);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 64");
            TestNumber[j] = j2;
         }
      }
      sstring_DeleteRes (res);
   }
   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sres_Basic *res;
      int d;
      res = sres_CreateBasic ();

      d = 1;
      N = 1 + nb / BILLION;
      n = nb / N - d;
      n -= n % 32;
      N = util_Min (100, N);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, N, n, 0, s, d);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor");
      }

      d = 2;
      N = 1 + nb / BILLION;
      n = nb / N - d;
      n -= n % 32;
      N = util_Min (100, N);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, N, n, 0, s, d);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor");
      }

      sres_DeleteBasic (res);
   }
   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sstring_Res3 *res;
      res = sstring_CreateRes3 ();
      nw = nb / 5;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (20, N);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (i = 0; i < Rep[j2]; ++i) {
         sstring_Run (gen, res, N, n, 0, s);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->NRuns->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->NRuns->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits");
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->NBits->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->NBits->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits");
       }
      sstring_DeleteRes3 (res);
   }

   {
     unif01_Gen * gen = workload_Clone(gen_base);
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      n = nb / (s * s);
      n = util_Min (n, 50 * MILLION);
      ++j2;
      if (n >= 50) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            smarsa_MatrixRank (gen, res, 1, n, 0, s, s, s);
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "MatrixRank, 32 x 32");
         }
      }

      gen = workload_Clone(gen_base);
      n = nb / (100.0 * s * s);
      n = util_Min (n, 300000);
      ++j2;
      if (n >= 50) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            smarsa_MatrixRank (gen, res, 1, n, 0, s, 10 * s, 10 * s);
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "MatrixRank, 320 x 320");
         }
      }

      gen = workload_Clone(gen_base);
      n = nb / (1024.0 * s * s);
      n = util_Min (n, 20000);
      ++j2;
      if (n >= 50) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (i = 0; i < Rep[j2]; ++i) {
            j++;
            smarsa_MatrixRank (gen, res, 1, n, 0, s, 32 * s, 32 * s);
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "MatrixRank, 1024 x 1024");
         }
      }
      sres_DeleteChi2 (res);
   }

   DoWalk (fileFlag, workload_Clone(gen), nb, &j, j2, Rep, globals);
   util_Assert (j2 <= RABBIT_NUM, "Rabbit:   j2 > RABBIT_NUM");

   bbattery_NTests = ++j;
   if (fileFlag) {
      if(swrite_Basic){
         WriteReport (fname, "Rabbit", bbattery_NTests,
              bbattery_pVal, Timer, TRUE, TRUE, nb, globals);
      }
      ufile_DeleteReadBin (gen);
   } else {
      GetName (gen, genName);
      if(swrite_Basic){
         WriteReport (genName, "Rabbit", bbattery_NTests, bbattery_pVal,
              Timer, FALSE, TRUE, nb, globals);
      }
   }
   chrono_Delete (Timer);
}


/*-------------------------------------------------------------------------*/

static void RabbitPar (unif01_Gen * gen, char *fname, double nb, int Rep[], TestGlobals &globals)
/*
 * A battery of statistical tests for a file of n random bits.
 */
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;

  for(int i = 0; i < RABBIT_NUM; i++){
    bbattery_pVal[i] = -1;
  }
  
   const int s = 32;
  //  int k, j = 0, j2 = 0;nw
  //  int i;
   
  //  double nw, x;
   chrono_Chrono *Timer;
   long bufsiz;
   lebool fileFlag;
   char genName[LEN + 1] = "";

   Timer = chrono_Create ();
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "          Starting Rabbit:   nb = %.0f\n"
         "          Version: %s\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n",
         nb, PACKAGE_STRING);
   }
   util_Assert (nb >= 500.0, "bbattery_Rabbit:   nb < 500");

   /* Bits will be read as 32-bit unsigned integers */
   nb -= fmod (nb, 32.0);
   
  //  bufsiz = nw;

   if (NULL == gen) {
      gen = ufile_CreateReadBin (fname, bufsiz);
      fileFlag = TRUE;
   } else
      fileFlag = FALSE;
 
   std::vector<std::function<void(unif01_Gen*, int, int)> > Functions;
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      ++j2;
      DoMultinomPar (fileFlag, gen, nb, &j, j2, Rep, globals);
   });

   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
     long n, N, L;
      const long NLIM = 4000000;
      snpair_Res *res;
      res = snpair_CreateRes ();
      int nw = nb / 32.0;
      N = 1 + nw / NLIM;
      n = nw / N;
      N = util_Min (N, 25);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairsBitMatch (gen, res, N, n / 2, 0, 2);
         bbattery_pVal[++j] = res->pVal[snpair_BM];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 2");
      }
      snpair_DeleteRes (res);
   });

    Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
     long n, N, L;
      const long NLIM = 4000000;
      snpair_Res *res;
      res = snpair_CreateRes ();
      int nw = nb / 32.0;
      N = 1 + nw / NLIM;
      n = nw / N;
      N = util_Min (N, 25);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         snpair_ClosePairsBitMatch (gen, res, N, n / 4, 0, 4);
         bbattery_pVal[++j] = res->pVal[snpair_BM];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "ClosePairsBitMatch, t = 4");
      }
      snpair_DeleteRes (res);
   });

   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      j2++;
      DoAppear (fileFlag, gen, nb, &j, j2, Rep, globals);
   });

   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      const long NLIM1 = 300000;
      const long NLIM2 = 10000;
      scomp_Res *res;
      res = scomp_CreateRes ();
      n = NLIM2 + 2.0 * sqrt (nb);
      n = util_Min (n, nb);
      n = util_Min (n, NLIM1);
      N = 1;
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         scomp_LinearComp (gen, res, N, n, 0, s);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->JumpSize->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->JumpSize->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp");
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->JumpNum->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->JumpNum->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LinearComp");
      }
      scomp_DeleteRes (res);
   });

   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      int k = num_Log2 (nb + 0.5);
      if (k > 28)
          k = 28;

      long n, N, L;
      N = 1;
      sres_Basic *res;
      res = sres_CreateBasic ();
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         scomp_LempelZiv (gen, res, N, k, 0, s);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "LempelZiv");
      }
      sres_DeleteBasic (res);
   });
   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sspectral_Res *res;
      int k = num_Log2 (nb + 0.5);
      k = util_Min (20, k);
      res = sspectral_CreateRes ();
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier1 (gen, res, 1, k, 0, s);
         j++;
         bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier1");
      }
      sspectral_DeleteRes (res);
   });

    Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sspectral_Res *res;
      int k = num_Log2 (nb + 0.5);
      k = util_Min (20, k);
      res = sspectral_CreateRes ();
      int x = sqrt (2.0 * nb);
      N = x / 2.0;
      if (N < 32) {
         k = 5;
         N = nb / 32.0;
      } else if (N >= 16384) {
         k = 14;
         N = nb / 16384.0;
      } else {
         k = num_Log2 (x / 2.0 + 0.5);
         N = nb / (num_TwoExp[k]);
      }
      N = util_Min (N, 300000);
      while ((num_TwoExp[k] + 32) * N > nb)
         N--;
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         sspectral_Fourier3 (gen, res, N, k, 0, s);
         j++;
         bbattery_pVal[j] = res->Bas->pVal2[gofw_AD];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Fourier3");
      }
      sspectral_DeleteRes (res);
   });

   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sstring_Res2 *res;
      res = sstring_CreateRes2 ();
      int x = util_Min (BILLION * 100.0, nb);
      n = 600;
      L = x / n;
      if (L <= 100000) {
         n /= 10;
         L *= 10;
      }
      if (L <= 10000) {
         n /= 2;
         L *= 2;
      }
      ++j2;
      if ((L >= 1032) && (n >= 30)) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            sstring_LongestHeadRun (gen, res, 1, n, 0, s, L);
            j++;
            bbattery_pVal[j] = res->Chi->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "LongestHeadRun");
         }
      }
      sstring_DeleteRes2 (res);
   });
   
   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      int nw = nb / 32.0;
      nw = util_Min (nw, 4.0 * BILLION);
      N = 1 + nw / BILLION;
      n = nw / N;
      ++j2;
      if (n >= 30) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            sstring_PeriodsInStrings (gen, res, N, n, 0, 31);
            ++j;
            if (N == 1)
               bbattery_pVal[j] = res->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "PeriodsInStrings");
         }
      }

      nw = nb / s;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (10, N);
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            sstring_HammingWeight (gen, res, N, n, 0, s, s);
            ++j;
            if (N == 1)
               bbattery_pVal[j] = res->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingWeight");
         }
      }
      sres_DeleteChi2 (res);
   });

   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sstring_Res *res;
      res = sstring_CreateRes ();
      int nw = nb / s;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (10, N);
      ++j2;
      if (n > 2) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingCorr (gen, res, N, n, 0, s, 32);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingCorr, L = 32");
         }
      }

      nw = nb / 64;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 2) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingCorr (gen, res, N, n, 0, s, 2 * s);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingCorr, L = 64");
         }
      }

      nw = nb / (4 * s);
      N = 1 + nw / BILLION * 4;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 2) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            sstring_HammingCorr (gen, res, N, n, 0, s, 4 * s);
            j++;
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingCorr, L = 128");
         }
      }

      nw = nb / s;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (5, N);
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingIndep (gen, res, N, n, 0, s, 16, 0);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 16");
         }
      }

      nw = nb / (2 * s);
      N = 1 + nw / BILLION * 2;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingIndep (gen, res, N, n, 0, s, s, 0);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 32");
         }
      }

      nw = nb / (4 * s);
      N = 1 + nw / BILLION * 10;
      n = nw / N;
      N = 1;
      ++j2;
      if (n > 29) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            sstring_HammingIndep (gen, res, N, n, 0, s, 2 * s, 0);
            if (N == 1)
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Mean];
            else
               bbattery_pVal[j] = res->Bas->pVal2[gofw_Sum];
            strcpy (bbattery_TestNames[j], "HammingIndep, L = 64");
            TestNumber[j] = j2;
         }
      }
      sstring_DeleteRes (res);
   });
   
   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sres_Basic *res;
      int d;
      res = sres_CreateBasic ();

      d = 1;
      N = 1 + nb / BILLION;
      n = nb / N - d;
      n -= n % 32;
      N = util_Min (100, N);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, N, n, 0, s, d);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor");
      }

      d = 2;
      N = 1 + nb / BILLION;
      n = nb / N - d;
      n -= n % 32;
      N = util_Min (100, N);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         sstring_AutoCor (gen, res, N, n, 0, s, d);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "AutoCor");
      }

      sres_DeleteBasic (res);
   });
   
   
   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sstring_Res3 *res;
      res = sstring_CreateRes3 ();
      int nw = nb / 5;
      N = 1 + nw / BILLION;
      n = nw / N;
      N = util_Min (20, N);
      if (fileFlag)
         ufile_InitReadBin ();
      ++j2;
      for (int i = 0; i < Rep[j2]; ++i) {
         sstring_Run (gen, res, N, n, 0, s);
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->NRuns->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->NRuns->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits");
         j++;
         if (N == 1)
            bbattery_pVal[j] = res->NBits->pVal2[gofw_Mean];
         else
            bbattery_pVal[j] = res->NBits->pVal2[gofw_Sum];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "Run of bits");
       }
      sstring_DeleteRes3 (res);
   });

   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      n = nb / (s * s);
      n = util_Min (n, 50 * MILLION);
      ++j2;
      if (n >= 50) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            smarsa_MatrixRank (gen, res, 1, n, 0, s, s, s);
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "MatrixRank, 32 x 32");
         }
      }
      sres_DeleteChi2 (res);
   });

    Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      n = nb / (100.0 * s * s);
      n = util_Min (n, 300000);
      ++j2;
      if (n >= 50) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            smarsa_MatrixRank (gen, res, 1, n, 0, s, 10 * s, 10 * s);
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "MatrixRank, 320 x 320");
         }
      }
      sres_DeleteChi2 (res);
    });

    Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      long n, N, L;
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      n = nb / (1024.0 * s * s);
      n = util_Min (n, 20000);
      ++j2;
      if (n >= 50) {
         if (fileFlag)
            ufile_InitReadBin ();
         for (int i = 0; i < Rep[j2]; ++i) {
            j++;
            smarsa_MatrixRank (gen, res, 1, n, 0, s, 32 * s, 32 * s);
            bbattery_pVal[j] = res->pVal2[gofw_Mean];
            TestNumber[j] = j2;
            strcpy (bbattery_TestNames[j], "MatrixRank, 1024 x 1024");
         }
      }
      sres_DeleteChi2 (res);
   });
   
   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      // DoWalk (fileFlag, gen, nb, &j, j2, Rep, globals);
      int * pj = &j;

      swalk_Res *res;
      long n, N, L;
      double z;
      int i;

      L = 128;
      z = nb / L;
      N = 1 + z / BILLION;
      n = z / N;
      N = 1;
      while (n < 100) {
          L /= 2;
          n *= 2;
      }
      if (L < 4)
          return; // TODO: shouldn't this return from the whole function?
      n = nb / (L * N);
      n = util_Min (n, 500 * MILLION);
      if (L < 32) {
          while (32 * n > nb)
            n--;
      }
      if (n < 30)
          return; // TODO: shouldn't this return from the whole function?

      res = swalk_CreateRes ();
      ++j2;
      if (fileFlag)
          ufile_InitReadBin ();
      for (int i = 0; i < Rep[j2]; ++i) {
          swalk_RandomWalk1 (gen, res, N, n, 0, 32, L, L);
          GetPVal_Walk (N, res, pj, "", j2, globals);
      }
      if (L < 96) {
          swalk_DeleteRes (res);
          return; // TODO: shouldn't this return from the whole function?
      }
      swalk_DeleteRes (res);
    });

   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      // DoWalk (fileFlag, gen, nb, &j, j2, Rep, globals);
      int * pj = &j;

      swalk_Res *res;
      long n, N, L;
      double z;
      int i;

      res = swalk_CreateRes ();
      L = 1024;
      z = nb / L;
      N = 1 + z / BILLION;
      n = z / N;
      n = util_Min (n, 50 * MILLION);
      N = 1;
      while ((double) n * L > nb)
          n--;
      if (n < 30) {
          swalk_DeleteRes (res);
          return; // TODO: shouldn't this return from the whole function?
      }

      ++j2;
      if (fileFlag)
          ufile_InitReadBin ();
      for (int i = 0; i < Rep[j2]; ++i) {
          swalk_RandomWalk1 (gen, res, N, n, 0, 32, L, L);
          GetPVal_Walk (N, res, pj, " (L = 1024)", j2, globals);
      }
      swalk_DeleteRes (res);
    });

   Functions.push_back([&](unif01_Gen *gen, int j, int j2){ 
      // DoWalk (fileFlag, gen, nb, &j, j2, Rep, globals);
      int * pj = &j;

      swalk_Res *res;
      long n, N, L;
      double z;
      int i;

      res = swalk_CreateRes ();
      L = 10016;
      z = nb / L;
      N = 1 + z / BILLION;
      n = z / N;
      n = util_Min (n, 5 * MILLION);
      N = 1;
      while ((double) n * L > nb)
          n--;
      if (n < 30) {
          swalk_DeleteRes (res);
          return; // TODO: shouldn't this return from the whole function?
      }
      ++j2;
      if (fileFlag)
          ufile_InitReadBin ();
      for (int i = 0; i < Rep[j2]; ++i) {
          swalk_RandomWalk1 (gen, res, N, n, 0, 32, L, L);
          GetPVal_Walk (N, res, pj, " (L = 10016)", j2, globals);
      }

      swalk_DeleteRes (res);
    });

    std::vector<int> j2_increments = {1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 6, 2, 1, 1, 1, 1, 1, 1, 1};
    std::vector<int> j_multipliers = {1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 5, 5, 5};

    int js[j2_increments.size()] = {-1};
    int j2s[j2_increments.size()] = {0};

    for(unsigned i = 1; i <= Functions.size(); i++ ){
      js[i] = js[i - 1] + j_multipliers[i - 1] * j2_increments[i - 1];
      j2s[i] = j2s[i - 1] + j2_increments[i - 1];  
    }
    int j2 = j2s[j2_increments.size() - 1];

    tbb::parallel_for(0u, (unsigned) Functions.size(), [&](unsigned i) {
      int j = js[i];
      int j2 = j2s[i];
      auto begin = std::chrono::system_clock::now();
      unif01_Gen *gen_copy = workload_Clone(gen);
      Functions[i](gen_copy, j, j2);

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> span=end-begin;
      double g=span.count();
      // fprintf(stdout, "%i, -1, TimeUsed, 0, %.16g\n", j2 + 1, g);

      workload_Destroy(gen_copy);
    });

    int j = js[j2_increments.size() - 1];
    int current_index = 0;
    for (int i = 0; i <= j; i++) {
      if (bbattery_pVal[i] != -1) {
         if (current_index != i) {
            bbattery_pVal[current_index] = bbattery_pVal[i];
            TestNumber[current_index] = TestNumber[i];
            strcpy(bbattery_TestNames[current_index], bbattery_TestNames[i]);
         }
         current_index++;
      }
    }
    
    for (int i = current_index; i <= j; i++) {
      bbattery_pVal[i] = -1;
    }

    j = current_index - 1;


    util_Assert (j2 <= RABBIT_NUM, "Rabbit:   j2 > RABBIT_NUM");

    bbattery_NTests = ++j;
    if (fileFlag) {
        if(swrite_Basic){
          WriteReport (fname, "Rabbit", bbattery_NTests,
                bbattery_pVal, Timer, TRUE, TRUE, nb, globals);
        }
        ufile_DeleteReadBin (gen);
    } else {
        GetName (gen, genName);
        if(swrite_Basic){
          WriteReport (genName, "Rabbit", bbattery_NTests, bbattery_pVal,
                Timer, FALSE, TRUE, nb, globals);
        }
    }
    chrono_Delete (Timer);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_Rabbit (unif01_Gen * gen, double nb)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= RABBIT_NUM; ++i)
      Rep[i] = 1;
   Rabbit (gen, NULL, nb, Rep, globals);
   return makeResults(globals);
}

std::vector<bbattery_Result> bbattery_RabbitPar (unif01_Gen * gen, double nb)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= RABBIT_NUM; ++i)
      Rep[i] = 1;
   RabbitPar (gen, NULL, nb, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_RabbitFile (char *filename, double nb)
{
  TestGlobals globals;
   int i;
   int Rep[NDIM + 1] = {0};
   for (i = 1; i <= RABBIT_NUM; ++i)
      Rep[i] = 1;
   Rabbit (NULL, filename, nb, Rep, globals);
   return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_RepeatRabbit (unif01_Gen * gen, double nb, int Rep[])
{
  TestGlobals globals;
  Rabbit (gen, NULL, nb, Rep, globals);
  return makeResults(globals);
}


/*=========================================================================*/

std::vector<bbattery_Result> bbattery_pseudoDIEHARD (unif01_Gen * gen)
/*
 * As close as possible to the DIEHARD test suite.
 */
{
  TestGlobals globals;

  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
   chrono_Chrono *Timer;
   smultin_Param *par = NULL;
   double ValDelta[] = { 1 };
   char genName[LEN + 1] = "";
   int k, i, j = -1;
   int j2 = 0;
   double x;
   long Count[7];
   double NumExp[7] = {
      67.668, 135.335, 135.335, 90.224, 45.112, 18.045, 8.282
   };

   Timer = chrono_Create ();
   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "                 Starting pseudoDIEHARD\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n");
   }
   {
      sres_Poisson *res;
      sres_Chi2 *Chi;
      Chi = sres_CreateChi2 ();
      sres_InitChi2 (Chi, 1, 6, "");
      res = sres_CreatePoisson ();
      printf ("smarsa_BirthdaySpacings test with r = 0, 1, 2, 3, 4, 5,"
         " 6, 7, 8,\n .....\n\n");
      swrite_Basic = FALSE;
      ++j2;
      for (i = 0; i <= 8; i++) {
         printf (" r = %d\n", i);
         for (k = 0; k <= 6; k++)
            Count[k] = 0;
         for (k = 0; k < 500; k++) {
            smarsa_BirthdaySpacings (gen, res, 1, 512, i, 16777216, 1, 1);
            if (res->sVal2 >= 6)
               ++Count[6];
            else
               ++Count[(int) res->sVal2];
         }
         x = gofs_Chi2 (NumExp, Count, 0, 6);
         printf ("ChiSquare statistic                   :");
         bbattery_pVal[++j] = fbar_ChiSquare2 (6, 12, x);
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "BirthdaySpacings");
         gofw_Writep2 (x, bbattery_pVal[j]);
      }
      printf ("\n\n\n\n");
      sres_DeletePoisson (res);
      sres_DeleteChi2 (Chi);
      swrite_Basic = TRUE;
   }
   ++j2;
   {
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      smarsa_MatrixRank (gen, res, 1, 40000, 0, 31, 31, 31);
      bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      TestNumber[j] = ++j2;
      strcpy (bbattery_TestNames[j], "MatrixRank");

      smarsa_MatrixRank (gen, res, 1, 40000, 0, 32, 32, 32);
      bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "MatrixRank");

      for (i = 0; i <= 24; i++) {
         smarsa_MatrixRank (gen, res, 1, 100000, i, 8, 6, 8);
         bbattery_pVal[++j] = res->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "MatrixRank");
      }
      sres_DeleteChi2 (res);
   }
   {
      smultin_Res *res;
      par = smultin_CreateParam (1, ValDelta, smultin_GenerCellSerial, 0);
      res = smultin_CreateRes (par);
      smultin_MultinomialBitsOver (gen, par, res, 20, 2097152, 0, 32, 20,
         TRUE);
      bbattery_pVal[++j] = res->pVal2[0][gofw_AD];
      TestNumber[j] = ++j2;
      strcpy (bbattery_TestNames[j], "MultinomialBitsOver");
      smultin_DeleteRes (res);
      smultin_DeleteParam (par);
   }
   {
      smarsa_Res *res;
      res = smarsa_CreateRes ();
      ++j2;
      for (i = 22; i >= 0; i--) {
         smarsa_Opso (gen, res, 1, i, 1);
         bbattery_pVal[++j] = res->Pois->pVal2;
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "OPSO");
      }
      ValDelta[0] = -1.0;
      ++j2;
      for (i = 27; i >= 0; i--) {
         if (swrite_Basic)
            printf ("***********************************************************\n"
               "Test OQSO calling smarsa_CollisionOver\n\n");
         smarsa_CollisionOver (gen, res, 1, 2097152, i, 32, 4);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "OQSO");
      }
      ++j2;
      for (i = 30; i >= 0; i--) {
         if (swrite_Basic)
            printf ("***********************************************************\n"
               "Test DNA calling smarsa_CollisionOver\n\n");
         smarsa_CollisionOver (gen, res, 1, 2097152, i, 4, 10);
         bbattery_pVal[++j] = res->Bas->pVal2[gofw_Mean];
         TestNumber[j] = j2;
         strcpy (bbattery_TestNames[j], "DNA");
      }
      smarsa_DeleteRes (res);
   }
   j2 += 2;
   {
      snpair_Res *res;
      res = snpair_CreateRes ();
      snpair_ClosePairs (gen, res, 100, 8000, 0, 2, 2, 1);
      bbattery_pVal[++j] = res->pVal[snpair_NP];
      TestNumber[j] = ++j2;
      strcpy (bbattery_TestNames[j], "ClosePairs");

      snpair_ClosePairs (gen, res, 20, 4000, 0, 3, 2, 1);
      bbattery_pVal[++j] = res->pVal[snpair_NP];
      TestNumber[j] = ++j2;
      strcpy (bbattery_TestNames[j], "ClosePairs");
      snpair_DeleteRes (res);
   }
   {
      sres_Chi2 *res;
      res = sres_CreateChi2 ();
      smarsa_Savir2 (gen, res, 1, 100000, 0, 90000, 18);
      bbattery_pVal[++j] = res->pVal2[gofw_Mean];
      TestNumber[j] = ++j2;
      strcpy (bbattery_TestNames[j], "Savir2");

      ++j2;
      sknuth_Run (gen, res, 10, 10000, 0, TRUE);
      bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      TestNumber[j] = ++j2;
      strcpy (bbattery_TestNames[j], "Run of U01");

      sknuth_Run (gen, res, 10, 10000, 0, FALSE);
      bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "Run of U01");

      sknuth_Run (gen, res, 10, 10000, 0, TRUE);
      bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      TestNumber[j] = j2;
      strcpy (bbattery_TestNames[j], "Run of U01");

      sknuth_Run (gen, res, 10, 10000, 0, FALSE);
      bbattery_pVal[++j] = res->pVal2[gofw_Sum];
      strcpy (bbattery_TestNames[j], "Run of U01");
      TestNumber[j] = j2;
      sres_DeleteChi2 (res);
   }

   bbattery_NTests = ++j;
   GetName (gen, genName);
   WriteReport (genName, "pseudoDIEHARD", bbattery_NTests, bbattery_pVal,
		Timer, FALSE, FALSE, 0.0, globals);
   chrono_Delete (Timer);

   return makeResults(globals);
}


/*=========================================================================*/

static double ProbabiliteLHR (long j, double Lnl)
/*
 * Returns the probability that the longest series of successive 1 has
 * length = j.
 */
{
   double x, temp;
   temp = (j + 1) * num_Ln2 - Lnl;
   x = exp (-exp (-temp));
   temp += num_Ln2;
   x = exp (-exp (-temp)) - x;
   return x;
}

/*-------------------------------------------------------------------------*/

static double GetPLongest (int longest)
/*
 * Get the probabilities for the longest run of 1 or 0 over 20000 bits.
 */
{
   double pLeft, pRight;
   double LnLen;
   int j;

   LnLen = log (20000.0);
   pLeft = 0.0;
   for (j = 0; j < longest; j++)
      pLeft += ProbabiliteLHR (j, LnLen);
   pRight = 1.0 - pLeft;
   pLeft += ProbabiliteLHR (longest, LnLen);
   return gofw_pDisc (pLeft, pRight);
}


/*-------------------------------------------------------------------------*/

static void WriteReportFIPS_140_2 (
   char *genName,                 /* Generator or file name */
   lebool Flag,                  /* = TRUE for a file, FALSE for a gen */
   int nbit,                      /* Number of bits */
   int longest0,                  /* Longest string of 0 */
   int longest1,                  /* Longest string of 1 */
   int nrun0[],                   /* Number of 0 runs */
   int nrun1[],                   /* Number of 1 runs */
   int ncount[],                  /* Number of 4 bits values */
   TestGlobals &globals
   )
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  
  
   int i, j;
   double X;
   fmass_INFO Q;
   double p, pLeft, pRight;
   lebool failFlag = FALSE;

   printf
      ("\n============== Summary results of FIPS-140-2 ==============\n\n");
   if (Flag) {
      printf (" File:             ");
   } else {
      printf (" Generator:        ");
   }
   printf ("%s", genName);
   printf ("\n Number of bits:   20000\n");

   printf ("\n       Test          s-value        p-value    FIPS Decision\n");
   printf (" --------------------------------------------------------\n");

   /* Monobit results */
   j = 0;
   printf (" %-20s", bbattery_TestNames[j]);
   printf (" %5d       ", nbit);
   Q = fmass_CreateBinomial (20000, 0.5, 0.5);
   pLeft = fdist_Binomial2 (Q, nbit);
   pRight = fbar_Binomial2 (Q, nbit);
   fmass_DeleteBinomial (Q);
   p = gofw_pDisc (pLeft, pRight);
   gofw_Writep0 (p);
   if ((nbit <= 9725) || nbit >= 10275) {
      printf (" %10s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %10s", "Pass");

   printf ("\n");

   /* Poker results */
   X = 0;
   for (i = 0; i < 16; i++)
      X += (double) ncount[i] * ncount[i];
   X = 16 * X / 5000 - 5000;
   j = 1;
   printf (" %-16s", bbattery_TestNames[j]);
   printf ("%10.2f       ", X);
   p = fbar_ChiSquare2 (15, 12, X);
   gofw_Writep0 (p);
   if ((X <= 2.16) || X >= 46.17) {
      printf (" %10s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %10s", "Pass");
   printf ("\n\n");

   /* Run results */
   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun0[1]);
   if ((nrun0[1] <= 2315) || nrun0[1] >= 2685) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun0[2]);
   if ((nrun0[2] <= 1114) || nrun0[2] >= 1386) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun0[3]);
   if ((nrun0[3] <= 527) || nrun0[3] >= 723) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun0[4]);
   if ((nrun0[4] <= 240) || nrun0[4] >= 384) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun0[5]);
   if ((nrun0[5] <= 103) || nrun0[5] >= 209) {
      failFlag = TRUE;
      printf (" %25s", "Fail");
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun0[6]);
   if ((nrun0[6] <= 103) || nrun0[6] >= 209) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun1[1]);
   if ((nrun1[1] <= 2315) || nrun1[1] >= 2685) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun1[2]);
   if ((nrun1[2] <= 1114) || nrun1[2] >= 1386) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun1[3]);
   if ((nrun1[3] <= 527) || nrun1[3] >= 723) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun1[4]);
   if ((nrun1[4] <= 240) || nrun1[4] >= 384) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun1[5]);
   if ((nrun1[5] <= 103) || nrun1[5] >= 209) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d", nrun1[6]);
   if ((nrun1[6] <= 103) || nrun1[6] >= 209) {
      printf (" %25s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %25s", "Pass");
   printf ("\n\n");

   /* Longest run results */
   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d       ", longest0);
   p = GetPLongest (longest0);
   gofw_Writep0 (p);
   if (longest0 >= 26) {
      printf (" %10s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %10s", "Pass");
   printf ("\n");

   printf (" %-20s", bbattery_TestNames[++j]);
   printf (" %5d       ", longest1);
   p = GetPLongest (longest1);
   gofw_Writep0 (p);
   if (longest1 >= 26) {
      printf (" %10s", "Fail");
      failFlag = TRUE;
   } else
      printf (" %10s", "Pass");
   printf ("\n");

   if (!failFlag) {
      printf (" ----------------------------------------------------------\n");
      printf (" All values are within the required intervals of FIPS-140-2\n");
   }
   printf ("\n\n\n");
}


/*-------------------------------------------------------------------------*/

#define SAMPLE 625                /* 625 * 32 = 20000 */
#define MASK4  15                 /* Mask of 4 bits */

static void FIPS_140_2 (unif01_Gen * gen, char *filename, TestGlobals &globals)
{
  // Inject globals
  auto &bbattery_pVal = globals.bbattery_pVal;
  auto &bbattery_TestNames = globals.bbattery_TestNames;
  auto &bbattery_NTests = globals.bbattery_NTests;
  auto &TestNumber = globals.TestNumber;
  

  
   int i, j;
   int nbit = 0;                  /* Number of bits */
   int longest0 = 0;              /* Longest string of 0 */
   int longest1 = 0;              /* Longest string of 1 */
   int nrun0[7] = { 0 };          /* Number of 0 runs */
   int nrun1[7] = { 0 };          /* Number of 1 runs */
   int ncount[16] = { 0 };        /* Number of 4 bits values */
   int prevBit;                   /* Previous bit */
   int len = 0;                   /* Length of run */
   unsigned long jBit;            /* Current bit */
   unsigned long Z;               /* Block of 32 bits */
   unsigned long Bits[SAMPLE + 1];
   lebool fileFlag = FALSE;
   char genName[LEN + 1] = "";

   InitBat (globals);
   if (swrite_Basic) {
      printf ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n"
         "                 Starting FIPS_140_2\n"
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n");
   }
   util_Assert (NULL == gen || NULL == filename,
      "bbattery_FIPS_140_2:   one of gen or filename must be NULL");
   util_Assert (!(NULL == gen && NULL == filename),
      "bbattery_FIPS_140_2:   no generator and no file");
   util_Assert (!(NULL == gen && !(strcmp (filename, ""))),
      "bbattery_FIPS_140_2:   no generator and no file");

   if ((NULL == gen) && filename && strcmp (filename, "")) {
      gen = ufile_CreateReadBin (filename, SAMPLE);
      fileFlag = TRUE;
   }

   for (j = 0; j < SAMPLE; j++)
      Bits[j] = unif01_StripB (gen, 0, 32);

   if (fileFlag) {
      ufile_DeleteReadBin (gen);
      strncpy (genName, filename, (size_t) LEN);
   } else {
      GetName (gen, genName);
   }

   /* Make sure to count the first run; set prevBit != {0, 1} */
   prevBit = 2;

   for (j = 0; j < SAMPLE; j++) {
      /* Count the number of 1 */
      Z = Bits[j];
      while (Z > 0) {
         Z &= Z - 1;              /* Clear lowest 1 bit */
         ++nbit;
      }

      /* Count the number of 4 bits values */
      Z = Bits[j];
      for (i = 0; i < 8; i++) {
         (ncount[Z & MASK4])++;
         Z >>= 4;
      }

      /* Count the number of runs and get the longest runs */
      Z = Bits[j];
      jBit = bitset_maskUL[31];

      while (jBit > 0) {
         if (Z & jBit) {          /* bit 1 */
            if (prevBit != 1) {
               if (len < 6)
                  (nrun0[len])++;
               else
                  (nrun0[6])++;
               if (len > longest0)
                  longest0 = len;
               len = 1;
            } else {
               len++;
            }
            prevBit = 1;

         } else {                 /* bit 0 */
            if (prevBit != 0) {
               if (len < 6)
                  (nrun1[len])++;
               else
                  (nrun1[6])++;
               if (len > longest1)
                  longest1 = len;
               len = 1;
            } else {
               len++;
            }
            prevBit = 0;
         }
         jBit >>= 1;
      }
   }

   strcpy (bbattery_TestNames[0], "Monobit");
   strcpy (bbattery_TestNames[1], "Poker");
   j = 1;
   strcpy (bbattery_TestNames[++j], "0 Runs, length 1: ");
   strcpy (bbattery_TestNames[++j], "0 Runs, length 2: ");
   strcpy (bbattery_TestNames[++j], "0 Runs, length 3: ");
   strcpy (bbattery_TestNames[++j], "0 Runs, length 4: ");
   strcpy (bbattery_TestNames[++j], "0 Runs, length 5: ");
   strcpy (bbattery_TestNames[++j], "0 Runs, length 6+: ");
   strcpy (bbattery_TestNames[++j], "1 Runs, length 1: ");
   strcpy (bbattery_TestNames[++j], "1 Runs, length 2: ");
   strcpy (bbattery_TestNames[++j], "1 Runs, length 3: ");
   strcpy (bbattery_TestNames[++j], "1 Runs, length 4: ");
   strcpy (bbattery_TestNames[++j], "1 Runs, length 5: ");
   strcpy (bbattery_TestNames[++j], "1 Runs, length 6+: ");

   strcpy (bbattery_TestNames[++j], "Longest run of 0: ");
   strcpy (bbattery_TestNames[++j], "Longest run of 1: ");

   WriteReportFIPS_140_2 (genName, fileFlag, nbit, longest0, longest1,
			  nrun0, nrun1, ncount, globals);
}


/*-------------------------------------------------------------------------*/

std::vector<bbattery_Result> bbattery_FIPS_140_2 (unif01_Gen * gen)
{
  TestGlobals globals;
  FIPS_140_2 (gen, NULL, globals);
  return makeResults(globals);
}


/*-------------------------------------------------------------------------*/

std::vector<bbattery_Result> bbattery_FIPS_140_2File (char *filename)
{
  TestGlobals globals;
  FIPS_140_2 (NULL, filename, globals);
  return makeResults(globals);
}


/*=========================================================================*/
