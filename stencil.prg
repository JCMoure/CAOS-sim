11
 0, 0, 0, 8
-4, 0, 0, 9
-5, 0, 0, 9
-3, 0, 0, 6
-2,-3, 0, 5
-1, 0, 0, 6
-1,-3, 0, 5
-10,-1, 0, 10
-11, 0, 0, 1
-1, 0, 0, 2
-1, 0, 0, 0

 // Total number of instructions
   // FMOV  (no dependence)
   // LOAD  (depends on IADD)
   // LOAD  (depends on IADD)
   // FMUL  (depends on FMOV)
   // FADD  (depends on 1st LOAD & 2nd LOAD)
   // FMUL  (depends on FADD)
   // FADD  (depends on 1st FMUL & 2nd FMUL)
   // STORE (depends on 2nd FADD & IADD)
   // IADD  (depends on itself)
   // ICMP  (depends on IADD)
   // BRN   (depends on ICMP)
