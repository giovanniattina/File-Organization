#include <stdlib.h>
#include <stdio.h>
#include "BtreeIndex.h"
#include "arquivos.h"
#include "bufferNRU.h"
#include <time.h>

bufferpool* createBuffer(){

	bufferpool *buffer = (bufferpool*)malloc(sizeof(bufferpool));

	//creating empty page 0 as the root
	buffer->page[0] = createPage();
	buffer->totalPages = 0;
	buffer->treeHeight = 0;

	//initializing remaning buffer variables
	for (int i=0; i<BUFFERSIZE; i++){
		buffer->changed[i] = 0;
		buffer->pageNums[i] = -1;
	}

	buffer->pageNums[0] = 0;
	srand( time(NULL) );

	return buffer;
}

int findBufferPos(bufferpool *buffer){

	for (int i=1; i<BUFFERSIZE; i++){
		if( buffer->pageNums[i] == -1 )
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

int updateBufferUse(bufferpool *buffer, int pagePos, int changedVal){

	int changed = buffer->changed[pagePos];

	if(changed == 3 || changed * changedVal == 2)
			buffer->changed[ pagePos ] = 3;
	else
		buffer->changed[ pagePos ] = changedVal;

	return 0;
}

int insertBuffer(bufferpool *buffer, btpage *page, int changedVal){

	//finding the postion for the insertion
	int insertPos = findBufferPos(buffer);
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

btpage* searchPage(bufferpool *buffer, int pageNum, int changedVal, int* hit, int* fault){

	//tries to find the page inside the buffer
	for (int i = 0; i < BUFFERSIZE; i++){
		if(buffer->pageNums[i] == pageNum){
			updateBufferUse(buffer, i, changedVal);
			*hit = *hit + 1;
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

	*fault++;
	
	return page;
}

btpage* getRootPage(bufferpool *buffer){

	return buffer->page[0];
}

int setRootPage(bufferpool *buffer, btpage *newRoot){
	if( !savePage(buffer, buffer->page[0]) ) return 0;

	buffer->page[0] = newRoot;
	buffer->pageNums[0] = newRoot->pageNum;
	return 1;
}

int saveAllPages(bufferpool *buffer){
	for (int i = 0; i < BUFFERSIZE; i++){
		if(buffer->pageNums[i] != -1){
			if( !savePage(buffer, buffer->page[i]) )
				return 0;
		}
	}
	return 1;
}

int savePage(bufferpool *buffer, btpage *page){

	FILE *btreeFile = fopen(INDEXFILENAME, "rb+");
	if (btreeFile == NULL)
		return 0;
	if( isCorruptedFile( btreeFile ) ){
		fclose( btreeFile );
		return 0;
	}

	setStatus(btreeFile, 0);
	//printf("------------------\n");
	//printBuffer(buffer);
	//printf("page->pageNum = %d\n", page->pageNum);
	//printf("------------------\n");
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

/*

*/
bufferpool* loadBuffer(int* hit, int* fault){
	bufferpool* buffer = createBuffer();
	
	printBuffer(buffer);

	FILE *btreeFile = fopen(INDEXFILENAME, "rb+");
	if (btreeFile == NULL)
		return NULL;
	if( isCorruptedFile( btreeFile ) ){
		fclose( btreeFile );
		return NULL;
	}

	setStatus(btreeFile, 1);

	int rootNum, bufferPos = 1, cont = 0, searchPos = 0, endLoad = 0;
	
	//retrieving root number, tree height and last page number
	fread(&(rootNum), sizeof(int), 1,btreeFile);
	fread(&(buffer->treeHeight), sizeof(int), 1,btreeFile);
	fread(&(buffer->totalPages), sizeof(int), 1,btreeFile);

	setStatus(btreeFile, 1);
	fclose(btreeFile);
	
	//set buffer position 0 to be the root
	buffer->page[0] = searchPage(buffer, rootNum, UNUSED, hit, fault);
	buffer->pageNums[0] = rootNum;
	buffer->pageNums[1] = -1;
	
	//will load 4 additional pages to the buffer
	while(!endLoad){
		//will look at the children of each filled position of the buffer starting with the root
		if(buffer->page[searchPos]->child[cont] != -1){
			searchPage(buffer, buffer->page[searchPos]->child[cont], UNUSED, hit, fault);
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
	
	*hit = 0;
	*fault = 0;
	
	return buffer;
}
