
#ifndef _B_TREE_INDEX_H_
#define _B_TREE_INDEX_H_

#define INDEXFILENAME "BtreeIndex.dat"
#define NODESIZE 116
#define INDEXHEADERSIZE 13
#define TREEORDER 10

typedef struct{
	int codINEP;
	int rrn;
} treekey;

typedef struct{
	int keycount;
	treekey key[TREEORDER-1];
	int child[TREEORDER];
	int pageNum;
} btpage;

typedef struct buf bufferpool;

int createBtreeIndexFile();
int insertKeyToIndex(bufferpool *buffer, int key, int rrn);
int printBtree();
btpage* createPage();
void printPage(btpage *page);

#endif
