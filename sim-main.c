#include "sim.h"
#include <stdio.h>
#include <stdlib.h>

void sim_SEQUENTIAL   (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE     (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE_MT2 (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPELINE_MT4 (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);
void sim_PIPE_ROB     (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount);

void main (int argc, char **argv) {
  unsigned  Cycles= 100;
  int       option= 0;
  Thread    *T0;
  Processor *Proc;

  if (argc < 5) {
     printf("arguments: Processor[.pr] Program[.prg] Cycles Option [ other args .... ]\n");
     printf("     0. Sequential\n     1. In-Order Pipeline\n");
     printf("     2. In-Order 2-threads\n     3. In-Order 4-threads\n");
     printf("     4. Out-Of-Order\n");
     return;
  }

  Proc = Processor_read (argv[1]);
  T0 = Thread_read (argv[2], "T0", Proc);
  Cycles= atoll(argv[3]);
  option= atoll(argv[4]);

  if (!Proc->PIPE_width || !T0) {
     Processor_dump(Proc);
     printf("error parsing input file!\n");
     return;
  }
  argc -=4;
  argv +=4;
  switch (option) {
    case 0: sim_SEQUENTIAL  ( argc, argv, Proc, T0, Cycles );  break;
    case 1: sim_PIPELINE    ( argc, argv, Proc, T0, Cycles );  break;
    case 2: sim_PIPELINE_MT2( argc, argv, Proc, T0, Cycles );  break;
    case 3: sim_PIPELINE_MT4( argc, argv, Proc, T0, Cycles );  break;
    case 4: sim_PIPE_ROB    ( argc, argv, Proc, T0, Cycles );  break;
    default:  printf ("\n\nOption NOT IMPLEMENTED !! \n\n  "); return;
  }
}
