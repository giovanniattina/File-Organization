#ifndef _BUFFERNRU_H_
#define _BUFFERNRU_H_

#include "BtreeIndex.h"

#define BUFFERSIZE 5
#define UNUSED 0
#define MODIFIED 1
#define ACCESSED 2
#define MOD_AND_ACC 3

/*
* Estrutura utilizada para o bufferpool 
*/
typedef struct buf{
	btpage *page[BUFFERSIZE];
	int pageNums[BUFFERSIZE];
	char changed[BUFFERSIZE];
	int totalPages, treeHeight;
	int hit, fault;
} bufferpool;

/*
* Cria um novo bufferpool em memoria primaria, com todas as posicoes de armazenamento vazias
*/
bufferpool* createBuffer();

/*
*
* Faz insercao de uma nova pagina no buffer utilizando a politica de substituicao NRU.
* Caso alguma pagina modificada seja removida, sera feito um acesso ao disco para grava-la.
*/
int insertBuffer(bufferpool *buffer, btpage *page, int changedVal);

/*
* Encontra a proxima posicao do buffer para isercao. A politica NRU eh aplicada aqui.
*/
int findBufferPos(bufferpool *buffer, int pgNum);

/*
* Imprime o conteudo do buffer, incluindo cada pagina contida nele
*/
void printBuffer(bufferpool *buffer);

/*
* Salva todas as paginas modificadas em disco. Libera toda a memoria alocada para o buffer.
*/
int saveAllPages(bufferpool *buffer);

/*
*Salva em disco a pagina passada
*/
int savePage(bufferpool *buffer, btpage *page);

/*
* Ira buscar no buffer pela pagina contendo o numero(rrn) passado. 
* Caso ela nao esteja no buffer, a pagina eh recuperada do disco e inserida no buffer.
*
* \return btPage* : a pagina de disco com o numero passado.
*/
btpage* searchPage(bufferpool *buffer, int pageNum, int changedVal);

/*
* Retorna a pagina da raiz armazenada no buffer
*/
btpage* getRootPage(bufferpool *buffer);

/*
* Define a pagina passada como a nova raiz do buffer.
* Fara 1 acesso ao disco para escrever a raiz antiga caso tenha sido modificada
*/
int setRootPage(bufferpool *buffer, btpage *newPage);

/*
* Acessa a arvore em largura ate preencher todas as posicos vazias do buffer ou 
* ate chegar ao final do arquivo de indice
*/
bufferpool* loadBuffer();

/*
* Remove uma pagina permanentemente da arvore. O dado nao eh salvo em disco.
* O dado em disco nao eh apagado, mas estara sujeito a ser sobrescrito durante novas insercoes
*/
int deletePage(bufferpool *buffer, int pgNum);

/*
* Gera um output em arquivo de quantos hits e faults foram computados pelo buffer
*/
int writeBufferInfo(bufferpool *buffer);

int BtreeSearchCode(int searchKey);

int searchTheKey(bufferpool *myBuffer, int searchKey, int pageNum);

int searchRoot(bufferpool *myBuffer, int searchKey);
#endif
