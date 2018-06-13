#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arquivos.h"
#include "bufferNRU.h"
#include "BtreeIndex.h"
#include <string.h>

//cria um novo arquivo de indice contendo apenas a raiz com 0 chaves
int createBtreeIndexFile(){

	FILE *indexFile = fopen(INDEXFILENAME, "wb+");

	unsigned char zero[INDEXHEADERSIZE] ={0};

	if(!indexFile) return 0;

	//status = 0, raiz = 0, altura = 0
	fwrite(zero, 1, INDEXHEADERSIZE, indexFile);

	setStatus(indexFile, 1);
	fclose(indexFile);
	return 1;
}

btpage *createPage(){

	btpage *page = (btpage*)calloc(1, sizeof(btpage));
	//numero de chaves 0 e todos os ponteiros com -1
	page->child[0] = -1;
	for(int i=0; i<TREEORDER; i++)
		page->child[i] = -1;

	return page;
}

int insertKey(bufferpool *buffer, btpage *root, int insertPos, int *ptr, int *key, int *rrn){


	if(root->keycount < TREEORDER-1){	//nao ocorre split
		//shifta elementos ate liberar a posicao de insercao
		for(int i = root->keycount; i > insertPos ; i--){
			root->key[i].codINEP = root->key[i-1].codINEP;
			root->key[i].rrn = root->key[i-1].rrn;
			root->child[i+1] = root->child[i];
		}
		root->key[insertPos].codINEP = *key;
		root->key[insertPos].rrn = *rrn;
		root->child[insertPos+1] = *ptr;
		root->keycount++;

		*ptr = -1;
		return 0;
	}
	//faz insercao com split

	//criando nova pagina do split
	btpage *newPage = createPage();
	//a antiga fica com metade
	root->keycount = TREEORDER/2;
	//a nova fica com metade menos a promocao
	newPage->keycount = TREEORDER/2 -1;

	if(insertPos < TREEORDER/2){//vai inserir no nodo antigo

		//numero da pagina criada
		newPage->pageNum = ++buffer->totalPages;

		//copia para a pagina nova os dados da segunda metade
		for(int i = 0; i < TREEORDER/2 -1; i++){//4 elementos copiados(5,6,7,8)

			newPage->key[i].codINEP = root->key[ TREEORDER/2 + i].codINEP;
			newPage->key[i].rrn = root->key[TREEORDER/2 + i].rrn;
			newPage->child[i] = root->child[TREEORDER/2 + i];
		}
		//falta o ultimo ponteiro
		newPage->child[TREEORDER/2 - 1] = root->child[TREEORDER-1];


		//shifta elementos da primeira metade ate liberar a posicao de insercao
		for(int i = TREEORDER/2; i > insertPos; i--){
			root->key[i].codINEP = root->key[i-1].codINEP;
			root->key[i].rrn = root->key[i-1].rrn;
			root->child[i+1] = root->child[i];
		}
		//insere
		root->key[insertPos].codINEP = *key;
		root->key[insertPos].rrn = *rrn;
		root->child[insertPos+1] = *ptr;

		//gera nodo promovido
		*ptr = newPage->pageNum;
		*key = root->key[TREEORDER/2].codINEP;
		*rrn = root->key[TREEORDER/2].rrn;


	}
	else{//a insercao ocorrera no nodo criado

		//para a primeira metade ja esta certa, pois basta atualizar as quantidades

		newPage->pageNum = ++buffer->totalPages;
		//definindo dados da pagina promovida
		if(insertPos != TREEORDER/2){	//se forem iguais, a chave promovida eh a mesma chave passada
			//senao, sera a primeira chave de root

			//vai copiar ate a posicao de insercao
				//o ponteiro esquerdo sempre se mantem
			newPage->child[0] = root->child[TREEORDER/2 + 1];
			for(int i = 0; i < insertPos - TREEORDER/2 -1; i++){
				newPage->key[i].codINEP = root->key[TREEORDER/2 + 1 + i].codINEP;
				newPage->key[i].rrn = root->key[TREEORDER/2 + 1 + i].rrn;
				newPage->child[i+1] = root->child[TREEORDER/2 + 2 + i];
			}
			//inseir dados
			newPage->key[insertPos - TREEORDER/2 -1].codINEP = *key;
			newPage->key[insertPos - TREEORDER/2 -1].rrn = *rrn;
			newPage->child[	insertPos - TREEORDER/2 ] = *ptr;

			//atualiza ponteiro promovido
			*ptr = newPage->pageNum;

			//terminar de copiar
			for(int i = insertPos - TREEORDER/2; i < TREEORDER/2 -1; i++){
				newPage->key[i].codINEP = root->key[TREEORDER/2 + i].codINEP;
				newPage->key[i].rrn = root->key[TREEORDER/2 + i].rrn;
				newPage->child[i] = root->child[TREEORDER/2 + 1 + i];
			}

			//passando dados da chave promovida
			*key = root->key[TREEORDER/2].codINEP;
			*rrn = root->key[TREEORDER/2].rrn;
		}
		else{//basta escrever as proximas 4 chaves e 5 ponteiros

			//o ponteiro da insercao vira o primeiro da esquerda
			newPage->child[0] = *ptr;
			//atualiza ponteiro propagado
			*ptr = newPage->pageNum;

			//copiando as chaves e ponteiros
			for( int i = 0; i < TREEORDER/2 -1; i++){
				newPage->key[i].codINEP = root->key[TREEORDER/2 + i].codINEP;
				newPage->key[i].rrn = root->key[TREEORDER/2 + i].rrn;
				newPage->child[i+1] = root->child[TREEORDER/2 + i + 1];
			}
		}
	}

	if( !insertBuffer(buffer, newPage, MODIFIED) ) return -1; //erro de leitura do arquivo

	return 1;
}

int findInsertPos(bufferpool* buffer, btpage *root, int *ptrRead, int *key, int *rrn, int* hit, int* fault){

	int insertPos = 0, keyRead, promoted = 0, pageNumMem = root->pageNum;

	unsigned char halt = 0;

	while(insertPos < root->keycount && !halt){
		//vai comparar as chaves para achar posicao da suposta insercao
		if(*key < root->key[insertPos].codINEP)
			halt = 1;
		else
			insertPos++;
	}

	// olha o ponteiro se nao for a primeira chave da pagina(pagina vazia->primeira chave da arvore)
	*ptrRead = ( root->keycount != 0 ) ? root->child[insertPos] : -1;

	if( *ptrRead == -1 )//eh no folha, entao vai inserir nele
		return insertKey(buffer, root, insertPos, ptrRead, key, rrn);

	else	//chama novamente pegando como raiz o ponteiro lido
		promoted = findInsertPos(buffer, searchPage(buffer, *ptrRead, ACCESSED, hit, fault), ptrRead, key, rrn, hit, fault);

	if( promoted < 0 ) return -1;	//erro durante isercao

	//volta da recursao. Promoted indica se houve split, ptrRead sera modificado para o nmr da pagina que propagou
	if( promoted ){// A posicao ja esta definida eh a mesma definida no while.
							//os parametros key e rrn ja foram alterados durante o split
		root = searchPage(buffer, pageNumMem, MODIFIED, hit, fault);//pode ter sido liberada durante uma insercao anterior
		return insertKey(buffer, root, insertPos, ptrRead, key, rrn);
	}
	else
		return 0;
}

int insertKeyToIndex(bufferpool *buffer, int key, int rrn, int* hit, int* fault){

	int ptr = -1, promoted = -1;
	btpage *newPage;
	//lendo a raiz do buffer
	btpage *root = getRootPage(buffer);

	//busca posicao e insere.
	//Parametros passados como ponteiros serao alterados para a promocao
	promoted = findInsertPos(buffer, root, &ptr, &key, &rrn, hit, fault);

	if(promoted < 0) return 0;	//erro na insercao


	if( promoted ){	//houve promocao para gerar uma nova raiz
		//cria nodo, atribui valores de ptr, key e rrn que foram atualizados no split
		newPage = createPage();
		newPage->pageNum = ++buffer->totalPages;
		newPage->keycount = 1;
		newPage->key[0].codINEP = key;
		newPage->key[0].rrn = rrn;
		newPage->child[0] = root->pageNum;
		newPage->child[1] = ptr;

		//salva raiz atual no disco disco e coloca a nova no buffer
		if(!setRootPage(buffer, newPage))
			return 0;
		buffer->treeHeight++;
	}

	return 1;
}

void printRec(FILE *indexFile, int root){
	printf("PAGINA %3d.", root);
	fseek(indexFile, INDEXHEADERSIZE + root*NODESIZE, SEEK_SET);
	int keyAmnt;

	fread(&keyAmnt, sizeof(int), 1, indexFile);
	printf(" Contem %d chaves: ", keyAmnt);
	int son, key, rrn, ptr;

	fread(&ptr, sizeof(int), 1, indexFile);
	printf("%3d ", ptr);

	for(int i=0; i<keyAmnt; i++){
		fread(&key, sizeof(int), 1, indexFile);
		fread(&rrn, sizeof(int), 1, indexFile);
		fread(&ptr, sizeof(int), 1, indexFile);
		printf("|%d %4d %3d| ", key, rrn, ptr);
	}

	long pos;
	printf("\n");
	fseek(indexFile, INDEXHEADERSIZE + root*NODESIZE + sizeof(int), SEEK_SET);
	fread(&son, sizeof(int), 1, indexFile);
	pos = ftell(indexFile);
	if(son != -1)
		printRec(indexFile, son);
	fseek(indexFile, pos , SEEK_SET);
	for(int i = 0; i < keyAmnt; i++){
		fseek(indexFile, 2*sizeof(int), SEEK_CUR);
		fread(&son, sizeof(int), 1, indexFile);
		pos = ftell(indexFile);
		if(son != -1)
			printRec(indexFile, son);

		fseek(indexFile, pos, SEEK_SET);
	}
}

int printBtree(){

	FILE *indexFile = fopen(INDEXFILENAME, "r");
	if( indexFile == NULL || isCorruptedFile(indexFile) )
		return 0;
	setStatus(indexFile, 0);
	fseek(indexFile, 1, SEEK_SET);
	int root, altura, ultimoRRN;
	fread(&root, sizeof(int), 1, indexFile);
	fread(&altura, sizeof(int), 1, indexFile);
	fread(&ultimoRRN, sizeof(int), 1, indexFile);
	printf("raiz: %d   altura: %d   ultimo RRN: %d\n", root, altura, ultimoRRN);
	printRec(indexFile, root);

	setStatus(indexFile, 0);
	fclose(indexFile);
}

void printPage(btpage *page){
	printf("\npage %d\n keyAmnt: %d -> %d|",page->pageNum, page->keycount, page->child[0]);

	for (int i=0; i<page->keycount; i++){
		printf("%d|%d|%d| ", page->key[i].codINEP, page->key[i].rrn, page->child[i+1]);
	}
	printf("\n\n");


}
