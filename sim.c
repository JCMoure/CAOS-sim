#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

Class PClasses[]= 
    {
     {"BRN",  1, 1},   // Branch is always element zero
     {"INT",  2, 2}, 
     {"FLOAT",1, 1}, 
     {"MEM",  1, 1} 
    };

Operation POps[]=     
    {
     {"BRN",  0, 1},  // Branch is always element zero
     {"IADD", 1, 1}, {"ICMP", 1, 1}, 
     {"FADD", 2, 3}, {"FMUL", 2, 5},
     {"LOAD", 3, 3}, {"STR",  3, 2}, {"LD-L2", 3, 13}, {"LD-RAM", 3, 113}
    };

Processor Proc = { 3 /* PIPE_Width */, 2, 0 /* CCount */, 4, PClasses, 9, POps };

Instruction Program[]= 
  { {5,-1,-1, 5, 3}, // LOAD  (depends on IADD)
    {0,-1,-1, 4, 2}, // FMUL  (depends on LOAD)
    {5,-1,-1, 5, 3}, // LOAD  (depends on IADD)
    {1, 2,-1, 3, 2}, // FADD  (depends on FMUL & LOAD)
    {3, 5,-1, 6, 3}, // STORE (depends on FADD)
    {5,-1,-1, 1, 1}, // IADD  (depends on itself)
    {5,-1,-1, 2, 1}, // ICMP  (depends on IADD)
    {6,-1,-1, 0, 0}, // BRN   (depends on ICMP)
  };

int When0[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int When1[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int When2[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int When3[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

Thread Thread0 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T0", Program, When0 };
Thread Thread1 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T1", Program, When1 };
Thread Thread2 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T2", Program, When2 };
Thread Thread3 = { 0 /* PC */, 8 /* N_Instr */, 0 /* ICount */, "T3", Program, When3 };


void output ( Processor *P, Thread *T ) {
  if (P == 0) {
    printf("...... ");
    return;
  }
  int classID  = T->Program [ T->PC ].classID;
  printf("%s:%d.%s ", T->name, T->PC, P->Classes[classID].name);
} 


void next_instruction ( Thread *T ) {  
   int PC = T->PC;
   int opID = T->Program[ PC ].operationID;
   PC++;
   if (PC == T->N_Instr ) PC= 0;  // assume last instruction is a backward branch
   T->PC = PC; // advance to next instruction
   T->ICount++;
}

int instruction_latency ( Thread *T, Processor *P ) {
  int opID= T->Program [ T->PC ].operationID;
  return P->Ops[opID].latency;
}


int check_dependences ( Thread *T, int currentCycle ) {  
  int s1 = T->Program[T->PC].source1;
  int s2 = T->Program[T->PC].source2;
  int s3 = T->Program[T->PC].source3;

  return ((s1 == -1) || (T->whenFinished[s1] <= currentCycle)) &&
         ((s2 == -1) || (T->whenFinished[s2] <= currentCycle)) &&
         ((s3 == -1) || (T->whenFinished[s3] <= currentCycle));
}

int check_resources ( Thread *T, Processor *P ) {
   int classID    = T->Program [ T->PC ].classID;
   return P->Classes[classID].available;
}

int consume_resources ( Thread *T, Processor *P ) {
   int classID    = T->Program [ T->PC ].classID;
   assert (P->Classes[classID].available > 0);
   P->Classes[classID].available--;
}


void reset_throughput ( Processor *P ) {
  int i;
  P->PIPE_avail = P->PIPE_width;
  for (i=0; i<P->Num_Classes; i++)
    P->Classes[i].available = P->Classes[i].throughput;
}

void sim_SEQUENTIAL (Processor *P, Thread *T, int CycleCount) {
  int CYCLE = 0;
  int WAIT_CYCLE = 0;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    if (WAIT_CYCLE == CYCLE) {
      WAIT_CYCLE += instruction_latency ( T, P );
      output ( P, T );
      next_instruction (T);
    } else {
      output (0, 0);
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}

void sim_PIPE1 (Processor *P, Thread *T, int CycleCount) {
  int CYCLE = 0;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    if (check_dependences (T, CYCLE)) {
      T->whenFinished[T->PC] = CYCLE + instruction_latency ( T, P );
      output (P, T);
      next_instruction (T);
    } else {
      output (0, 0);
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}

void sim_THROUGHPUT (Processor *P, Thread *T, int CycleCount) {
  int CYCLE = 0;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    reset_throughput( P );
    while (P->PIPE_avail) {
      if ( check_resources ( T, P )) {
        consume_resources ( T, P );;
        output (P, T);
        next_instruction (T);
      } else
        output (0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}


void sim_PIPELINE (Processor *P, Thread *T, int CycleCount) {
  int CYCLE = 0;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    reset_throughput( P );
    while (P->PIPE_avail) {
      if ( check_resources ( T, P ) && check_dependences (T, CYCLE)) {
        T->whenFinished[T->PC] = CYCLE + instruction_latency ( T, P );
        consume_resources ( T, P );;
        output ( P, T );
        next_instruction (T);
      } else
        output (0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}



void sim_PIPELINE_MT2 (Processor *P, Thread *T0, Thread *T1, int CycleCount) {

  Thread *T;
  int CYCLE = 0, NO_ISSUE=0;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    reset_throughput( P );
    T = T0;       // thread 0 has priority
    NO_ISSUE = 0; // init exit condition
    while (P->PIPE_avail && NO_ISSUE < 2) {
      if ( check_resources ( T, P ) && check_dependences (T, CYCLE)) {
        T->whenFinished[T->PC] = CYCLE + instruction_latency ( T, P );
        consume_resources ( T, P );;
        output ( P, T );
        next_instruction (T);
        P->PIPE_avail--;
      } else
        NO_ISSUE++;   // avoid deadlock situation
      if (T==T0) T=T1;   // switch threads
      else       T=T0;
    }
    while (P->PIPE_avail) {
      output (0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}


void sim_PIPELINE_MT4 (Processor *P, Thread *T0, Thread *T1, Thread *T2, Thread *T3, int CycleCount) {

  Thread *T;
  int CYCLE = 0, NO_ISSUE=0, turn=0; // thread 0 starts with priority
  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    reset_throughput( P );
    // round-robin priority: do nothing
    // random
    turn = rand()%4;
    switch (turn) {
      case 0: T=T0; break;
      case 1: T=T1; break;
      case 2: T=T2; break;
      case 3: T=T3; break;
    }
    NO_ISSUE = 0; // init exit condition
    while (P->PIPE_avail && NO_ISSUE < 4) {
      if ( check_resources ( T, P ) && check_dependences (T, CYCLE)) {
        T->whenFinished[T->PC] = CYCLE + instruction_latency ( T, P );
        consume_resources ( T, P );;
        output ( P, T );
        next_instruction (T);
        P->PIPE_avail--;
      } else
        NO_ISSUE++;   // avoid deadilock situation
      turn++; if (turn>3) turn=0;
      switch (turn) {
        case 0: T=T0; break;
        case 1: T=T1; break;
        case 2: T=T2; break;
        case 3: T=T3; break;
      }
    }
    while (P->PIPE_avail) {
      output (0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}




void main (int argc, char **argv) {
  int Cycles= 100;
  int option= 0;
  int seed = 0;
  if (argc>1) { Cycles= atoll(argv[1]); }
  if (argc>2) { option= atoll(argv[2]); }
  if (argc>3) { seed  = atoll(argv[3]); }
  if (argc<2) {
     printf("argumentos: Cycles [option [seed]]\n");
     return;
  }
  srand(seed);
  switch (option) {
    case 0: sim_SEQUENTIAL ( &Proc, &Thread0, Cycles ); 
            printf ("\n\nProcessor= SEQUENTIAL    "); break;
    case 1: sim_THROUGHPUT ( &Proc, &Thread0, Cycles );
            printf ("\n\nProcessor= THROUGHPUT    "); break;
    case 2: sim_PIPE1      ( &Proc, &Thread0, Cycles );
            printf ("\n\nProcessor= PIPE SIMPLE   "); break;
    case 3: sim_PIPELINE   ( &Proc, &Thread0, Cycles );
            printf ("\n\nProcessor= PIPELINE      "); break;
    case 4: sim_PIPELINE_MT2( &Proc, &Thread0, &Thread1, Cycles );
            printf ("\n\nProcessor= PIPELINE-MT2  "); break;
    case 5: sim_PIPELINE_MT4( &Proc, &Thread0, &Thread1, &Thread2, &Thread3, Cycles );
            printf ("\n\nProcessor= PIPELINE-MT4  "); break;
    default:
            printf ("\n\nOption NOT IMPLEMENTED !! \n\n  "); return;
  }
  if (option < 4)
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / Thread0.ICount, ((float) Thread0.ICount) / Cycles); 
  else if (option == 4)
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / (Thread0.ICount+Thread1.ICount),
                                     ((float) Thread0.ICount + Thread1.ICount) / Cycles); 
  else
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / (Thread0.ICount+Thread1.ICount+Thread2.ICount+Thread3.ICount),
                                     ((float) Thread0.ICount+Thread1.ICount+Thread2.ICount+Thread3.ICount) / Cycles); 
}
