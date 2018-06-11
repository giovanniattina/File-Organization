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
	for (int i=0; i<5; i++){
		buffer->changed[i] = -1;
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
		
		return vet[ 1 + rand() % vet[0] ];
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
	
	//if this position is not empty, the data gets written to the disk
	if( buffer->pageNums[ insertPos ] != -1){
		if (!savePage(buffer, buffer->page[insertPos])) return 0;
	}
	
	buffer->changed[insertPos] = changedVal;
	//inserting the page and updating 
	buffer->page[ insertPos ] = page;
	buffer->pageNums[ insertPos ] = page->pageNum;
	
	return 1;
}

btpage* searchPage(bufferpool *buffer, int pageNum, int changedVal){

	//tries to find the page inside the buffer
	for (int i = 0; i < BUFFERSIZE; i++){
		if( buffer->pageNums[i] == pageNum){
			updateBufferUse(buffer, i, changedVal);
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
	if( buffer->pageNums[0] == page->pageNum ){	//will write the root, so updates the header first
		fwrite(&(page->pageNum), sizeof(int), 1, btreeFile);
		fwrite(&(buffer->treeHeight), sizeof(int), 1, btreeFile);
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





