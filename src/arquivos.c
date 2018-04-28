#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arquivos.h>

#define REGSIZE 87
#define REGCABSIZE 5

char *getField(FILE *fileName){
  char c = 0;
  int contador = 0;
  char *field = NULL;

  do{
    c = fgetc(fileName);
    field = (char *)realloc(field, sizeof(char)*(contador+1));
    field[contador++] = c;
  } while(c != 10 && c != 11 && c != 12 && c != 13 && c != EOF && c != ';');

  field[contador-1] = '\0';

  return field;
}

int readFile(char *fileName){
  FILE *csvFile = fopen(fileName, "r");
  FILE *binFile = fopen("data.dat", "wb");
  int size, numReg = 0;
  char zero = '0';

  if(!csvFile){
    printf("Falha no carregamento do arquivo.\n");
  } else {
    fseek(csvFile, 0, SEEK_END);
    size = ftell(csvFile);
    rewind(csvFile);

    while(ftell(csvFile) != size) {
      registro *reg = malloc(sizeof(registro));

      reg->prestadora = getField(csvFile);
      reg->tam_prestadora = (int)strlen(reg->prestadora);
      reg->dataAtiv = getField(csvFile);
      reg->codINEP = atoi(getField(csvFile));
      reg->nomeEscola = getField(csvFile);
      reg->tam_nomeEscola = (int)strlen(reg->nomeEscola);
      reg->municipio = getField(csvFile);
      reg->tam_municipio = (int)strlen(reg->municipio);
      reg->uf = getField(csvFile);

      fwrite(&reg->codINEP, sizeof(int), 1, binFile);
      if (strlen(reg->dataAtiv)) {
        fwrite(reg->dataAtiv, 10*sizeof(char), 1, binFile);
      } else {
        fwrite("0000000000", 10*sizeof(char), 1, binFile);
      }
      if (strlen(reg->uf)) {
        fwrite(reg->uf, 2*sizeof(char), 1, binFile);
      } else {
        fwrite("00", 2*sizeof(char), 1, binFile);
      }
      fwrite(&reg->tam_nomeEscola, sizeof(int), 1, binFile);
      fwrite(reg->nomeEscola, reg->tam_nomeEscola * sizeof(char), 1, binFile);
      fwrite(&reg->tam_municipio, sizeof(int), 1, binFile);
      fwrite(reg->municipio, reg->tam_municipio * sizeof(char), 1, binFile);
      fwrite(&reg->tam_prestadora, sizeof(int), 1, binFile);
      fwrite(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

      int tamReg = 28 + reg->tam_prestadora + reg->tam_municipio + reg->tam_nomeEscola;

      for (size_t i = 0; i < REGSIZE - tamReg; i++) {
        fwrite(&zero, sizeof(char), 1, binFile);
      }

      numReg++;

      free(reg);
    }

    fclose(csvFile);
    fclose(binFile);

    return numReg;
  }
}

void showAll(){
  FILE *binFile = fopen("data.dat", "rb");
  int size;

  fseek(binFile, 0, SEEK_END);
	size = ftell(binFile);
	rewind(binFile);

  while(ftell(binFile) != size) {
    registro *reg = malloc(sizeof(registro));

    fread(&reg->codINEP, sizeof(int), 1, binFile);
    fread(reg->dataAtiv, 10 * sizeof(char), 1, binFile);
    fread(reg->uf, 2 * sizeof(char), 1, binFile);
    fread(&reg->tam_nomeEscola, sizeof(int), 1, binFile);
    reg->nomeEscola = realloc(reg->nomeEscola, reg->tam_nomeEscola);
    fread(reg->nomeEscola, reg->tam_nomeEscola * sizeof(char), 1, binFile);
    fread(&reg->tam_municipio, sizeof(int), 1, binFile);
    reg->municipio = realloc(reg->municipio, reg->tam_municipio);
    fread(reg->municipio, reg->tam_municipio * sizeof(char), 1, binFile);
    fread(&reg->tam_prestadora, sizeof(int), 1, binFile);
    fread(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

    printReg(reg);

    free(reg);

    int tamReg = 28 + reg->tam_prestadora + reg->tam_municipio + reg->tam_nomeEscola;
    fseek(binFile, REGSIZE - tamReg, SEEK_CUR);
  }

  fclose(binFile);
}

void findRRN(int RRN){
  FILE *binFile = fopen("data.dat", "rb");
  int size;

  fseek(binFile, RRN * REGSIZE, SEEK_SET);

  registro *reg = malloc(sizeof(registro));

  fread(&reg->codINEP, sizeof(int), 1, binFile);
  fread(reg->dataAtiv, 10 * sizeof(char), 1, binFile);
  fread(reg->uf, 2 * sizeof(char), 1, binFile);
  fread(&reg->tam_nomeEscola, sizeof(int), 1, binFile);
  fread(reg->nomeEscola, reg->tam_nomeEscola * sizeof(char), 1, binFile);
  fread(&reg->tam_municipio, sizeof(int), 1, binFile);
  fread(reg->municipio, reg->tam_municipio * sizeof(char), 1, binFile);
  fread(&reg->tam_prestadora, sizeof(int), 1, binFile);
  fread(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

  printReg(reg);

  free(reg);
}

void printReg(registro *reg){
  int i;

  printf("%d ", reg->codINEP);
  printf("%s ", reg->dataAtiv);
  printf("%s ", reg->uf);
  printf("%d ", reg->tam_nomeEscola);
  if (reg->tam_nomeEscola) {
    for(i = 0; i < reg->tam_nomeEscola; i++){
      printf("%c", reg->nomeEscola[i]);
    }
    printf(" ");
  }
  printf("%d ", reg->tam_municipio);
  if (reg->tam_municipio) {
    for(i = 0; i < reg->tam_municipio; i++){
      printf("%c", reg->municipio[i]);
    }
    printf(" ");
  }
  printf("%d ", reg->tam_prestadora);
  if (reg->tam_prestadora) {
    for(i = 0; i < reg->tam_prestadora; i++){
      printf("%c", reg->prestadora[i]);
    }
  }
  printf("\n");
}
