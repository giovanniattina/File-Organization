#include <stdlib.h>
#include <stdio.h>

#include "BtreeIndex.h"
#include "arquivos.h"
#include "bufferNRU.h"
#include <time.h>

/**
 *	Funcao usada para alocar um novo buffer
 *  Return: bufferpool * : o buffer gerado com raiz 0  e todas as posicoes livres
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

/*
*Encontra a proxima posicao do buffer para isercao. A politica NRU e' aplicada aqui.
*/
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
 * Atualiza o estado de alteracao da pagina de disco passada
 *
 * \param bufferpool *buffer: bufferpool contendo a pagina que sera atualizada
 * \param int pagePos : posicao no buffer da pagina que sera alterada
 * \param int changedVal: novo valor da politica da pagina
 * return int: 0 informando que foi bem sucedida
 */

int updateBufferUse(bufferpool *buffer, int pagePos, int changedVal){

	int changed = buffer->changed[pagePos];

	if(changed == 3 || changed * changedVal == 2)
			buffer->changed[ pagePos ] = 3;
	else
		buffer->changed[ pagePos ] = changedVal;

	return 0;
}

/*
*
* Faz insercao de uma nova pagina no buffer utilizando a politica de substituicao NRU.
* Caso alguma pagina modificada seja removida, sera feito um acesso ao disco para grava-la.
*
* \param bufferpool *buffer : o bufferpool contendo as paginas de indice
* \param btpage *page : a pagina a ser salva
* \param int changedVal : valor do modificador de acesso a ser atribuido para a pagina
*/
int insertBuffer(bufferpool *buffer, btpage *page, int changedVal){

	//encontrando posicao de insercao
	int insertPos = findBufferPos(buffer, page->pageNum);

	//se essa posicao nao esta vazia, os dados serao escritos no disco antes de modificar
	if( buffer->pageNums[ insertPos ] != -1){
		if (!savePage(buffer, buffer->page[insertPos])) return 0;
	}

	buffer->changed[insertPos] = changedVal;
	//inserindo a pagina e atualizando numero das paginas em buffer
	buffer->page[ insertPos ] = page;
	buffer->pageNums[ insertPos ] = page->pageNum;

	return 1;
}

/*
* Ira buscar no buffer pela pagina contendo o numero(rrn) passado. 
* Caso ela nao seja encontrada, sera recuperada do disco e inserida no buffer.
*
* \param int pageNum : rrn do arquivo de indice da pagina buscada
* \param int changedVal : modificador de acesso da operacao que se deseja realizar na pagina
* \return btPage* : a pagina de disco com o numero passado.
*/
btpage* searchPage(bufferpool *buffer, int pageNum, int changedVal){
	

	//tenta encontrar a pagina no buffer
	for (int i = 0; i < BUFFERSIZE; i++){
		if(buffer->pageNums[i] == pageNum){
			updateBufferUse(buffer, i, changedVal);
			buffer->hit++;
			return buffer->page[i];
		}
	}
	//pagina nao encontrada. Vai recuperar do disco
	FILE *btree =fopen(INDEXFILENAME, "rb+");

	if (btree == NULL) return NULL;

	if (isCorruptedFile(btree)){
		fclose(btree);
		return NULL;
	}
	setStatus(btree, 0);

	//criando nova pagina vazia
	btpage* page = createPage();

	//lendo quantidade de chaves e primeiro ponteiro
	fseek(btree, INDEXHEADERSIZE + NODESIZE * pageNum, SEEK_SET);
	fread(&(page->keycount), sizeof(int), 1, btree);
	fread(&(page->child[0]), sizeof(int), 1, btree);

	//lendo as chaves e os ponteiros restantes
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

/*
* Retorna a pagina da raiz armazenada no buffer
* 
* \return btpage * : a raiz da arvore
*/
btpage* getRootPage(bufferpool *buffer){
	
	buffer->hit++;
	return buffer->page[0];
}

/*
* Define a pagina passada como a nova raiz do buffer.
* Fara 1 acesso ao disco para escrever a raiz antiga caso tenha sido modificada
*
* \param btpage *newRoot : pagina que se tornara a nova raiz da arvore
* \return int : 1 se terminou sem erros. 0 se algum erro ocorreu. 
*/
int setRootPage(bufferpool *buffer, btpage *newRoot){
	if( !savePage(buffer, buffer->page[0]) ) return 0;

	buffer->page[0] = newRoot;
	buffer->pageNums[0] = newRoot->pageNum;
	buffer->fault++;
	return 1;
}

/*
* Salva todas as paginas modificadas em disco. Libera toda a memoria alocada para o buffer.
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

/*
* Salva em disco a pagina passada, liberando o espaco ocupado por ela no buffer
* 
* \param bufferpool *buffer : bufferpool contendo paginas de disco do indice
* \param btpage* page : pagina que sera retirada do buffer e salva
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

/*
* Imprime o conteudo do buffer, incluindo cada pagina contida nele
*/
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
* Acessa o disco para recuperar os dados de uma pagina do arquivo de indices.
*
* \param int pageNum : rrn da pagina a ser recuperada
* \param FILE *btreeFile : handler do arquivo ja aberto para leitura da pagina
* return btpage* : a pagina recuperada
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

/*
* Acessa a arvore em largura ate preencher todas as posicos vazias do buffer ou 
* ate chegar ao final do arquivo de indice
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
	
	//recuperando o numero da pagina raiz, altura da arvore e numero da ultima pagina criada
	fread(&(rootNum), sizeof(int), 1,btreeFile);
	fread(&(buffer->treeHeight), sizeof(int), 1,btreeFile);
	fread(&(buffer->totalPages), sizeof(int), 1,btreeFile);

	setStatus(btreeFile, 1);
	fclose(btreeFile);
	
	buffer->pageNums[0] = -1;//setando como posicao livre para conseguir buscar
	//setando posicao 0 do vetor de paginas para ser a raiz
	buffer->page[0] = searchPage(buffer, rootNum, UNUSED);
	buffer->pageNums[0] = rootNum;
	buffer->pageNums[1] = -1;

	//ira carregar mais 4 paginas do arquivo de indice
	while(!endLoad){

		//vai olhar os filhos de cada posicao preenchida do buffer, iniciando pela raiz
		if(buffer->page[searchPos]->child[cont] != -1){
			searchPage(buffer, buffer->page[searchPos]->child[cont], UNUSED);
			buffer->pageNums[bufferPos++] = buffer->page[searchPos]->child[cont];
		}
		cont++;
		//checando se chegou no ultimo ponteiro
		if(cont >= buffer->page[searchPos]->keycount + 1){
			searchPos++;
			cont =0;
		}
		
		//checando condicao de parada: todas as posicoes do buffer preenchidas ou sem paginas restantes para ler
		if(bufferPos >= BUFFERSIZE || buffer->page[searchPos] == NULL || searchPos >= BUFFERSIZE)
			endLoad = 1;
	}
	
	//desconsiderando inicializacao do buffer para contagem
	buffer->hit = 0;
	buffer->fault = 0;
	
	return buffer;
}

/*
* Funcao inicia buscando a raiz da arvore que e' salva no bufferpool ate encontrar o rrn requisitado
*
* \param bufferpool *buffer: bufferpool carregado com paginas para a busca
* \param int searchKey : chave que esta sendo buscada na arvore
* \return : -1 se a chave nao foi encontrada. RRN do registro no arquivo de dados.
*/
int searchRoot(bufferpool *myBuffer, int searchKey){

	/*	variavel para guardar a posicao em que a chave e' salva na pagina e tambem para guardar a posicao do ponteiro filho para quando a chave nao for encontrada*/
	int pos = -1;
	int rrn = -1;

	//primeiro checa se existe uma raiz no buffer
	if (myBuffer->pageNums[0] != -1){
		
		// existe raiz no buffer, vai buscar a chave na pagina. nao encontrando, busca nos filhos
		for (int i = 0; i < myBuffer->page[0]->keycount; i++){

			if (myBuffer->page[0]->key[i].codINEP >= searchKey){
				pos = i;
				break;
			}
		}

		//se chave encontrada, retorna rrn do registro no arquivo de dados
		if (myBuffer->page[0]->key[pos].codINEP == searchKey)
			rrn = myBuffer->page[0]->key[pos].rrn;
		else{

			if (myBuffer->page[0]->child[pos] != -1)
				rrn = searchTheKey(myBuffer, searchKey, myBuffer->page[0]->child[pos]);
		}
	}
	return rrn;
}

/*
* Funcao que percorre  pagina buscando a chave. E' recursiva ate chegar em uma folha da arvore
* 
* \param bufferpool *mybuffer: bufferpool carregado com paginas para a busca
* \param int searchKey : chave que esta sendo buscada na arvore
* \param int pageNum : numero da pagina que esta sendo percorrida
* \return : -1 se a chave nao foi encontrada. RRN do registro no arquivo de dados.
*/
int searchTheKey(bufferpool *myBuffer, int searchKey, int pageNum){

	/*  
    btpage -> receve a pagina da arvore onde a chave esta salva
        pos -> variavel para guardar a posicao em que a chave e' salva na pagina e tambem para guardar a posicao do ponteiro filho para quando a chave nao for encontrada
  */
	btpage *pageReturn = searchPage(myBuffer, pageNum, 2);
	int pos = -1;

	for (int i = 0; i < pageReturn->keycount; i++)
		if (pageReturn->key[i].codINEP >= searchKey){
			pos = i;
			break;
		}

	if (pageReturn->key[pos].codINEP == searchKey){
		return pageReturn->key[pos].rrn;
	}else{

		if (pageReturn->child[pos] != -1){

			return searchTheKey(myBuffer, searchKey, pageReturn->child[pos]);
		}else	return -1;
	}
}

/**
* Funcao que retorna o RRN na busca por uma chave na arvore
*
* \param int searchKey: chave buscada
* \return : -1 se a chave nao foi encontrada. RRN do registro no arquivo de dados.
*/
int BtreeSearchCode(int searchKey){

	bufferpool *myBuffer;
	myBuffer = loadBuffer();

	return searchRoot(myBuffer, searchKey);
}
