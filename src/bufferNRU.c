#include <stdlib.h>
#include <stdio.h>

#include "BtreeIndex.h"
#include "arquivos.h"
#include "bufferNRU.h"
#include <time.h>

/**
 *	Function is used in the first time that Buffer Pool is create
 *  Return:
 * 			- A empity buffer with page with RRN 0 in the root 
 * 
 */
bufferpool* createBuffer(){

	bufferpool *buffer = (bufferpool*)calloc(1, sizeof(bufferpool));

	//creating empty page 0 as the root
	buffer->page[0] = createPage();
	
	//initializing remaning buffer variables
	for (int i=1; i<BUFFERSIZE; i++){
		buffer->pageNums[i] = -1;
	}
	buffer->pageNums[0] = 0;
	buffer->changed[0] = 1;//modificada

	srand( time(NULL) );

	return buffer;
}

int findBufferPos(bufferpool *buffer, int pgNum){
	
	//verifica se a pagina ja esta no buffer, retorna a posicao dela no buffer
	for (int i=1; i<BUFFERSIZE; i++){
		if(buffer->pageNums[i] == pgNum){
			buffer->pageNums[i] = -1;
			return i;
		}
	}
	//verifica se ha posicoes vazias. retorna a primeira que encontrar
	for (int i=1; i<BUFFERSIZE; i++){
		if( buffer->pageNums[i] == -1 || buffer->pageNums[i] == pgNum)
			return i;
	}

	//buffer is full: will use NRU substitution policy
	//0-> not used; 1->modified; 2->accessed: 3-> 1 and 2
	int min = 3;
	for(int i=1; i< BUFFERSIZE; i++){
		if(buffer->changed[i] < min)
			min = buffer->changed[i];
	}

	int vet[5]={0}; //vet[0] will store the amnt of used positions
	for (int i =1; i < BUFFERSIZE; i++){
		if (buffer->changed[i] == min){
			vet[vet[0] + 1] = i;
			vet[0]++;
		}
	}

	int i = 1 + rand() % vet[0];	

	return vet[ i ];
}
/**
 * 
 * Update the substitution policies value by the pass through parameter in a specific page
 * Parameters:
 * 			- buffer: Buffer-pool with the data to be change
 * 			- pagePos: page to modify the policy value.
 * 			- changedVal: new policy value
 * Return:
 * 			- 0 to show the function finalization
 *  
 */

int updateBufferUse(bufferpool *buffer, int pagePos, int changedVal){

	int changed = buffer->changed[pagePos];

	if(changed == 3 || changed * changedVal == 2)
			buffer->changed[ pagePos ] = 3;
	else
		buffer->changed[ pagePos ] = changedVal;

	return 0;
}

/**
 * Pass a page to be added in the buffer pool, with our political number
 * Parameter:
 * 			-buffer: Buffer Pool to insert the new page
 * 			-page: new page to be inserted
 * 			 - changeVal: new political value
 * Return:
 * 			- 0 if any error occur
 * 			- 1 if succeed 
 */
int insertBuffer(bufferpool *buffer, btpage *page, int changedVal){

	//finding the postion for the insertion
	int insertPos = findBufferPos(buffer, page->pageNum);
	//printf("insertPos = %d\n", insertPos);

	//if this position is not empty, the data gets written to the disk
	if( buffer->pageNums[ insertPos ] != -1){
		//printf("buffer->page[insertPos]->pageNum = %d\n", buffer->page[insertPos]->pageNum);
		if (!savePage(buffer, buffer->page[insertPos])) return 0;
	}

	buffer->changed[insertPos] = changedVal;
	//inserting the page and updating
	buffer->page[ insertPos ] = page;
	buffer->pageNums[ insertPos ] = page->pageNum;

	return 1;
}
/**
 * Search a page in the buffer pool and change the value for NRU, if the page is not in the buffer search it in the file and adds following the substitution policies
 * Parameters:
 * 				- Buffer to search the page
 * 				- pageNum: page to search
 * 				- changeVal: new political value
 * Return:
 * 			Page founded
 */
btpage* searchPage(bufferpool *buffer, int pageNum, int changedVal){
	

	//tries to find the page inside the buffer
	for (int i = 0; i < BUFFERSIZE; i++){
		if(buffer->pageNums[i] == pageNum){
			updateBufferUse(buffer, i, changedVal);
			buffer->hit++;
			return buffer->page[i];
		}
	}
	//page not found. Will retrieve from disk
	FILE *btree =fopen(INDEXFILENAME, "rb+");

	if (btree == NULL) return NULL;

	if (isCorruptedFile(btree)){
		fclose(btree);
		return NULL;
	}
	setStatus(btree, 0);

	//creating an empty page
	btpage* page = createPage();

	//reading key amount and first pointer
	fseek(btree, INDEXHEADERSIZE + NODESIZE * pageNum, SEEK_SET);
	fread(&(page->keycount), sizeof(int), 1, btree);
	fread(&(page->child[0]), sizeof(int), 1, btree);

	//reading remaining keys and pointers
	for(int i=0; i<page->keycount; i++){
		fread(&(page->key[i]), sizeof(treekey), 1, btree);
		fread(&(page->child[i+1]), sizeof(int), 1, btree);
	}

	setStatus(btree, 1);
	fclose(btree);

	page->pageNum = pageNum;
	
	insertBuffer(buffer, page, changedVal);

	buffer->fault++;
	return page;
}

/**
 * Return the root page save in the buffer pool
 * Parameters:
 * 				- buffer: Buffer Pool with the infos
 * Retun:
 * 		 - the root page
 * 
 */
btpage* getRootPage(bufferpool *buffer){
	
	buffer->hit++;
	return buffer->page[0];
}
/**
 *	Change the root page in the Buffer
 *  Parameters:
 * 				- Buffer: Buffer to work on
 * 				- newRoot: new root page
 * Return:
 * 			- 0 if any error occur
 * 			- 1 if succeed 
 */
int setRootPage(bufferpool *buffer, btpage *newRoot){
	if( !savePage(buffer, buffer->page[0]) ) return 0;

	buffer->page[0] = newRoot;
	buffer->pageNums[0] = newRoot->pageNum;
	return 1;
}
/**
 * SAve all the pages in the Buffer-Pool in the file
 * Parameters:
 * 				- Buffer: buffer pool with the pages to save
 *  Return:
 * 			- 0 if any error occur
 * 			- 1 if succeed 
*/

int saveAllPages(bufferpool *buffer){
	
	for (int i = 0; i < BUFFERSIZE; i++){
		if(buffer->pageNums[i] != -1){
			if( !savePage(buffer, buffer->page[i]) )
				return 0;
		}
	}

	FILE *bufferInfo =fopen("buffer-info.txt", "a+");
	fprintf(bufferInfo, "Page fault: %d; Page hit: %d.\n", buffer->fault, buffer->hit);
	fclose(bufferInfo);
	return 1;
}

/**
 * Save one page file in the B-Tree file, checking if the page to save is the root, so att th file header
 * Parameters:
 * 			-  buffer: Buffer if the data
 * 			- page: page to save
 * Return:
 * 			- 0 if any error occur
 * 			- 1 if succeed
 */
int savePage(bufferpool *buffer, btpage *page){
	
	FILE *btreeFile = fopen(INDEXFILENAME, "rb+");
	if (btreeFile == NULL)
		return 0;
	if( isCorruptedFile( btreeFile ) ){
		fclose( btreeFile );
		return 0;
	}
	
	setStatus(btreeFile, 0);
	if( buffer->pageNums[0] == page->pageNum ){	//will write the root, so updates the header first
		fwrite(&(page->pageNum), sizeof(int), 1, btreeFile);
		fwrite(&(buffer->treeHeight), sizeof(int), 1, btreeFile);
		fwrite(&(buffer->totalPages), sizeof(int), 1, btreeFile);
	}

	//seeking insert position
	fseek(btreeFile, INDEXHEADERSIZE + page->pageNum * NODESIZE, SEEK_SET);

	//writing amount of keys and first pointer
	fwrite(&(page->keycount), sizeof(int), 1, btreeFile);
	fwrite(&(page->child[0]), sizeof(int), 1, btreeFile);

	for (int i = 0; i < page->keycount; i++){
		fwrite(&(page->key[i]), sizeof(treekey), 1, btreeFile);
		fwrite(&(page->child[i+1]), sizeof(int), 1, btreeFile);
	}
	setStatus(btreeFile, 1);
	fclose(btreeFile);
	free(page);
	return 1;
}

void printBuffer(bufferpool* buffer){

	printf("total pages: %d, treeHeight: %d\n", buffer->totalPages, buffer->treeHeight);
	printf("pages in buffer: ");
	for (int i=0; i < BUFFERSIZE; i++)
		printf("%d ", buffer->pageNums[i]);
	printf("\n");
	printf("chaged: ");
	for (int i=0; i < BUFFERSIZE; i++)
		printf("%d ", buffer->changed[i]);
	printf("\n");
}

/*

*/
btpage* readIndexPage(int pageNum, FILE* btreeFile){
	fseek(btreeFile, INDEXHEADERSIZE + NODESIZE * pageNum, SEEK_SET);

	btpage* page = createPage();

	fread(&(page->keycount), sizeof(int), 1,btreeFile);
	fread(&(page->child[0]), sizeof(int), 1,btreeFile);
	page->pageNum = pageNum;

	for (int i = 0; i < page->keycount; i++) {
		fread(&(page->key[i]), sizeof(treekey), 1,btreeFile);
		fread(&(page->child[i+1]), sizeof(int), 1,btreeFile);
	}

	return page;
}

/**
 * Busca no buffer pela pagina passada, removendo-a
 * Parametros: buffer: Buffer para remover a pagina
 * 			   pageNum: pagina a ser removida
 * Return: 0: Se tiver alguma falha.
 * 		   1: Se ouve sucesso.
*/
int deletePage(bufferpool *buffer, int pageNum){

	for (int i = 1; i< BUFFERSIZE; i++){
		if(buffer->pageNums[i] == pageNum){
			free(buffer->page[i]);
			buffer->pageNums[i] = -1;
			return 1;
		}
	}
	return 0;
}

/**
 * Create the Buffer-Pool withe the tre pages preloades, always trying to save a page from each height of the tree
 * Returns an address for the Buffer-Pool data   
 */

bufferpool* loadBuffer(){
	
	FILE *btreeFile = fopen(INDEXFILENAME, "rb+");
	if (btreeFile == NULL)
		return NULL;
	if( isCorruptedFile( btreeFile ) ){
		fclose( btreeFile );
		return NULL;
	}

	setStatus(btreeFile, 1);

	int rootNum, bufferPos = 1, cont = 0, searchPos = 0, endLoad = 0;
	bufferpool* buffer = createBuffer();
	
	//retrieving root number, tree height and last page number
	fread(&(rootNum), sizeof(int), 1,btreeFile);
	fread(&(buffer->treeHeight), sizeof(int), 1,btreeFile);
	fread(&(buffer->totalPages), sizeof(int), 1,btreeFile);

	setStatus(btreeFile, 1);
	fclose(btreeFile);
	
	buffer->pageNums[0] = -1;//setando como posicao livre para conseguir buscar
	//set buffer position 0 to be the root
	buffer->page[0] = searchPage(buffer, rootNum, UNUSED);
	buffer->pageNums[0] = rootNum;
	buffer->pageNums[1] = -1;

	//will load 4 additional pages to the buffer
	while(!endLoad){
		//will look at the children of each filled position of the buffer starting with the root
		if(buffer->page[searchPos]->child[cont] != -1){
			searchPage(buffer, buffer->page[searchPos]->child[cont], UNUSED);
			buffer->pageNums[bufferPos++] = buffer->page[searchPos]->child[cont];
		}
		cont++;
		//checking if the last pointer was reached
		if(cont >= buffer->page[searchPos]->keycount + 1){
			searchPos++;
			cont =0;
		}
		
		//checking stop conditions: all positions filled, no more pages to access
		if(bufferPos >= BUFFERSIZE || buffer->page[searchPos] == NULL || searchPos >= BUFFERSIZE)
			endLoad = 1;
	}
	
	//desconsiderando inicializacao do buffer para contagem
	buffer->hit = 0;
	buffer->fault = 0;
	
	return buffer;
}
/**  
 *Function starts by searching the root of the tree that is saved in the buffer pool until it finds the RNN value of the required data
 * Parameters::
 *     myBuffer: Buffer Pool with preloaded data to do the search
 *     searchKey: Key you were looking for in the tree
 *     Return: 
 *      -1: if I do not find the key in the tree
 *      c.a.c: RNN in the data file
*/

int searchRoot(bufferpool *myBuffer, int searchKey){

	/*
        pos -> variable to save the position that the key is saved on the page and also to save the location of the child page pointer if you can not find the key on the page
    	rrn -> variable saves the RNN value of the data being searched in the data file
    */
	int pos = -1;
	int rrn = -1;

	//first checks to see if there is a root in the buffer
	if (myBuffer->pageNums[0] != -1){
		printf("Raiz: %d\n", myBuffer->pageNums[0]);

		//exist the root in the buffer, look for the key in the page (root), not finding, search in the children
		for (int i = 0; i < myBuffer->page[0]->keycount; i++){

			if (myBuffer->page[0]->key[i].codINEP >= searchKey){
				printf("Verificando na chave %d\n", myBuffer->page[0]->key[i].codINEP);
				pos = i;
				break;
			}
		}
		printf("pos salva %d\n\n", pos);
		//if I find the key, returns the rnn of the data file
		if (myBuffer->page[0]->key[pos].codINEP == searchKey)
			rrn = myBuffer->page[0]->key[pos].rrn;
		else{
			printf("vai para a pagina %d\n", myBuffer->page[0]->child[pos]);

			if (myBuffer->page[0]->child[pos] != -1)
			{
				printf("chave after %d\n", myBuffer->page[0]->key[pos - 1].codINEP);

				printf("chave %d\n", myBuffer->page[0]->key[pos].codINEP);

				rrn = searchTheKey(myBuffer, searchKey, myBuffer->page[0]->child[pos]);
			}
		}
	}
	return rrn;
}

/**
 *  Function searches on a page for the searched key, it is recursive, that is, it goes to the child node of the tree
 *  Parametros:
 *    mtBuffer: buffer with preloaded pages
 *    searchKey: key you are looking for in the tree
 *    pageNum: page that is looking for the key

 *  Return: 
 *    -1: if  does not find the key in the tree
 *    c.a.c: RNN in the data file
 */

int searchTheKey(bufferpool *myBuffer, int searchKey, int pageNum){

	/*  
    btpage -> receives the page from the tree that the key is saved
        pos -> variable to save the position that the key is saved on the page and also to save the location of the child page pointer if you can not find the key on the page
  */
	btpage *pageReturn = searchPage(myBuffer, pageNum, 2);
	int pos = -1;

	for (int i = 0; i < pageReturn->keycount; i++)
		if (pageReturn->key[i].codINEP >= searchKey){
			printf("Verificando na chave %d\n", pageReturn->key[i].codINEP);
			pos = i;
			break;
		}

	printf("pos salva %d\n\n", pos);
	if (pageReturn->key[pos].codINEP == searchKey){
		printf("Ai\n");
		return pageReturn->key[pos].rrn;
	}else{
		printf("vai para a pagina %d\n", pageReturn->child[pos]);

		if (pageReturn->child[pos] != -1){
			printf("chave %d\n", pageReturn->key[pos].codINEP);
			printf("a chave %d\n", pageReturn->key[pos - 1].codINEP);

			return searchTheKey(myBuffer, searchKey, pageReturn->child[pos]);
		}else	return -1;
	}
}

/**
 * Function returns the RNN in the search for a key in the tree
 * Parametros:
 *       searchKey: key being searched on the tree
 * Return:
 *       RNN in the data file
 *       -1 if does not find the key in the tree
 
*/
int BtreeSearchCode(int searchKey){

	bufferpool *myBuffer;
	myBuffer = loadBuffer();
	printBuffer(myBuffer);
	printf("------\n");
	printf("Buffer Load\n");
	printf("------\n");

	return searchRoot(myBuffer, searchKey);
}