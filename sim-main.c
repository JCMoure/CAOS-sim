#include "sim.h"
#include <stdio.h>
#include <stdlib.h>

Class PClasses[]= 
    {
     {" BRN ",  1, 1},   // Branch is always element zero
     {" INT ",  2, 2}, 
     {"FLOAT",1, 1}, 
     {" MEM ",  1, 1} 
    };

Operation POps[]=     
    {
     {"BRN ",  0, 1},  // Branch is always element zero
     {"IADD", 1, 1}, {"ICMP", 1, 1}, 
     {"FADD", 2, 3}, {"FMUL", 2, 5},
     {"LOAD", 3, 3}, {"STR ",  3, 2}, {"LdL2", 3, 13}, {"LRAM", 3, 113}
    };

Processor Proc = { 3 /* PIPE_Width */, 2, 0 /* CCount */, 4, PClasses, 9, POps };

Instruction Program[]= 
  { {-3, 0, 0, 5, 3, 0}, // LOAD  (depends on IADD)
    {-1, 0, 0, 4, 2, 0}, // FMUL  (depends on LOAD)
    {-5, 0, 0, 5, 3, 0}, // LOAD  (depends on IADD)
    {-2,-1, 0, 3, 2, 0}, // FADD  (depends on FMUL & LOAD)
    {-1,-7, 0, 6, 3, 0}, // STORE (depends on FADD & IADD)
    {-8, 0, 0, 1, 1, 0}, // IADD  (depends on itself)
    {-1, 0, 0, 2, 1, 0}, // ICMP  (depends on IADD)
    {-1, 0, 0, 0, 0, 0}, // BRN   (depends on ICMP)
  };

int When0[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int When1[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int When2[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int When3[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

Thread Thread0 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T0", Program, When0 };
Thread Thread1 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T1", Program, When1 };
Thread Thread2 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T2", Program, When2 };
Thread Thread3 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T3", Program, When3 };


void sim_SEQUENTIAL   (Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPE1        (Processor *P, Thread *T, unsigned CycleCount);
void sim_THROUGHPUT   (Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE     (Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE_MT2 (int argc, char **argv, Processor *P, Thread *T0, Thread *T1, unsigned CycleCount);
void sim_PIPELINE_MT4 (int argc, char **argv, Processor *P, Thread *T0, Thread *T1, Thread *T2, Thread *T3, 
 										    unsigned CycleCount);
void sim_PIPE_ROB     (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);


void main (int argc, char **argv) {
  unsigned Cycles= 100;
  int option= 0;
  if (argc>1) { Cycles= atoll(argv[1]); }
  if (argc>2) { option= atoll(argv[2]); }
  else {
     printf("argumentos: Cycles option [ other args .... ]\n");
     return;
  }
  switch (option) {
    case 0: sim_SEQUENTIAL ( &Proc, &Thread0, Cycles );  break;
    case 1: sim_THROUGHPUT ( &Proc, &Thread0, Cycles );  break;
    case 2: sim_PIPE1      ( &Proc, &Thread0, Cycles );  break;
    case 3: sim_PIPELINE   ( &Proc, &Thread0, Cycles );  break;
    case 4: sim_PIPELINE_MT2( argc, argv, &Proc, &Thread0, &Thread1, Cycles ); break;
    case 5: sim_PIPELINE_MT4( argc, argv, &Proc, &Thread0, &Thread1, &Thread2, &Thread3, Cycles ); break;
    case 6: sim_PIPE_ROB    ( argc, argv, &Proc, &Thread0, Cycles );  break;
    default:  printf ("\n\nOption NOT IMPLEMENTED !! \n\n  "); return;
  }
  if (option < 4 || option > 5)
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / Thread0.ICount, ((float) Thread0.ICount) / Cycles); 
  else if (option == 4)
    printf ("CPI: %f   IPC: %f  T0-ICount: %d  T1-ICount: %d\n\n", 
             ((float) Cycles) / (Thread0.ICount+Thread1.ICount),
             ((float) Thread0.ICount + Thread1.ICount) / Cycles,
             Thread0.ICount, Thread1.ICount); 
  else
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / (Thread0.ICount+Thread1.ICount+Thread2.ICount+Thread3.ICount),
                                     ((float) Thread0.ICount+Thread1.ICount+Thread2.ICount+Thread3.ICount) / Cycles); 
}
