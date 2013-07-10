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
   T->Program[T->PC].count++;
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

int  ROB_getPC  ( ROB *R, int Pos ) {
  int PC = Thread_getPC ( R->T );
  int Ps = R->Head;
  while ( Ps != Pos) {
    PC = Thread_getNext ( R->T, PC );
    Ps = (Ps == R->Size-1)? 0: Ps+1; 
  }
  return PC;
}


int check_dependences ( Thread *T, int PC, unsigned currentCycle ) {  
  int s1 = T->Program[PC].source1;
  int s2 = T->Program[PC].source2;
  int s3 = T->Program[PC].source3;

  T->whenFinished[PC]= 0;

  s1 = PC+s1; if ( s1 > T->N_Instr ) s1 = s1 - T->N_Instr; 
  s2 = PC+s2; if ( s2 > T->N_Instr ) s2 = s2 - T->N_Instr; 
  s3 = PC+s3; if ( s3 > T->N_Instr ) s3 = s3 - T->N_Instr; 

  return ( (T->whenFinished[s1] <= currentCycle) ) &&
         ( (T->whenFinished[s2] <= currentCycle) ) &&
         ( (T->whenFinished[s3] <= currentCycle) );
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

int ROB_getAvail ( ROB *R, unsigned CYCLE ) {
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

int ROB_setFinished ( ROB *R, int Pos, unsigned CYCLE) {
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
  int classID= Thread_getClassID ( T, PC );
  int OpID=    Thread_getOpID    ( T, PC );
  printf("%d(%d).%s(%s)   ", PC, T->Program[PC].count, P->Classes[classID].name, P->Ops[OpID].name);
} 


void output_thread ( Processor *P, Thread *T, int PC ) {
  if (P == 0) {
    printf("...... ");
    return;
  }
  int classID  = Thread_getClassID ( T, PC );
  printf("%s:%d.%s   ", T->name, PC, P->Classes[classID].name);
} 
