#include "sim.h"
#include <stdio.h>
#include <stdlib.h>

Class PClasses[]= 
    {
     {"BRN", 1, 1},
     {"INT", 2, 2}, 
     {"FLOAT", 1, 1}, 
     {"MEM", 1, 1} 
    };

Operation POps[]=     
    {
     {"BRN", 0, 1},
     {"IADD", 1, 1}, {"ICMP", 1, 1}, {"IMUL", 1, 6},  {"IDIV", 1, 12},
     {"FADD", 2, 3}, {"FMUL", 2, 5}, {"FDIV", 2, 12}, {"FMOV", 2, 2},
     {"LOAD", 3, 3}, {"STR", 3, 2},  {"LdL2", 3, 13}, {"LRAM", 3, 113}
    };

void sim_SEQUENTIAL   (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPE1        (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_THROUGHPUT   (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE     (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE_MT2 (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE_MT4 (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPE_ROB     (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPE_ROB_thr (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);


void main (int argc, char **argv) {
  unsigned Cycles= 100;
  int      option= 0;
  Thread  *T0;
  Processor Proc;

  Proc.PIPE_width    = 3; // Default width
  Proc.Num_Classes   = 4;
  Proc.Classes       = PClasses;
  Proc.Num_Operations= 13;
  Proc.Ops           = POps;

  if (argc>1) { T0 = Thread_read (argv[1], "T0", &Proc); }
  if (argc>2) { Cycles= atoll(argv[2]); }
  if (argc>3) { option= atoll(argv[3]); }
  if (!T0) {
     printf("error parsing input file!\n");
     return;
  }
  if (argc<4) {
     printf("argumentos: Program Cycles Option [ other args .... ]\n");
     return;
  }
  argc -=3;
  argv +=3;
  switch (option) {
    case 0: sim_SEQUENTIAL  ( argc, argv, &Proc, T0, Cycles );  break;
    case 1: sim_THROUGHPUT  ( argc, argv, &Proc, T0, Cycles );  break;
    case 2: sim_PIPE1       ( argc, argv, &Proc, T0, Cycles );  break;
    case 3: sim_PIPELINE    ( argc, argv, &Proc, T0, Cycles );  break;
    case 4: sim_PIPELINE_MT2( argc, argv, &Proc, T0, Cycles );  break;
    case 5: sim_PIPELINE_MT4( argc, argv, &Proc, T0, Cycles );  break;
    case 6: sim_PIPE_ROB    ( argc, argv, &Proc, T0, Cycles );  break;
    case 7: sim_PIPE_ROB_thr( argc, argv, &Proc, T0, Cycles );  break;
    default:  printf ("\n\nOption NOT IMPLEMENTED !! \n\n  "); return;
  }
}
