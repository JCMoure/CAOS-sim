#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//////////////// THREAD //////////////////7

int Thread_getPC ( Thread *T) {
  return T->PC;
}

int Thread_getCurrentOpID ( Thread *T) {
  return T->Program[ T->PC ].operationID;
}

int Thread_getCurrentClassID ( Thread *T) {
  return T->Program[ T->PC ].classID;
}

int Thread_getOpID ( Thread *T, int PC) {
  return T->Program[ PC ].operationID;
}

int Thread_getClassID ( Thread *T, int PC) {
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

////////// PIPE ////////////////////////

PIPE * PIPE_init ( Thread * T) {
  int i;
  PIPE *PP = malloc (sizeof(PIPE));
  PP->T   = T;
  PP->Sz  = T->N_Instr;
  PP->whenFinished= malloc (  PP->Sz*sizeof(unsigned) );
  for (i=0; i<PP->Sz; i++)
    PP->whenFinished[i] = 0;
  return PP;
}

void PIPE_setFinished ( PIPE *PP, int Pos, unsigned cycle ) {
   PP->whenFinished[Pos] = cycle;
}

int PIPE_check ( PIPE *PP, int PC, unsigned currentCycle ) {  
  int s1 = PP->T->Program[PC].source1;
  int s2 = PP->T->Program[PC].source2;
  int s3 = PP->T->Program[PC].source3;
  int N= PP->Sz;

  PP->whenFinished[PC]= 0;

  s1 = PC+s1; if ( s1 > N ) s1 = s1 - N; 
  s2 = PC+s2; if ( s2 > N ) s2 = s2 - N; 
  s3 = PC+s3; if ( s3 > N ) s3 = s3 - N; 

  return ( (PP->whenFinished[s1] <= currentCycle) ) &&
         ( (PP->whenFinished[s2] <= currentCycle) ) &&
         ( (PP->whenFinished[s3] <= currentCycle) );
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
  R->whenFinished= malloc (  Sz*sizeof(unsigned) );
  for (i=0; i<Sz; i++)
    R->whenFinished[i] = 0; // Instructions not in ROB always available 
  return R;
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
  while ( k && R->n && (R->whenFinished[R->Head] <= currentCycle) ) {
    Thread_next ( R->T );  // retire instruction
    R->whenFinished[R->Head]= 0;
    R->Head = (R->Head == R->Size-1)? 0: R->Head+1; 
    R->n--;
    k--;
  }
}

int  ROB_getHead( ROB *R ) {
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


int ROB_check ( ROB *R, int Pos, int checked, int PC, unsigned CYCLE) {
  int s1, s2, s3;

  if (R->whenFinished[Pos] != (unsigned) -1)
    return 0;  // already executed

  s1 = R->T->Program[PC].source1;
  s2 = R->T->Program[PC].source2;
  s3 = R->T->Program[PC].source3;

  if (checked+s1 < 0) s1 = 0;  // dependent instruction already retired
  if (checked+s2 < 0) s2 = 0;  // dependent instruction already retired
  if (checked+s3 < 0) s3 = 0;  // dependent instruction already retired
  
  s1 = Pos+s1; if ( s1 < 0 ) s1 = s1 + R->Size; // circular buffer
  s2 = Pos+s2; if ( s2 < 0 ) s2 = s2 + R->Size; 
  s3 = Pos+s3; if ( s3 < 0 ) s3 = s3 + R->Size; 

  return ( s1 == Pos || R->whenFinished[s1] <= CYCLE ) &&
         ( s2 == Pos || R->whenFinished[s2] <= CYCLE ) &&
         ( s3 == Pos || R->whenFinished[s3] <= CYCLE );
}

int ROB_getReady ( ROB *R, unsigned CYCLE ) {
  int Pos = R->Head;
  int PC = Thread_getPC ( R->T );
  int checked = 0;

  while ( checked < R->n && !ROB_check ( R, Pos, checked, PC, CYCLE ) ) {
    PC = Thread_getNext ( R->T, PC );
    Pos= (Pos == R->Size-1)? 0: Pos+1;
    checked++;
  }
  if ( checked == R->n)
    return -1;
  return Pos;
}


int ROB_getReady_Avail ( ROB *R, int Pinit, Processor *P, unsigned CYCLE ) {
  int Pos = R->Head;
  int PC = Thread_getPC ( R->T );
  int checked = 0;

  while ( checked < R->n && Pos != Pinit ) { // seek starting position
    PC = Thread_getNext ( R->T, PC );
    Pos= (Pos == R->Size-1)? 0: Pos+1;
    checked++;
  }

CONTINUE:

  while ( checked < R->n && !ROB_check ( R, Pos, checked, PC, CYCLE ) ) {
    PC = Thread_getNext ( R->T, PC );
    Pos= (Pos == R->Size-1)? 0: Pos+1;
    checked++;
  }
  if ( checked == R->n)
    return -1;

  int classID = Thread_getClassID ( R->T, PC );
  if ( Processor_checkResource ( P, classID )) {
    Processor_consumeResource ( P, classID );
    return Pos;
  }
  PC = Thread_getNext ( R->T, PC );
  Pos= (Pos == R->Size-1)? 0: Pos+1;
  checked++;
  goto CONTINUE;
}



void ROB_setFinished ( ROB *R, int Pos, unsigned CYCLE) {
  R->whenFinished[Pos]= CYCLE;
}


void ROB_dump ( ROB *R ) {
  int i, p;
  printf("ROB Head: %d; Tail: %d; N: %d; PC: %d, Wait:[", R->Head, R->Tail, R->n, R->T->PC);
  for (i=0, p=R->Head; i<R->n; i++) {
    printf("%d, ", R->whenFinished[p]);
    p= (p == R->Size-1)? 0: p+1;
  }
  printf("]");
} 

/////// PROCESSOR //////////////////////////7

void Processor_reset (Processor *P ) {
  int i;
  for (i=0; i<P->Num_Classes; i++)
    P->Classes[i].available = P->Classes[i].throughput;
}

int Processor_getLatency (Processor *P, int OpID) {
  return P->Ops[OpID].latency;
}

int Processor_getClassID (Processor *P, int OpID) {
  return P->Ops[OpID].classID;
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
    printf(".............. ");
    return;
  }
  int classID= Thread_getClassID ( T, PC );
  int OpID=    Thread_getOpID    ( T, PC );
  printf("%2d.%s(%s) ", PC, P->Classes[classID].name, P->Ops[OpID].name);
} 


void output_thread ( Processor *P, Thread *T, int PC ) {
  if (P == 0) {
    printf("...........  ");
    return;
  }
  int classID  = Thread_getClassID ( T, PC );
  printf("%s:%2d.%s  ", T->name, PC, P->Classes[classID].name);
} 

////////////////// INPUT //////////////////////////

Thread * Thread_dup ( Thread *Tin, char *name ) {
  Thread *T;
  T = (Thread *) malloc ( sizeof(Thread) );
  T->N_Instr= Tin->N_Instr;
  T->PC = 0;
  T->ICount= 0;
  T->name = name;
  T->Program = Tin->Program;
  return T;
}

Thread * Thread_read (char *Filename, char *ThreadName, Processor *P) {
  Instruction *I;
  Thread      *T;
  FILE *F;
  int i, N, s1, s2, s3, opID;

  F= fopen(Filename, "r");
  if (!F) return 0;

  T = (Thread *) malloc ( sizeof(Thread) );
  fscanf( F, "%d\n", &N);
  T->N_Instr= N;
  T->PC = 0;
  T->ICount= 0;
  T->name  = ThreadName;

  I = (Instruction *) malloc ( N*sizeof(Instruction) );
  T->Program = I;
  for (i=0; i<N; i++) {
    fscanf( F, "%d,%d,%d,%d\n", &s1, &s2, &s3, &opID);
    I[i].source1 = s1;
    I[i].source2 = s2;
    I[i].source3 = s3;
    I[i].operationID = opID;
    I[i].classID     = Processor_getClassID (P, opID);
  }
  return T;
}
