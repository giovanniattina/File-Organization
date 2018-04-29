#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arquivos.h>

#define REGSIZE 87
#define REGCABSIZE 5


/** \brief Funcao que separa a linha lida do arquivo CSV em campos.
  *
  * \param *fileName - Arquivo a ser aberto.
  *
  * \return field - Campo lido
  */
char *getField(FILE *fileName){
  char c = 0;
  int contador = 0;
  char *field = NULL;

  do{ //percorre toda a linha até achar o delimitador
    c = fgetc(fileName);
    field = (char *)realloc(field, sizeof(char)*(contador+1));
    field[contador++] = c;
  } while(c != 10 && c != ';');

  field[contador-1] = '\0';

  return field; //retorna o campo para ser salvo
}

/** \brief Funcao que le o arquivo CSV.
  *
  * \param *fileName - Arquivo a ser aberto.
  *
  * \return numReg - Quantidade de registros.
  */
int readFile(char *fileName){
  FILE *csvFile = fopen(fileName, "r");
  FILE *binFile = fopen("data.dat", "wb");
  int size, numReg = 0;
  char zero = '0';

  if(!csvFile){
    printf("Falha no carregamento do arquivo.\n");
  } else {
    fseek(csvFile, 0, SEEK_END); // encontra o tamanho do arquivo CSV
    size = ftell(csvFile);
    rewind(csvFile);

    while(ftell(csvFile) != size) { // percorre todo o arquivo CSV e atribui os campos as variaveis para salvar
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

      // Salva todos os campos no arquivo binario
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

      for (size_t i = 0; i < REGSIZE - tamReg; i++) { // preenche os bytes que estão sobrando até chegar no tamanho do registro
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

/** \brief Funcao que le o arquivo binario e imprime todos os registros.
  *
  */
void showAll(){
  FILE *binFile = fopen("data.dat", "rb");
  int size;

  fseek(binFile, 0, SEEK_END);
	size = ftell(binFile);
	rewind(binFile);
  int tamReg = 0;

  while(ftell(binFile) != size) {
    // aloca as memórias para cada campo
    registro *reg = malloc(sizeof(registro));
    reg->nomeEscola = NULL;
    reg->municipio = NULL;
    reg->prestadora = NULL;
    reg->dataAtiv = malloc(10*sizeof(char));
    reg->uf = malloc(2*sizeof(char));

    // Le os dados do arquivo binario e salva nas variaveis
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
    reg->prestadora = realloc(reg->prestadora, reg->tam_prestadora);
    fread(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

    printReg(reg);

    free(reg);

    tamReg = 28 + reg->tam_nomeEscola + reg->tam_municipio + reg->tam_prestadora;
    fseek(binFile, REGSIZE - tamReg, SEEK_CUR);
  }

  fclose(binFile);
}

/** \brief Funcao que busca um determinado registro no arquivo binario pelo RRN e o imprime.
  *
  * \param RRN - O numero RRN a ser procurado.
  */
void findRRN(int RRN){
  FILE *binFile = fopen("data.dat", "rb");
  int size;

  // busca o byte offset do registro procurado no arquivo
  fseek(binFile, RRN * REGSIZE, SEEK_SET);

  registro *reg = malloc(sizeof(registro));
  reg->nomeEscola = NULL;
  reg->municipio = NULL;
  reg->prestadora = NULL;
  reg->dataAtiv = malloc(10*sizeof(char));
  reg->uf = malloc(2*sizeof(char));

  // Le os dados do arquivo binario e salva nas variaveis
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
  reg->prestadora = realloc(reg->prestadora, reg->tam_prestadora);
  fread(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

  // Se encontrou o registro imprime ele.
  if(reg->codINEP)
    printReg(reg);
  else
    printf("Registro inexistente.\n");

  free(reg);
}

/** \brief Funcao que imprime os campos do registro no formato especificado.
  *
  * \param *reg - O registro a ser impresso.
  */
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
