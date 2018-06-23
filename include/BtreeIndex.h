
#ifndef _B_TREE_INDEX_H_
#define _B_TREE_INDEX_H_

#define INDEXFILENAME "BtreeIndex.dat"
#define NODESIZE 116
#define INDEXHEADERSIZE 13
#define TREEORDER 10
#define REMOVE_FROM_DISK 1
#define REMOVE_FROM_INDEX 0

/*
*Definicao do conteudo armazenado por uma chave da arvore: codigoINEP e
*								rrn do registro no arquivo de dados
*/
typedef struct{
	int codINEP;
	int rrn;
} treekey;

/*
*Definicao da estrutura de uma pagina(nodo) da arvore B
*/
typedef struct{
	int keycount;
	treekey key[TREEORDER-1];
	int child[TREEORDER];
	int pageNum;
} btpage;

//define-se a estrutura do buffer para ser reconhecida como um parametro valido
typedef struct buf bufferpool;

/*
*Cria um novo arquivo de indice(arvore B) com raiz 0, 0 paginas e altura 0
*/
int createBtreeIndexFile();

/*
*Faz a insercao de uma nova chave na arvore.
*
*\param bufferpool *buffer : buffer utilizado para fazer a insercao, minimizando acessos a disco
*\param int key, rrn : dados da chave que sera inserida
*/
int insertKeyToIndex(bufferpool *buffer, int key, int rrn);

/*
*Imprime a arvore B fazendo acesso em profundidade
*/
int printBtree();

/*
*Cria uma nova pagina de disco(no da arvore) com 0 chaves
*/
btpage* createPage();

/*
*Imprime os dados de uma pagina de disco
*/
void printPage(btpage *page);

/*
* Faz a busca por uma chave da arvore.
*
*\param int codINEP : chave a ser buscada
*\return int : quando encontrada a chave, sera o RRN do registro de dados. Senao, sera -1.
*/
int BtreeSearch(int codINEP);

/*
*Faz a remocao de uma pagina da arvore. Permite a opcao de remover o registro tambem do arquivo de dados
*\param int codINEP : chave a ser removida
*\param unsigned char removeType : 1 para remover tambem do arquivo de dados, 0 para remover apenas do indice
*/
int BtreeRemove(int codINEP, unsigned char removeType);

#endif






