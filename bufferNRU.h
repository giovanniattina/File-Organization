#ifndef _BUFFERNRU_H_
#define _BUFFERNRU_H_

#include "BtreeIndex.h"

#define BUFFERSIZE 5
#define UNUSED 0
#define MODIFIED 1
#define ACCESSED 2
#define MOD_AND_ACC 3

struct buf{
	btpage *page[BUFFERSIZE];
	int pageNums[BUFFERSIZE];
	char changed[BUFFERSIZE];
	int totalPages, treeHeight;
};//bufferpool -> included as typedef at "BtreeIndex.h"

bufferpool* createBuffer();
int insertBuffer(bufferpool *buffer, btpage *page, int changedVal);
int findBufferPos(bufferpool *buffer);
void printBuffer(bufferpool *buffer);
int saveAllPages(bufferpool *buffer);
int savePage(bufferpool *buffer, btpage *page);
btpage* searchPage(bufferpool *buffer, int pageNum, int changedVal, int* hit, int* fault);
btpage* getRootPage(bufferpool *buffer);
int setRootPage(bufferpool *buffer, btpage *newPage);
bufferpool* loadBuffer(int* hit, int* fault);
#endif
