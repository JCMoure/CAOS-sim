#define NO_INPUT_INSTRUCTION ((unsigned)-1)

typedef struct I {  // data structure of a single instruction
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
  Instruction *Program;       // Actual program: instruction sequence
  int         *whenFinished;  // cycle when each instruction finishes
} Thread;

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
  int PIPE_avail;
  int CCount;          // Counts executed cycles
  int Num_Classes;
  Class     *Classes;
  int Num_Operations;
  Operation *Ops;  
} Processor;

