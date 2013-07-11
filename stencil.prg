11
 0, 0, 0, 5, 3
-1, 0, 0, 4, 2
-5, 0, 0, 5, 3
-2,-1, 0, 3, 2
-1,-7, 0, 6, 3
-8, 0, 0, 1, 1
-1, 0, 0, 2, 1
-1, 0, 0, 0, 0

 // Total number of instructions
   // FMOV 5-2  (no dependence)
   // LOAD  (depends on IADD)
   // FMUL  (depends on LOAD)
   // LOAD  (depends on IADD)
   // FADD  (depends on FMUL & LOAD)
   // STORE (depends on FADD & IADD)
   // IADD  (depends on itself)
   // ICMP  (depends on IADD)
   // BRN   (depends on ICMP)
