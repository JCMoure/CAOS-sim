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


//////////////// THREAD //////////////////7

int Thread_getPC ( Thread *T) {
  return T->PC;
}

int Thread_getCurrentOpID ( Thread *T)
{
  return T->Program[ T->PC ].operationID;
}

int Thread_getCurrentClassID ( Thread *T)
{
  return T->Program[ T->PC ].classID;
}

int Thread_getOpID ( Thread *T, int PC)
{
  return T->Program[ PC ].operationID;
}

int Thread_getClassID ( Thread *T, int PC)
{
  return T->Program[ PC ].classID;
}

int Thread_getNext (Thread *T, int PC) {
   PC++;
   if (PC == T->N_Instr ) PC= 0;  // assume last instruction is a backward branch
   return PC;
}

void Thread_next ( Thread *T ) {  
   T->PC = Thread_getNext ( T, T->PC);
   T->ICount++;
}

void Thread_setFinished ( Thread *T, int cycle ) {
   T->whenFinished[T->PC] = cycle;
}


////////////// ROB ///////////////////////////

ROB * ROB_init   ( Thread *T, int Sz ){
  int i;
  ROB *R = malloc (sizeof(ROB));
  R->T=    T;
  R->Head= 0;
  R->Tail= 0;
  R->n   = 0;
  R->Size= Sz;
  R->whenFinished= malloc (  Sz*sizeof(int) );
  for (i=0; i<Sz; i++)
    R->whenFinished[i] = (unsigned) -1; // maximum unsigned value 
}


void ROB_insert ( ROB *R, int k ) {
  k= ((R->n+k) > R->Size)? R->Size-R->n: k;
  R->n += k;
  while (k) {
    R->whenFinished[R->Tail]= (unsigned) -1;
    R->Tail = (R->Tail == R->Size-1)? 0: R->Tail+1; 
    k--;
  }
}


void ROB_retire ( ROB *R, int k, unsigned currentCycle ) {
  while ( k && R->n && R->whenFinished[R->Head] < currentCycle ) {
    Thread_next ( R->T );  // retire instruction
    R->Head = (R->Head == R->Size-1)? 0: R->Head+1; 
    R->n--;
    k--;
  }
}

int  ROB_getPos  ( ROB *R ) {
  return R->Head;
}

int  ROB_getPC  ( ROB *R, int Pos ) {
  int PC = Thread_getPC ( R->T );
  int Ps = R->Head;
  while ( Ps != Pos) {
    PC = Thread_getNext ( R->T, PC );
    Ps = (Ps == R->Size-1)? 0: Ps+1; 
  }
  return PC;
}

int ROB_check ( ROB *R, int PC, unsigned CYCLE) {
  int s1 = R->T->Program[PC].source1;
  int s2 = R->T->Program[PC].source2;
  int s3 = R->T->Program[PC].source3;

  return ((s1 == -1) || (R->whenFinished[s1] <= CYCLE)) &&
         ((s2 == -1) || (R->whenFinished[s2] <= CYCLE)) &&
         ((s3 == -1) || (R->whenFinished[s3] <= CYCLE));
}

int ROB_getAvail ( ROB *R, int Pos, unsigned CYCLE ) {
  int PC = ROB_getPC ( R, Pos );

  while ( Pos != R->Tail && !ROB_check ( R, PC, CYCLE ) ) {
    PC = Thread_getNext ( R->T, PC );
    Pos= (Pos == R->Size-1)? 0: Pos+1; 
  }
  if (Pos == R->Tail)
    return -1;
  return Pos;
}

int ROB_setFinished ( ROB *R, int Pos, unsigned CYCLE) {
  R->whenFinished[Pos]= CYCLE;
}


int check_dependences ( Thread *T, int PC, unsigned currentCycle ) {  
  int s1 = T->Program[PC].source1;
  int s2 = T->Program[PC].source2;
  int s3 = T->Program[PC].source3;

  return ((s1 == -1) || (T->whenFinished[s1] <= currentCycle)) &&
         ((s2 == -1) || (T->whenFinished[s2] <= currentCycle)) &&
         ((s3 == -1) || (T->whenFinished[s3] <= currentCycle));
}

/////// PROCESSOR //////////////////////////7

void Processor_reset (Processor *P ) {
  int i;
  P->PIPE_avail = P->PIPE_width;
  for (i=0; i<P->Num_Classes; i++)
    P->Classes[i].available = P->Classes[i].throughput;
}

int Processor_getLatency (Processor *P, int OpID) {
  return P->Ops[OpID].latency;
}

int Processor_checkResource ( Processor *P, int classID ) {
   return P->Classes[classID].available;
}

void Processor_consumeResource ( Processor *P, int classID ) {
   assert (P->Classes[classID].available > 0);
   P->Classes[classID].available--;
}



///////////// OUTPUT ///////////////////
void output ( Processor *P, Thread *T, int PC ) {
  if (P == 0) {
    printf("...... ");
    return;
  }
  int classID  = Thread_getClassID ( T, PC );
  printf("%s:%d.%s ", T->name, PC, P->Classes[classID].name);
} 


///////////////////////////////////////////////////////////7

void sim_SEQUENTIAL (Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int WAIT_CYCLE = 0;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    if (WAIT_CYCLE == CYCLE) {
      WAIT_CYCLE += Processor_getLatency ( P, Thread_getCurrentOpID( T ));
      output ( P, T, Thread_getPC (T) );
      Thread_next (T);
    } else {
      output (0, 0, 0);
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}



///////////////////////////////////////////////////////////7

void sim_PIPE1 (Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    PC = Thread_getPC (T);
    if (check_dependences (T, PC, CYCLE)) {
      Thread_setFinished (T, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC )));
      output (P, T, PC );
      Thread_next (T);
    } else {
      output (0, 0, 0);
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}



///////////////////////////////////////////////////////////7

void sim_THROUGHPUT (Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
    while (P->PIPE_avail) {
      PC = Thread_getPC (T);
      int classID = Thread_getClassID ( T, PC );
      if ( Processor_checkResource ( P, classID )) {
        Processor_consumeResource ( P, classID );;
        output (P, T, PC);
        Thread_next (T);
      } else
        output (0, 0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}



///////////////////////////////////////////////////////////7

void sim_PIPELINE (Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC, classID;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
    while (P->PIPE_avail) {
      PC = Thread_getPC (T);
      classID = Thread_getClassID ( T, PC );
      if ( Processor_checkResource ( P, classID ) && check_dependences (T, PC, CYCLE)) {
        Thread_setFinished (T, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC ) ));
        Processor_consumeResource ( P, classID );
        output ( P, T, PC );
        Thread_next (T);
      } else
        output (0, 0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}



///////////////////////////////////////////////////////////7

void sim_PIPELINE_MT2 (Processor *P, Thread *T0, Thread *T1, unsigned CycleCount) {

  Thread *T;
  unsigned CYCLE = 0;
  int  NO_ISSUE=0;
  int PC, classID;

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
    T = T0;       // thread 0 has priority
    NO_ISSUE = 0; // init exit condition
    while (P->PIPE_avail && NO_ISSUE < 2) {
      PC = Thread_getPC (T);
      classID = Thread_getClassID ( T, PC );
      if ( Processor_checkResource ( P, classID ) && check_dependences (T, PC, CYCLE)) {
        Thread_setFinished (T, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC ) ));
        Processor_consumeResource ( P, classID );
        output ( P, T, PC );
        Thread_next (T);
        P->PIPE_avail--;
        NO_ISSUE = 0; // init exit condition
      } else
        NO_ISSUE++;   // avoid deadlock situation
      if (T==T0) T=T1;   // switch threads
      else       T=T0;
    }
    while (P->PIPE_avail) {
      output (0, 0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}



///////////////////////////////////////////////////////////7

void sim_PIPELINE_MT4 (Processor *P, Thread *T0, Thread *T1, Thread *T2, Thread *T3, unsigned CycleCount) {

  Thread *T;
  unsigned CYCLE = 0;
  int NO_ISSUE=0, turn=0; // thread 0 starts with priority
  int PC, classID;
  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
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
      PC = Thread_getPC (T);
      classID = Thread_getClassID ( T, PC );
      if ( Processor_checkResource ( P, classID ) && check_dependences (T, PC, CYCLE)) {
        Thread_setFinished (T, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC ) ));
        Processor_consumeResource ( P, classID );
        output ( P, T, PC );
        Thread_next (T);
        P->PIPE_avail--;
        NO_ISSUE = 0; // init exit condition
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
      output (0, 0, 0);
      P->PIPE_avail--;
    }
    printf("\n");
  }
  P->CCount = CYCLE;
}


///////////////////////////////////////////////////////////7

void sim_PIPE_ROB_1 (Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC, Pos;
  ROB *R;

  R = ROB_init ( T, 32);  // ROB size is 32

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    ROB_retire ( R, 1, CYCLE);  // ROB retire width
    ROB_insert ( R, 1);         // ROB insert width
    Pos = ROB_getPos (R);
    if ( (Pos = ROB_getAvail ( R, Pos, CYCLE )) >= 0 ) {
      PC = ROB_getPC (R, Pos);
      output ( P, T, PC );
      ROB_setFinished ( R, Pos, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC )));
    }
    else
      output ( 0, 0, 0 );

    printf("\n");
  }
  P->CCount = CYCLE;
}


void main (int argc, char **argv) {
  unsigned Cycles= 100;
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
    case 6: sim_PIPE_ROB_1 ( &Proc, &Thread0, Cycles );
            printf ("\n\nProcessor= PIPE ROB      "); break;
    default:
            printf ("\n\nOption NOT IMPLEMENTED !! \n\n  "); return;
  }
  if (option < 4 || option > 5)
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / Thread0.ICount, ((float) Thread0.ICount) / Cycles); 
  else if (option == 4)
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / (Thread0.ICount+Thread1.ICount),
                                     ((float) Thread0.ICount + Thread1.ICount) / Cycles); 
  else
    printf ("CPI: %f   IPC: %f\n\n", ((float) Cycles) / (Thread0.ICount+Thread1.ICount+Thread2.ICount+Thread3.ICount),
                                     ((float) Thread0.ICount+Thread1.ICount+Thread2.ICount+Thread3.ICount) / Cycles); 

}
