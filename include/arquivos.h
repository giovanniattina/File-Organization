
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

/*
*Abre e faz leitura de um arquivo .csv
*\param : char* nome do arquivo
*/
int readFile(char*);

/*
*
*/
char *getField(FILE*);

/*
*Imprime todos os registros armazenados em disco
*/
int showAll();
void printReg(registro *reg);

/*
*Acessa o arquivo para recuperar o registro por RRN 
*\param int RRN : RRN que sera recuperado caso nao esteja removido
*/
int findRRN(int RRN);

/*
*Faz a remocao logica do registro de RRN passado
*/
int removeReg(int RRN);

/*
*Faz insercao de um no registro no arquivo de dados
*/
int insertReg(int codInep, char *dataAtiv, char *uf, char *nomeEscola, char *municipio, char *prestadora);

/*
*Busca sequencialmente(registro registro) pelo dado contendo o valor passado no campo passado
*/
int search(char  *campName, char *value);

int checkToPrint(registro *reg, char *camp, char *value);
char *stripField(registro *reg, char *camp);

/*
*Atualiza os campos do registro com RRN informado
*/
int updateReg(int RNN, int campo1, char *campo2, char *campo3, char *campo4, char *campo5, char *campo6);

/*
*
*Faz a compactacao eficiente recuperando o espaco dos registros logicamente removidos
*/
int compact();
int* showStack(int* sizeStack);

/*
*Atualiza status do arquivo para o valor passado
*/
void setStatus(FILE *binFile, unsigned char status);

/*
*Verifica status do arquivo, informando se ele esta corrompido ou nao
*/
unsigned char isCorruptedFile(FILE *binFile);

#endif
