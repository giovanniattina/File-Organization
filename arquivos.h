#ifndef _ARQUIVOS_H_
#define _ARQUIVOS_H_

typedef struct{
	unsigned char status;
	int topoPilha;
}regCabecalho;

typedef struct{
  int codINEP;
  char dataAtiv[10];
  char uf[2];
  char *nomeEscola;
  char *municipio;
  char *prestadora;
  int tam_nomeEscola;
  int tam_municipio;
  int tam_prestadora;
}registro;

void readCsvFile(char *fileName);
void printReg(registro *reg, int *tamCampos);
void parseLinha(char *linha, registro *reg, int *tamVet);
#endif
