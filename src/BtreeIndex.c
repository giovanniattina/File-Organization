#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arquivos.h"
#include "BtreeIndex.h"
#include <string.h>

#define NODESIZE 116
#define INDEXHEADERSIZE 9
#define INDEXFILENAME "BtreeIndex.dat"
#define TREEORDER 10

void setIndexRoot(FILE *indexFile, int newRoot){
	
	fseek(indexFile, 1, SEEK_SET);
	fwrite(&newRoot, sizeof(int), 1, indexFile);

	//chamar funcao para carregar raiz no bufferpool

}

int createPage(FILE *indexFile){
	int keyAmnt = 0, value[TREEORDER + (TREEORDER-1)*2];
	fseek(indexFile, 0, SEEK_END);
	int pos = ftell(indexFile);
	for(int i = 0; i < TREEORDER + (TREEORDER-1)*2; i++)
		value[i]= -1;

	//preenche espaco do no do com 0 pro qtd de chaves e -1 para as chaves, referencias e ponteiros
	fwrite(&keyAmnt, sizeof(int), 1, indexFile);
	fwrite(value, sizeof(int), TREEORDER + (TREEORDER-1)*2, indexFile);
	
	//sai da funcao no inicio da nova pagina criada
	fseek(indexFile, pos, SEEK_SET);
	//retorna numero da pagina 

	return (pos - INDEXHEADERSIZE)/(NODESIZE);
}

//cria um novo arquivo de indice contendo apenas a raiz com 0 chaves
int createBtreeIndexFile(){

	FILE *indexFile = fopen(INDEXFILENAME, "wb+");
	
	unsigned char zero[9] ={0};

	if(!indexFile) return 0;
	
	//status = 0, raiz = 0, altura = 0
	fwrite(&zero, 1, INDEXHEADERSIZE, indexFile);
	
	createPage(indexFile);

	setStatus(indexFile, 1);
	fclose(indexFile);
	return 1;
}

int insertKey(FILE *indexFile, int currPage, int keyAmnt, int insertPos, int *root, int *key, int *rrn){
	
	int vet[TREEORDER + (TREEORDER-1)*2];
	long indexPagePos = INDEXHEADERSIZE + currPage * NODESIZE;
	long indexInsertPos = indexPagePos + 2*sizeof(int) + insertPos*3*sizeof(int);
	
	if(keyAmnt < TREEORDER-1){	//nao ocorre split
		fseek(indexFile, indexInsertPos, SEEK_SET);
		//salva dados para realizar deslocamento
		fread(vet, sizeof(int), (keyAmnt - insertPos)*3, indexFile);
		fseek(indexFile, indexInsertPos, SEEK_SET);
		//escreve nova chave com referencia e ponteiro 
		fwrite(key, sizeof(int), 1, indexFile);
		fwrite(rrn, sizeof(int), 1, indexFile);
		fwrite(root, sizeof(int), 1, indexFile);
		//escreve os dados copiados
		fwrite(vet, sizeof(int), (keyAmnt - insertPos)*3, indexFile); keyAmnt++;
		//atualiza quantidade de chaves
		fseek(indexFile, indexPagePos, SEEK_SET);
		fwrite(&keyAmnt, sizeof(int), 1, indexFile);
	
		*root = -1;
		return 0;
	}

	//faz insercao com split
	int vet2[TREEORDER + (TREEORDER-1)*2], newPgAmnt1 = TREEORDER/2, newPgAmnt2 = TREEORDER/2 -1;	
	if(insertPos < TREEORDER/2){//insere no nodo que ja existe
		//insere no vetor e salva dados ate a metade
		vet[0] = *key;	vet[1] = *rrn;	vet[2] = *root;
		fseek(indexFile, indexInsertPos, SEEK_SET);
		fread(vet+3, sizeof(int), (TREEORDER/2-1 - insertPos)*3, indexFile);
		//salva separadamente os dados da outra metade
		fread(vet2, sizeof(int), (TREEORDER/2) *3, indexFile);
		fseek(indexFile, indexPagePos, SEEK_SET);
		//atualiza quantidade de chaves
		fwrite(&newPgAmnt1, sizeof(int), 1, indexFile);
		fseek(indexFile, indexInsertPos, SEEK_SET);
		//atualiza a primeira metade
		fwrite(vet, sizeof(int), (TREEORDER/2 - insertPos)*3, indexFile);
		//cria uma nova pagina e escreve nela a nova metade
		*root = createPage(indexFile);	//guarda numeracao da pagina criada para fazer a promocao de no
		*key = vet2[0]; *rrn = vet2[1];	//guarda chave que sera promovida
		//atualiza quantidade de chaves e as escreve
		fwrite(&newPgAmnt2, sizeof(int), 1, indexFile);
		fwrite(vet2 + 2, sizeof(int), (TREEORDER/2 -1)*3 + 1, indexFile);
	}
	else{//a insercao ocorrera no nodo criado
		//posiciona no inicio da pagina e atualiza quantidade
		fseek(indexFile, indexPagePos, SEEK_SET);
		fwrite(&newPgAmnt1, sizeof(int), 1, indexFile);
		//posiciona no meio e guarda informacao ate chegar na posicao de insercao
		fseek(indexFile, ((TREEORDER/2)*3 + 1) * sizeof(int), SEEK_CUR);
		fread(vet, sizeof(int), (insertPos - TREEORDER/2)*3, indexFile);
		//guarda a nova chave e termina de copiar os dados pro vetor
		vet[(insertPos - TREEORDER/2)*3] = *key;
		vet[(insertPos - TREEORDER/2)*3 +1] = *rrn;
		vet[(insertPos - TREEORDER/2)*3 + 2] = *root;
		fread(vet + (insertPos - TREEORDER/2)*3 + 3, sizeof(int), (TREEORDER-1 - insertPos)*3,indexFile);
		//cria uma nova pagia. O numero da pagina eh armazenado assim como os dados da chave promovida
		*root = createPage(indexFile);
		*key = vet[0];	*rrn = vet[1];
		//por fim, escreve dados copiados no nodo atualizando a qtd de chaves
		fwrite(&newPgAmnt2, sizeof(int), 1, indexFile);
		fwrite(vet + 2, sizeof(int), (TREEORDER/2 -1)*3 + 1, indexFile);
	}
		return 1;
}

int findInsertPos(FILE *indexFile, int *root, int *key, int *rrn){
	
	int keyAmnt, insertPos = 0, rootParam = *root, keyRead;
	unsigned char halt = 0;
	
	//posiciona na pagina e le quantidade de chaves existentes
	fseek(indexFile, INDEXHEADERSIZE + NODESIZE*rootParam, SEEK_SET); 
	fread(&keyAmnt, sizeof(int), 1, indexFile);
	fseek(indexFile, -sizeof(int), SEEK_CUR);

	while(insertPos < keyAmnt && !halt){
		//vai ler chave para comparacao. precisa pular referencia e ponteiro
		fseek(indexFile, 2*sizeof(int), SEEK_CUR);
		fread(&keyRead, sizeof(int), 1, indexFile);
		if(*key < keyRead)
			halt = 1;
		else
			insertPos++;
	}
	if( !halt ){//vai ler ponteiro da ultima chave na pagina
		fseek(indexFile, sizeof(int), SEEK_CUR);
		fread(root, sizeof(int), 1, indexFile);
	}
	else{
		//vai ler ponteiro anterior a chave que parou o loop
		fseek(indexFile, -2*sizeof(int), SEEK_CUR);
		fread(root, sizeof(int), 1, indexFile);
	}
	
	if( *root == -1 )//eh no folha, entao vai inserir nele
		return insertKey(indexFile, rootParam, keyAmnt, insertPos, root, key, rrn); 
	
	else	//chama novamente pegando como raiz o ponteiro lido
		findInsertPos(indexFile, root, key, rrn);
	
	//volta da recursao. Precisa verificar a promocao de no. Ocorrera se a root foi alterada para deixar de valer -1
	if( *root != -1 )//houve promocao. Vai inserir no nodo. A posicao ja esta definida.
							//os parametros ja foram alterados na propria insercao durante o split
		return insertKey(indexFile, rootParam, keyAmnt, insertPos, root, key, rrn);
	
	else
		return 1;
}

int insertKeyToIndex(int key, int rrn){
	
	int root, auxRoot, one = 1;

	FILE *indexFile = fopen(INDEXFILENAME, "rb+");
	
	//verificando integridade do arquivo
	if(!indexFile) return 0;
	if(isCorruptedFile(indexFile)){
		fclose(indexFile);
		return 0;
	}
	//marcando como aberto
	setStatus(indexFile, 0);
	
	fread(&root, sizeof(int), 1, indexFile);
	auxRoot = root;
	//busca posicao e insere.
	//Parametros passados como ponteiros serao alterados para a promocao
	findInsertPos(indexFile, &root, &key, &rrn);
	
	if(root != -1){	//houve promocao para gerar uma nova raiz
		//cria nodo, atribui valores
		int newRoot = createPage(indexFile);
		fwrite(&one, sizeof(int), 1, indexFile);
		fwrite(&auxRoot, sizeof(int), 1, indexFile);
		fwrite(&key, sizeof(int), 1, indexFile);
		fwrite(&rrn, sizeof(int), 1, indexFile);
		fwrite(&root, sizeof(int), 1, indexFile);
		//escreve nova raiz e nova altura da arvore
		fseek(indexFile, 1, SEEK_SET);
		fwrite(&newRoot, sizeof(int), 1, indexFile);
		fread(&one, sizeof(int), 1, indexFile);
		fseek(indexFile, -sizeof(int), SEEK_CUR); one++;
		fwrite(&one, sizeof(int), 1, indexFile);
	}

	setStatus(indexFile, 1);
	fclose(indexFile);
}

void printRec(FILE *indexFile, int root){
	printf("PAGINA %d.", root);
	fseek(indexFile, INDEXHEADERSIZE + root*NODESIZE, SEEK_SET);
	int keyAmnt;
	
	fread(&keyAmnt, sizeof(int), 1, indexFile);
	printf(" Contem %d chaves: ", keyAmnt);
	int son, key, rrn, ptr;
	
	fread(&ptr, sizeof(int), 1, indexFile);
	printf("%d ", ptr);
		
	for(int i=0; i<keyAmnt; i++){
		fread(&key, sizeof(int), 1, indexFile);
		fread(&rrn, sizeof(int), 1, indexFile);
		fread(&ptr, sizeof(int), 1, indexFile);
		printf("|%d %d %d| ", key, rrn, ptr);
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
	int root, altura;
	fread(&root, sizeof(int), 1, indexFile);
	fread(&altura, sizeof(int), 1, indexFile);
	printf("raiz: %d   altura: %d\n", root, altura);
	printRec(indexFile, root);
		
	setStatus(indexFile, 0);
	fclose(indexFile);
}



