// Defines the number of cycles at the end of the simulated execution 
//  that are considered for visualization
#define OUTPUT_RANGE 30

////////////////// PROCESSOR ///////////////////

typedef struct OP {  // data structure of operations
  char * name;
  int    classID;
  int    latency;
} Operation;

typedef struct CL {  // data structure of operation classes
  char * name;
  int    throughput; // potential throughput (or issue rate per cycle)
  int    available;  // issue slots available in current cycle
} Class;

typedef struct PR {  // data structure for processor
  int PIPE_width;
  int ROB_size;
  int Num_Classes;
  Class     *Classes;
  int Num_Operations;
  Operation *Ops;  
} Processor;

void Processor_reset           ( Processor *P );
int  Processor_getLatency      ( Processor *P, int OpID );
int  Processor_getClassID      ( Processor *P, int OpID );
int  Processor_checkResource   ( Processor *P, int classID );
void Processor_consumeResource ( Processor *P, int classID );

void Processor_dump (Processor *P);
Processor * Processor_read (char *filename);


////////////////// THREAD /////////////////////////

typedef struct I { // data structure of a single instruction
  int source1;     // identifies 1st instruction providing input data
  int source2;     // identifies 2nd instr. ..
  int source3;     // identifies 3rd instr. ..
  int operationID; // operation
  int classID;     // operation class
} Instruction;

typedef struct THR {
  int          PC;            // Program Counter: points to current instruction
  int          N_Instr;       // Number of instructions in program
  int          ICount;        // Counts executed instructions in this thread
  char        *name;          // Thread identifier
  Instruction *Program;       // Actual program: instruction sequence
} Thread;


Thread * Thread_read( char *Filename, char *ThreadName, Processor *P );
Thread * Thread_dup ( Thread *Tin, char *name );

int  Thread_getPC             ( Thread *T );
int  Thread_getCurrentOpID    ( Thread *T );
int  Thread_getCurrentClassID ( Thread *T );
int  Thread_getOpID    (Thread *T, int PC);  // get operation ID of current instruction (PC)
int  Thread_getClassID (Thread *T, int PC);  // get class     ID of current instruction (PC)
int  Thread_getNext    (Thread *T, int PC);  // compute nextPC from intput PC
void Thread_next       (Thread *T);          // advance current PC of thread


////// Pipeline Control: controls pipeline dependences 
typedef struct PP {
  int       Sz;            // Maximum pipeline depth
  Thread   *T;
  unsigned *whenFinished;  // cycle when each instruction finishes
} PIPE;

PIPE  * PIPE_init ( Thread *T );
void    PIPE_setFinished ( PIPE *P, int Pos, unsigned Cycle );
int     PIPE_check       ( PIPE *P, int Pos, unsigned Cycle );

////// ROB:  Reorder Buffer used for controlling dynamic execution reordering

typedef struct RB {  // data structure of a ReOrder Buffer
  int       Head;
  int       Tail;
  int       Size;
  int       n;
  Thread   *T;             // Thread associated to ROB
  unsigned *whenFinished;  // state of instructions in reorder buffer (0: not issued)
} ROB;

ROB *ROB_init   ( Thread *T, int Sz );
void ROB_insert ( ROB *R, int k ); 
void ROB_retire ( ROB *R, int k, unsigned currentCycle );
int  ROB_getPC  ( ROB *R, int Pos );
int  ROB_getHead( ROB *R );
int  ROB_getReady       ( ROB *R, unsigned CYCLE );
int  ROB_getReady_Avail ( ROB *R, int Pinit, Processor *P, unsigned CYCLE );
void ROB_setFinished    ( ROB *R, int Pos, unsigned CYCLE );
void ROB_dump   ( ROB *R );
