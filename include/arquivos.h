#ifndef _ARQUIVOS_H_
#define _ARQUIVOS_H_

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
int showAll();
void printReg(registro *reg);
int findRRN(int RRN);
int removeReg(int RRN);
int insertReg(int codInep, char *dataAtiv, char *uf, char *nomeEscola, char *municipio, char *prestadora);
#endif
