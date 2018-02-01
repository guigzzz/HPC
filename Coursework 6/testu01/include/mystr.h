 
/* mystr.h for ANSI C */

#ifndef MYSTR_H
#define MYSTR_H
 

void mystr_Delete (char S[], unsigned int index, unsigned int len);


void mystr_Insert (char Res[], const char Source[], unsigned int Pos);



void mystr_ItemS (char R[], char S[], const char T[], unsigned int N);



int mystr_Match (const char Source[], const char Pattern[]);



void mystr_Slice (char R[], char S[], unsigned int P, unsigned int L);



void mystr_Subst (char Source[], const char OldPattern[], const char NewPattern[]);



void mystr_Position (const char Substring[], const char Source[], unsigned int at,
                     unsigned int * pos, int * found);

 

#endif
 
