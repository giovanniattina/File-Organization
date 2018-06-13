
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
int search(char  *campName, char *value);
int checkToPrint(registro *reg, char *camp, char *value);
char *stripField(registro *reg, char *camp);
int updateReg(int RNN, int campo1, char *campo2, char *campo3, char *campo4, char *campo5, char *campo6);
int compact();
int* showStack(int* sizeStack);
void setStatus(FILE *binFile, unsigned char status);
unsigned char isCorruptedFile(FILE *binFile);

#endif
