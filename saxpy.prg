8
-3, 0, 0, 9 
-1, 0, 0, 6
-5, 0, 0, 9
-2,-1, 0, 5
-1,-7, 0, 10
-8, 0, 0, 1
-1, 0, 0, 2
-1, 0, 0, 0

 // Total number of instructions
   // LOAD  (depends on IADD)
   // FMUL  (depends on LOAD)
   // LOAD  (depends on IADD)
   // FADD  (depends on FMUL & LOAD)
   // STORE (depends on FADD & IADD)
   // IADD  (depends on itself)
   // ICMP  (depends on IADD)
   // BRN   (depends on ICMP)
