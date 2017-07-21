Processor Simulator
CAOS Department, Engineering School, UAB, Barcelona, SPAIN

compile:
  
gcc -Ofast sim.c sim-simulators.c sim-main.c -o sim

execute:

./sim processors/proc1.c programs/saxpy.txt 30 0

./sim processors/proc1.c programs/saxpy.txt 30 1

...

