#ifndef _ARQUIVOS_H_
#define _ARQUIVOS_H_

typedef struct{
	char status;
	int topoPilha;
}regCabecalho;

typedef struct{
  int codINEP;
  char *dataAtiv;
  char *uf;
  char *nomeEscola;
  char *municipio;
  char *prestadora;
  int tam_nomeEscola;
  int tam_municipio;
  int tam_prestadora;
}registro;

int readFile(char*);
char *getField(FILE*);
void showAll();
void printReg(registro *reg);
void findRRN(int RRN);

#endif
