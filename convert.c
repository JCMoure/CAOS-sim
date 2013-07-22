#include <stdio.h>
  
char  OutVar[40][20];
char  Operation[40][20];
char  In1Var[40][20];
char  In2Var[40][20];
char  In3Var[40][20];
char  DUMP[121];

int findInput ( char *InputVar, int pos, int N) {
  int i = pos, j=0;
  do {
    j--;
    i = i-1;
    if (i<0) i = N-1;
  } while (pos != i && strcmp(InputVar, OutVar[i]) );
  if (!strcmp(InputVar, OutVar[i]) )
    return j;
  return 0;
} 

int convert (char *InputFilename, char *OutputFilename) {
  FILE *Fin, *Fout;
  int   i, N;

  Fin= fopen(InputFilename, "r");
  if (!Fin) return 0;

  Fout= fopen(OutputFilename, "w");
  if (!Fout) return 0;

  for (i=0; i<40; i++) {
    fscanf ( Fin, "%s = %s ( %s", OutVar[i], Operation[i], In1Var[i]);
    if (!strcmp(OutVar[i], "*")) break;  // end of file
    if (!strcmp(In1Var[i], ")")) {
      In1Var[i][0]='\0';
      In2Var[i][0]='\0';
      In3Var[i][0]='\0';
    } else {
      fscanf ( Fin, "%s", In2Var[i]);
      if (!strcmp(In2Var[i], ")")) {
        In2Var[i][0]='\0';
        In3Var[i][0]='\0';
      } else {
        fscanf ( Fin, "%s", In3Var[i]);
        if (!strcmp(In3Var[i], ")")) {
          In3Var[i][0]='\0';
        } else
        fscanf ( Fin, ")");
     }
   }
   fgets ( DUMP, 120, Fin);
  }
  N=i;
  fprintf( Fout, "%d  // Total number of instructions\n", N);
  for (i=0; i<N; i++) {
    int s1=0, s2=0, s3=0; 
    if (In1Var[i]) s1 = findInput( In1Var[i], i, N);
    if (In2Var[i]) s2 = findInput( In2Var[i], i, N);
    if (In3Var[i]) s3 = findInput( In3Var[i], i, N);
    fprintf( Fout, "%d, %d, %d, %4s    //", s1, s2, s3, Operation[i]);
    if (In1Var[i])  fprintf( Fout, "%s ", In1Var[i]);
    if (In2Var[i])  fprintf( Fout, "%s ", In2Var[i]);
    if (In3Var[i])  fprintf( Fout, "%s ", In3Var[i]);
    fprintf( Fout, "\n");
  }
  return N;
}

void main (int argc, char **argv) {
  if (argc<3) {
     printf("argumentos: InputFile[.txt] OutputFile[.prg]\n");
     return;
  }
  if (!convert(argv[1], argv[2]))
    printf("error in conversion: files not valid\n");
}
