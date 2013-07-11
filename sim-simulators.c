#include "sim.h"
#include <stdio.h>
#include <assert.h>

///////////////////////////////////////////////////////////7

void sim_SEQUENTIAL (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount) {
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
  printf ("\n\nProcessor: SEQUENTIAL execution\n");
  printf ("CPI: %f   IPC: %f\n\n", ((float) CycleCount) / T->ICount, ((float) T->ICount) / CycleCount); 
}



///////////////////////////////////////////////////////////7

void sim_PIPE1 (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int      PC;
  PIPE *PP= PIPE_init (T);

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    PC = Thread_getPC (T);
    if (PIPE_check (PP, PC, CYCLE)) {
      PIPE_setFinished (PP, PC, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC )));
      output (P, T, PC );
      Thread_next (T);
    } else {
      output (0, 0, 0);
    }
    printf("\n");
  }
  printf ("\n\nProcessor: In-Order SIMPLE PIPELINE (single-issue)\n");
  printf ("CPI: %f   IPC: %f\n\n", ((float) CycleCount) / T->ICount, ((float) T->ICount) / CycleCount); 
}



///////////////////////////////////////////////////////////7

void sim_THROUGHPUT (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC, PIPE_avail;

  if (argc>1) { P->PIPE_width = atoll(argv[1]); }

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
    PIPE_avail= P->PIPE_width;
    while (PIPE_avail) {
      PC = Thread_getPC (T);
      int classID = Thread_getClassID ( T, PC );
      if ( Processor_checkResource ( P, classID )) {
        Processor_consumeResource ( P, classID );;
        output (P, T, PC);
        Thread_next (T);
      } else
        output (0, 0, 0);
      PIPE_avail--;
    }
    printf("\n");
  }
  printf ("\n\nProcessor: In-Order THROUGHPUT-ONLY (No dependence control), PIPE_WIDTH= %d\n", P->PIPE_width);
  printf ("CPI: %f   IPC: %f\n\n", ((float) CycleCount) / T->ICount, ((float) T->ICount) / CycleCount); 
}



///////////////////////////////////////////////////////////7

void sim_PIPELINE (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC, classID, PIPE_avail;
  PIPE *PP = PIPE_init (T);

  if (argc>1) { P->PIPE_width = atoll(argv[1]); }

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
    PIPE_avail= P->PIPE_width;
    while (PIPE_avail) {
      PC = Thread_getPC (T);
      classID = Thread_getClassID ( T, PC );
      if ( Processor_checkResource ( P, classID ) && PIPE_check (PP, PC, CYCLE)) {
        PIPE_setFinished (PP, PC, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC ) ));
        Processor_consumeResource ( P, classID );
        output ( P, T, PC );
        Thread_next (T);
      } else
        output (0, 0, 0);
      PIPE_avail--;
    }
    printf("\n");
  }
  printf ("\n\nProcessor: In-Order PIPELINE Width= %d\n", P->PIPE_width);
  printf ("CPI: %f   IPC: %f\n\n", ((float) CycleCount) / T->ICount, ((float) T->ICount) / CycleCount); 
}


///////////////////////////////////////////////////////////7

void sim_PIPELINE_MT2 (int argc, char **argv, Processor *P, Thread *T0, unsigned CycleCount) {

  unsigned CYCLE = 0;
  int   NO_ISSUE=0, PC, classID, policy=0, PIPE_avail;
  Thread * T1= Thread_dup (T0, "T1");
  PIPE *PP0 = PIPE_init (T0);
  PIPE *PP1 = PIPE_init (T1);
  PIPE *PP, *PPmain;

  if (argc>1) { P->PIPE_width = atoll(argv[1]); }
  if (argc>2) { policy  = atoll(argv[2]); }

  PP = PPmain = PP0;  // Thread 0 starts with priority

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
    PIPE_avail= P->PIPE_width;
    if (policy==1)        // Last thread executing gains priority
      PP = PP;  // do not change priority
    else if (policy==2) // Each cycle priority swaps
    {
      if (PPmain==PP0) PPmain=PP1;   // switch threads
      else             PPmain=PP0;
      PP = PPmain;
    }
    else    // Thread 0 always has priority
      PP = PP0;       // thread 0 has priority

    NO_ISSUE = 0; // init exit condition
    while (PIPE_avail && NO_ISSUE < 2) {
      PC = Thread_getPC (PP->T);
      classID = Thread_getClassID ( PP->T, PC );
      if ( Processor_checkResource ( P, classID ) && PIPE_check (PP, PC, CYCLE)) {
        PIPE_setFinished (PP, PC, CYCLE + Processor_getLatency ( P, Thread_getOpID( PP->T, PC ) ));
        Processor_consumeResource ( P, classID );
        output_thread ( P, PP->T, PC );
        Thread_next (PP->T);
        PIPE_avail--;
        NO_ISSUE = 0; // init exit condition
      } else
        NO_ISSUE++;   // avoid deadlock situation

      if (PP==PP0) PP=PP1;   // switch threads
      else         PP=PP0;
    }
    while (PIPE_avail) {
      output_thread (0, 0, 0);
      PIPE_avail--;
    }
    printf("\n");
  }
  printf ("\n\nProcessor: In-Order PIPELINE(%d) x 2 H/W Threads (", P->PIPE_width);
  switch (policy) {
    case 0: printf("Thread 0 always first - then round-robin"); break;
    case 1: printf("Last thread executing always first - then round-robin"); break;
    case 2: printf("Swap first thread - then round-robin"); break;
  }
  printf (")\n");
  printf ("CPI: %f   IPC: %f  T0-ICount: %d  T1-ICount: %d\n\n", 
             ((float) CycleCount) / (T0->ICount+T1->ICount),
             ((float) T0->ICount + T1->ICount) / CycleCount,
             T0->ICount, T1->ICount); 
}



///////////////////////////////////////////////////////////7

void sim_PIPELINE_MT4 (int argc, char **argv, Processor *P, Thread *T0, unsigned CycleCount) 
{
  Thread * T1= Thread_dup (T0, "T1");
  Thread * T2= Thread_dup (T0, "T2");
  Thread * T3= Thread_dup (T0, "T3");
  PIPE *PP0 = PIPE_init (T0);
  PIPE *PP1 = PIPE_init (T1);
  PIPE *PP2 = PIPE_init (T2);
  PIPE *PP3 = PIPE_init (T3);
  PIPE *PP;
  unsigned CYCLE = 0;
  int NO_ISSUE=0, turn=0; // thread 0 starts with priority
  int PC, classID, PIPE_avail;
  int seed = 0;

  if (argc>1) { P->PIPE_width = atoll(argv[1]); }
  if (argc>2) { seed  = atoll(argv[2]); }
  srand(seed);
  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    Processor_reset( P );
    PIPE_avail= P->PIPE_width;
    // round-robin priority: do nothing
    // random priority at the beginning, then round-robin
    turn = rand()%4;
    switch (turn) {
      case 0: PP=PP0; break;
      case 1: PP=PP1; break;
      case 2: PP=PP2; break;
      case 3: PP=PP3; break;
    }
    NO_ISSUE = 0; // init exit condition
    while (PIPE_avail && NO_ISSUE < 4) {
      PC = Thread_getPC (PP->T);
      classID = Thread_getClassID ( PP->T, PC );
      if ( Processor_checkResource ( P, classID ) && PIPE_check (PP, PC, CYCLE)) {
        PIPE_setFinished (PP, PC, CYCLE + Processor_getLatency ( P, Thread_getOpID( PP->T, PC ) ));
        Processor_consumeResource ( P, classID );
        output_thread ( P, PP->T, PC );
        Thread_next (PP->T);
        PIPE_avail--;
        NO_ISSUE = 0; // init exit condition
      } else
        NO_ISSUE++;   // avoid deadilock situation

      // Round-Robin policy for next turns
      turn++; if (turn>3) turn=0;
      switch (turn) {
        case 0: PP=PP0; break;
        case 1: PP=PP1; break;
        case 2: PP=PP2; break;
        case 3: PP=PP3; break;
      }
    }
    while (PIPE_avail) {
      output_thread (0, 0, 0);
      PIPE_avail--;
    }
    printf("\n");
  }
  printf ("\n\nProcessor: In-Order PIPELINE(%d) x 4 H/W Threads (random policy each cycle, then round-robin)\n", 
          P->PIPE_width);
  printf ("CPI: %f   IPC: %f  T0: %d  T1: %d  T2: %d  T3: %d\n\n", 
              ((float) CycleCount) / (T0->ICount+T1->ICount+T2->ICount+T3->ICount),
              ((float) T0->ICount+T1->ICount+T2->ICount+T3->ICount) / CycleCount,
              T0->ICount, T1->ICount, T2->ICount, T3->ICount); 
}


///////////////////////////////////////////////////////////7

void sim_PIPE_ROB (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC, Pos, i, ROB_size=8, ROB_rate=1;
  ROB *R;
  
  if (argc>1) { ROB_size = atoll(argv[1]); }
  if (argc>2) { ROB_rate = atoll(argv[2]); }

  R = ROB_init ( T, ROB_size);

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    ROB_retire ( R, ROB_rate, CYCLE);  // ROB retire width
    ROB_insert ( R, ROB_rate);         // ROB insert width
    for (i=0; i<ROB_rate; i++) {
      if ( (Pos = ROB_getReady ( R, CYCLE )) >= 0 ) {
        PC = ROB_getPC (R, Pos);
        output ( P, T, PC );
        ROB_setFinished ( R, Pos, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC )));
      }
      else {
        output ( 0, 0, 0 );
      }
    }
#ifdef DEBUG
    ROB_dump ( R );
#endif
    printf("\n");
  }
  printf("\n\nPROCESSOR: Dynamic Execution Reordering, Full PIPE, ROB size: %d, Issue/Retire rate: %d\n", 
         ROB_size, ROB_rate);
  printf ("CPI: %f   IPC: %f\n\n", ((float) CycleCount) / T->ICount, ((float) T->ICount) / CycleCount); 
}



///////////////////////// ROB + Full Pipeline ///////////

void sim_PIPE_ROB_thr (int argc, char **argv, Processor *P, Thread *T, unsigned CycleCount) {
  unsigned CYCLE = 0;
  int PC, classID, Pos, i, PIPE_avail, ROB_size=8, ROB_rate=1;
  ROB *R;
  
  if (argc>1) { P->PIPE_width = atoll(argv[1]); }
  if (argc>2) { ROB_size = atoll(argv[2]); }
  if (argc>3) { ROB_rate = atoll(argv[3]); }

  R = ROB_init ( T, ROB_size);

  for (; CYCLE < CycleCount; CYCLE++) { 
    printf("%3d  ", CYCLE);
    ROB_retire ( R, ROB_rate, CYCLE);  // ROB retire width
    ROB_insert ( R, ROB_rate);         // ROB insert width
    Processor_reset( P );
    PIPE_avail = P->PIPE_width;
    Pos = ROB_getHead (R);

    while (PIPE_avail && (Pos = ROB_getReady_Avail ( R, Pos, P, CYCLE )) >= 0 ) {
      PC = ROB_getPC (R, Pos);
      output ( P, T, PC );
      ROB_setFinished ( R, Pos, CYCLE + Processor_getLatency ( P, Thread_getOpID( T, PC )));
      PIPE_avail--;
    }
    for (i=0; i<PIPE_avail; i++)
      output ( 0, 0, 0 );
#ifdef DEBUG
    ROB_dump ( R );
#endif
    printf("\n");
  }
  printf("\n\nPROCESSOR: Dynamic Execution Reordering, Full PIPE(%d), ROB size: %d, Issue/Retire rate: %d\n", 
         P->PIPE_width, ROB_size, ROB_rate);
  printf ("CPI: %f   IPC: %f\n\n", ((float) CycleCount) / T->ICount, ((float) T->ICount) / CycleCount); 
}

