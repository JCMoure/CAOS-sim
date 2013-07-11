#include "sim.h"
#include <stdio.h>
#include <stdlib.h>

Class PClasses[]= 
    {
     {" BRN ", 1, 1},
     {" INT ", 2, 2}, 
     {"FLOAT", 1, 1}, 
     {" MEM ", 1, 1} 
    };

Operation POps[]=     
    {
     {"BRN ", 0, 1},
     {"IADD", 1, 1}, {"ICMP", 1, 1}, {"IMUL", 1, 6},  {"IDIV", 1, 12},
     {"FADD", 2, 3}, {"FMUL", 2, 5}, {"FDIV", 2, 12}, {"FMOV", 2, 2},
     {"LOAD", 3, 3}, {"STR ", 3, 2}, {"LdL2", 3, 13}, {"LRAM", 3, 113}
    };

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
  int      option= 0;
  Thread  *T0, *T1, *T2, *T3;
  Processor Proc;

  Proc.PIPE_width    = 3;
  Proc.Num_Classes   = 4;
  Proc.Classes       = PClasses;
  Proc.Num_Operations= 13;
  Proc.Ops           = POps;

  if (argc>1) { T0 = Thread_read (argv[1], "T0", &Proc); }
  if (argc>2) { Cycles= atoll(argv[2]); }
  if (argc>3) { option= atoll(argv[3]); }
  else {
     printf("argumentos: Program Cycles Option [ other args .... ]\n");
     return;
  }
  argc -=3;
  argv +=3;
  T1= Thread_dup (T0, "T1");
  T2= Thread_dup (T0, "T2");
  T3= Thread_dup (T0, "T3");
  switch (option) {
    case 0: sim_SEQUENTIAL ( &Proc, T0, Cycles );  break;
    case 1: sim_THROUGHPUT ( &Proc, T0, Cycles );  break;
    case 2: sim_PIPE1      ( &Proc, T0, Cycles );  break;
    case 3: sim_PIPELINE   ( &Proc, T0, Cycles );  break;
    case 4: sim_PIPELINE_MT2( argc, argv, &Proc, T0, T1, Cycles ); break;
    case 5: sim_PIPELINE_MT4( argc, argv, &Proc, T0, T1, T2, T3, Cycles ); break;
    case 6: sim_PIPE_ROB    ( argc, argv, &Proc, T0, Cycles );  break;
    default:  printf ("\n\nOption NOT IMPLEMENTED !! \n\n  "); return;
  }
  if (option < 4 || option > 5)
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / T0->ICount, ((float) T0->ICount) / Cycles); 
  else if (option == 4)
    printf ("CPI: %f   IPC: %f  T0-ICount: %d  T1-ICount: %d\n\n", 
             ((float) Cycles) / (T0->ICount+T1->ICount),
             ((float) T0->ICount + T1->ICount) / Cycles,
             T0->ICount, T1->ICount); 
  else
    printf ("CPI: %f   IPC: %f  T0: %d  T1: %d  T2: %d  T3: %d\n\n", 
              ((float) Cycles) / (T0->ICount+T1->ICount+T2->ICount+T3->ICount),
              ((float) T0->ICount+T1->ICount+T2->ICount+T3->ICount) / Cycles,
              T0->ICount, T1->ICount, T2->ICount, T3->ICount); 
}
