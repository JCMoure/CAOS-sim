8
-3, 0, 0, 5
-1, 0, 0, 4
-5, 0, 0, 5
-2,-1, 0, 3
-1,-7, 0, 6
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
