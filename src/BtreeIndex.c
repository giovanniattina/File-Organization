#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arquivos.h"
#include "bufferNRU.h"
#include "BtreeIndex.h"

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

/*
*Cria uma nova pagina de disco(no da arvore) com 0 chaves
*/
btpage *createPage(){

	btpage *page = (btpage*)calloc(1, sizeof(btpage));
	//numero de chaves 0 e todos os ponteiros com -1
	page->child[0] = -1;
	for(int i=0; i<TREEORDER; i++)
		page->child[i] = -1;

	return page;
}

/*
* Faz a insercao da chave de fato. recebe a pagina e a posicao de insercao ja definidas.
* Verifica também se houve overflow, realizando entao split com criacao de uma nova pagina e
* promocao de uma chave.
*
*\param *root : pagina onde sera feita a insercao
*\param *buffer : buffer utilizado para diminuir acessos feitos a disco
*\param int insertPos : posicao de insercao na chave passada ( de 0 ate ordem da arvore - 1)
*\param int *ptr, *key, *rrn : dados da chave a ser inserida. Caso haja promocao de chave, 
*	serao alterados para o valor da chave que foi promovida
*/
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
				newPage->child[i+1] = root->child[TREEORDER/2 + 1 + i];
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

/*
* Percorre a arvore procurando pela posicao de insercao da chave passada.
* Funcao recursiva que vai acessando as paginas com chaves mais proximas da passada.
* E responsavel por gerir as chaves promovidas da funcao insertKey durante a volta da recursao
*/
int findInsertPos(bufferpool* buffer, btpage *root, int *ptrRead, int *key, int *rrn){

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
		promoted = findInsertPos(buffer, searchPage(buffer, *ptrRead, ACCESSED), ptrRead, key, rrn);

	if( promoted < 0 ) return -1;	//erro durante isercao

	//volta da recursao. Promoted indica se houve split, ptrRead sera modificado para o nmr da pagina que propagou
	if( promoted ){// A posicao ja esta definida eh a mesma definida no while.
							//os parametros key e rrn ja foram alterados durante o split
		root = searchPage(buffer, pageNumMem, MODIFIED);//pode ter sido liberada durante uma insercao anterior
		return insertKey(buffer, root, insertPos, ptrRead, key, rrn);
	}
	else
		return 0;
}

/*
*Inicia uma nova insercao no arquivo de indice. Para tal, carrega a raiz da arvore e 
* chama a funcao da recursao.
* precisa analisar o retorno da recursao para recuperar uma nova pagina promovida para a raiz
*/
int insertKeyToIndex(bufferpool *buffer, int key, int rrn){

	int ptr = -1, promoted = -1;
	btpage *newPage;
	//lendo a raiz do buffer
	btpage *root = getRootPage(buffer);

	//busca posicao e insere.
	//Parametros passados como ponteiros serao alterados para a promocao
	promoted = findInsertPos(buffer, root, &ptr, &key, &rrn);
	
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

/*
* Parte recursiva da busca por uma chave que ira acessar paginas de forma a tentar encontrar
* a chave(codINEP) passada
*/
int searchKey(btpage *root, bufferpool *buffer, int codINEP){

	int i = 0, keyAmnt, halt = 0;
	keyAmnt = root->keycount;

	//compara todas as chaves da pagina com a chave buscada
	while( i < keyAmnt  && !halt){

		if( codINEP <= root->key[i].codINEP )//encontrou ou a chave ou codigo maior
			halt = 1;
		else
			i++;
	}
	// o 'i' foi posicionado no ponteiro que devera ser acessado
	// ou na chave igual a buscada
	
	if(halt && root->key[i].codINEP == codINEP) return root->key[i].rrn;

	if(root->child[i] == -1) return -1;
	
	//chama a busca novamente para a pagina do ponteiro lido
	return searchKey(searchPage(buffer, root->child[i], ACCESSED), buffer, codINEP);
}

/*
* Inicializa a busca da arvore, recuperando a raiz pelo buffer e chamando a busca recursiva.
* \return int RRN : Se encontrou a chave, retorna o RRN do registro no arquivo de dados, senao
*		retorna -1;
*/
int BtreeSearch(int codINEP){
	
	//inicializa o buffer, carregando com paginas do indice
	bufferpool *buffer = loadBuffer();
	if (!buffer) return -2;
	//recupera raiz da arvore
	btpage *root = getRootPage(buffer);
	if (!root) return -2;
	
	//chama busca recursiva
	int rrn = searchKey(root, buffer, codINEP);
	
	//liberando as paginas do buffer(nao foram alteradas, logo nao serao reescritas)
	for (int i = 0; i < BUFFERSIZE; i++){
		if(buffer->pageNums[i] != -1)
			deletePage(buffer, buffer->pageNums[i]);
	}
	
	writeBufferInfo(buffer);

	free(buffer);
	return rrn;	
}

/*
*Parte recursiva da impressao da arvore
*/
void printRec(FILE *indexFile, int root){

	//vai acessar a pagina no rrn passado em root
	//vai ler os dados da pagina e imprimir	
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

/*
*Imprime a arvore B fazendo acesso em profundidade
*/
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

/*
*Imprime os dados de uma pagina de disco
*/
void printPage(btpage *page){
	printf("\npage %d\n keyAmnt: %d -> %d|",page->pageNum, page->keycount, page->child[0]);

	for (int i=0; i<page->keycount; i++){
		printf("%d|%d|%d| ", page->key[i].codINEP, page->key[i].rrn, page->child[i+1]);
	}
	printf("\n\n");


}

//copia chaves e ponteiros de page a partir de uma posicao especificada
int copyToVet(treekey* keys, int* children, btpage* page, int insertPos){
	
	//enquanto nao chegar na posicao limite, copia dados da pagina
	for(int i = 0; i < page->keycount; i++){
		keys[insertPos].codINEP = page->key[i].codINEP;
		keys[insertPos].rrn = page->key[i].rrn;
		children[insertPos++] = page->child[i];
	}
	//ao terminar, restara copiar o ultimo ponteiro(direito)
	children[insertPos] = page->child[page->keycount];

	return insertPos;
}

/*
* Copia dados de chave e rrn para preencher uma pagina de indice
*/
int copyToPage(treekey* keys, int* children, btpage* page, int insertPos, int vetSize){
	
	//percorre os vetores ate o final copiando os dados
	for(int i = 0; i < vetSize; i++){
		page->key[i].codINEP = keys[insertPos].codINEP;
		page->key[i].rrn = keys[insertPos].rrn;
		page->child[i] = children[insertPos++];
	}
	page->child[vetSize] = children[insertPos];

	page->keycount = vetSize;

	return insertPos;	

}

/*
* Faz a redistribuicao de fato
*
*\param btPage *childCpy : pagina de um dos filhos que sera usada para a copia
*\param btPage *parentCpy : pagina do pai que mediara a redistribuicao
*\param child : qual ponteiro do pai(parentCpy) esta sofrendo redistribuicao
*\param right : informa se a pagina childCpy passada foi o filho direito ou esquerdo de parentCpy
*		permitindo saber qual pagina ainda precisa ser recuperada do buffer para tentar redistribuir
*/
int redistribPages(bufferpool* buffer, btpage* childCpy, btpage* parentCpy, int child, unsigned char right){

	treekey keys[TREEORDER + (TREEORDER/2) - 2];
	int children[TREEORDER + (TREEORDER/2) - 1];

	btpage* secondChild = searchPage(buffer, parentCpy->child[child], MODIFIED);

	if(secondChild == NULL) return -1;
	
	int insertPos = 0;

	if(right){	//se vai pegar o irmao da direita 
		//juntando os dados das paginas
		insertPos = copyToVet(keys, children, secondChild, insertPos);
		keys[insertPos].codINEP = parentCpy->key[child].codINEP;
		keys[insertPos++].rrn = parentCpy->key[child].rrn;
		insertPos = copyToVet(keys, children, childCpy, insertPos);

		//redefinindo as paginas dos filhos
		int insertPagePos = 0;
		insertPagePos = copyToPage(keys, children, secondChild, insertPagePos, insertPos/2);
		insertPagePos = copyToPage(keys, children, childCpy, insertPagePos + 1, insertPos - (insertPos/2) - 1);
	}
	else{
		child--;
		//juntando os dados das paginas
		insertPos = copyToVet(keys, children, childCpy, insertPos);
		keys[insertPos].codINEP = parentCpy->key[child].codINEP;
		keys[insertPos++].rrn = parentCpy->key[child].rrn;
		insertPos = copyToVet(keys, children, secondChild, insertPos);

		//redefinindo as paginas dos filhos
		int insertPagePos = 0;
		insertPagePos = copyToPage(keys, children, childCpy, insertPagePos, insertPos/2);
		insertPagePos = copyToPage(keys, children, secondChild, insertPagePos + 1, insertPos - (insertPos/2) - 1);
	}

	//definindo nova chave do pai
	parentCpy->key[child].codINEP = keys[insertPos/2].codINEP;
	parentCpy->key[child].rrn = keys[insertPos/2].rrn;

	return insertBuffer(buffer, parentCpy, MODIFIED) && insertBuffer(buffer, childCpy, MODIFIED);

}

/*
* Verifica se a redistribuicao eh possivel. Se for, chama a funcao que a realizara.
* Caso nao seja possivel, ira retornar 0.
*\param btPage *parent : nodo pai dos nodos que serao testados
*\param int child : indica qual ponteiro de parent deve ser tentada a redistribuicao
*/
int redistrib(bufferpool* buffer, btpage* parent, int child){

	treekey keys[TREEORDER + (TREEORDER/2) - 2];
	int children[TREEORDER + (TREEORDER/2) - 1];

	btpage* parentCpy = createPage();

	memcpy(parentCpy, parent, sizeof(btpage));		//faz copia para nao perder dados ao chamar searchPage()

	//caso em que tera apenas irmao direito
	if(child == 0){	
		
		btpage* rightChild = searchPage(buffer, parentCpy->child[1], MODIFIED);

		//verificando quantidade  de nodos no irmao
		if(rightChild->keycount < TREEORDER/2){
			free(parentCpy);
			return 0;	//nao consegue fazer redistribuicao
		}

		btpage* rightChildCpy = createPage();
		memcpy(rightChildCpy, rightChild, sizeof(btpage));		//copia pois vai chamar searchPage() novamente

		return redistribPages(buffer, rightChildCpy, parentCpy, child, 1);

	}

	//caso em que tera apenas irmao esquerdo
	if(child == parentCpy->keycount){	

		btpage* leftChild = searchPage(buffer, parentCpy->child[parentCpy->keycount-1], MODIFIED);
		if(leftChild->keycount < TREEORDER/2){
			free(parentCpy);
			return 0;   //nao consegue fazer redistribuicao
		}

		btpage* leftChildCpy = createPage();
		memcpy(leftChildCpy, leftChild, sizeof(btpage));		//copia pois vai chamar searchPage() novamente

		return redistribPages(buffer, leftChildCpy, parentCpy, child, 0);
		
	}
	
	//resta o caso em que tem os dois irmaos

	btpage* rightChild = searchPage(buffer, parentCpy->child[child + 1], MODIFIED);
	//verificando quantidade  de nodos no irmao
	if(rightChild->keycount < TREEORDER/2){	//se nao consegue fazer na direita

		btpage* leftChild = searchPage(buffer, parentCpy->child[child - 1], MODIFIED);
		if(leftChild->keycount < TREEORDER/2){	//tenta fazer na esquerda
			free(parentCpy);
			return 0;	//nao conseguiu redistribuir com nenhum dos irmaos
		}
		btpage* leftChildCpy = createPage();
		memcpy(leftChildCpy, leftChild, sizeof(btpage));		//copia pois vai chamar searchPage() novamente

		return redistribPages(buffer, leftChildCpy, parentCpy, child, 0);
	}

	btpage* rightChildCpy = createPage();
	memcpy(rightChildCpy, rightChild, sizeof(btpage));		//copia pois vai chamar searchPage() novamente

	return redistribPages(buffer, rightChildCpy, parentCpy, child, 1);
}
/*
* Faz a concatenacao de um nodo com seu nodo irmao.
*\param btpage *parent : o nodo pai dos irmaos que serao concatenados
*\param int child : ponteiro da pagina parent que devera ser olhado para recuperar os irmaos
*/
int concat(bufferpool* buffer, btpage* parent, int child){
	
	int leastPage;//o numero da pagina que ficar no fim da concatenacao
	btpage* parentCpy = createPage();
	memcpy(parentCpy, parent, sizeof(btpage));		//faz copia para nao perder dados ao chamar searchPage()
	
	if(child == parentCpy->keycount){		//vai concatenar com irmao esquerdo
		
		//faz copia do irmao direito
		btpage* rightChild = searchPage(buffer, parentCpy->child[child], MODIFIED);
		if(rightChild == NULL) return -2;
		btpage* rightChildCpy = createPage();
		memcpy(rightChildCpy, rightChild, sizeof(btpage));
		
		//recupera irmao esquerdo
		btpage* leftChild = searchPage(buffer, parentCpy->child[child - 1], MODIFIED);
		if(leftChild == NULL) return -2;
		
		//o irmao esquerdo se mantem. O pai vai entrar no final dele
		leftChild->key[leftChild->keycount].codINEP = parentCpy->key[child - 1].codINEP;
		leftChild->key[leftChild->keycount++].rrn = parentCpy->key[child - 1].rrn;
		//o nodo direito eh copiado para as posicoes restantes
		for(int i = 0; i < rightChildCpy->keycount; i++){
			leftChild->key[leftChild->keycount].codINEP = rightChildCpy->key[i].codINEP;
			leftChild->key[leftChild->keycount].rrn = rightChildCpy->key[i].rrn;
			leftChild->child[leftChild->keycount++] = rightChildCpy->child[i];			
		}
		leftChild->child[leftChild->keycount] = rightChildCpy->child[rightChildCpy->keycount];			
		
		parentCpy->keycount--;
		
		//sera mantida a pagina de menor numero. 
		int rightPageNum = rightChildCpy->pageNum;
		//remove do buffer(e da arvore) 
		deletePage(buffer, rightPageNum);
		free(rightChildCpy);
		
		//deletada do buffer se estiver nele para retirar duplicata
		btpage *leftChildCpy = createPage();
		memcpy(leftChildCpy, leftChild, sizeof(btpage));
		deletePage(buffer, leftChild->pageNum);
		
		//definindo nmr da pagina como a menor das duas
		leastPage = (leftChildCpy->pageNum > rightPageNum) ?  rightPageNum : leftChildCpy->pageNum;
		
		leftChildCpy->pageNum = leastPage;
		parentCpy->child[parentCpy->keycount] = leastPage;
		
		//garante que o numero total de paginas nao cresca indefinidamente
		if(buffer->totalPages == leastPage) buffer->totalPages--;
		
		if(!(insertBuffer(buffer, leftChildCpy, MODIFIED) && insertBuffer(buffer, parentCpy, MODIFIED))) return - 2;

	}
	else{
		//vai concatenar com irmao direito

		//fazendo copia do irmao direito
		btpage* rightChild = searchPage(buffer, parentCpy->child[child + 1], MODIFIED);
		if(rightChild == NULL) return -2;
		btpage* rightChildCpy = createPage();
		memcpy(rightChildCpy, rightChild, sizeof(btpage));
		
		//recuperando o nodo esquerdo
		btpage* leftChild = searchPage(buffer, parentCpy->child[child], MODIFIED);
		if(leftChild == NULL) return -2;
		
		//novamente, basta preencher a partir do fim do nodo esquerdo
		leftChild->key[leftChild->keycount].codINEP = parentCpy->key[child].codINEP;
		leftChild->key[leftChild->keycount++].rrn = parentCpy->key[child].rrn;
		
		//termina de copiar
		for(int i = 0; i < rightChildCpy->keycount; i++){
			leftChild->key[leftChild->keycount].codINEP = rightChildCpy->key[i].codINEP;
			leftChild->key[leftChild->keycount].rrn = rightChildCpy->key[i].rrn;
			leftChild->child[leftChild->keycount++] = rightChildCpy->child[i];
		}
		leftChild->child[leftChild->keycount] = rightChildCpy->child[rightChildCpy->keycount];
		
		//precisa shiftar as chaves e ponteiros do pai pra esquerda 1 unidade
		for(int i = child; i < parentCpy->keycount - 1; i++){
			parentCpy->key[i].codINEP = parentCpy->key[i + 1].codINEP;
			parentCpy->key[i].rrn = parentCpy->key[i + 1].rrn;
			parentCpy->child[i] = parentCpy->child[i + 1];
		}
		parentCpy->child[parentCpy->keycount - 1] = parentCpy->child[parentCpy->keycount--];
		
		int rightPageNum = rightChildCpy->pageNum;
		
		//precisa tirar a direita do buffer antes para nao ter duplicata
		deletePage(buffer, rightPageNum);
		free(rightChildCpy);

		btpage *leftChildCpy = createPage();
		memcpy(leftChildCpy, leftChild, sizeof(btpage));
		deletePage(buffer, leftChild->pageNum);
		
		//definindo nmr da pagina como a menor das duas
		leastPage = (leftChildCpy->pageNum > rightPageNum) ?  rightPageNum : leftChildCpy->pageNum;

		leftChildCpy->pageNum = leastPage;
		
		//atualizando ponteiro do pai para apontar para a pagina concatenada
		parentCpy->child[child] = leftChildCpy->pageNum;
		//garante que o numero total de paginas nao cresca indefinidamente
		if(buffer->totalPages == leastPage) buffer->totalPages--;

		if(!(insertBuffer(buffer, leftChildCpy, MODIFIED) && insertBuffer(buffer, parentCpy, MODIFIED))) return - 2;
	}
	
	//se a quantidade de nodos for menor que a ocupacao minima
	if(parentCpy->keycount < TREEORDER/2 - 1){
		//se for raiz e diferente de 0, nao tem problema
		if(parentCpy->pageNum == buffer->pageNums[0] && parentCpy->keycount > 0) return -1;
		//senao propaga concatenacao
		return leastPage;
	}
	return -1;
}

/*
* Faz a remocao ( de fato ) de uma chave da pagina.
* Eh recursiva para o caso onde a remocao foi iniciada num nodo nao folha.
* Portanto, eh responsavel tambem por realizar a troca com a chave folha nesses casos
* 
*\param btpage *leaf : folha que tera chave removida
*\int removePos : posicao da chave que sera removida na pagina da filha
*\int rmPageNum : numero da pagina onde ocorrera a remocao.
*/
int removeKey(bufferpool* buffer, btpage* leaf, int removePos, int rmPageNum){

	if(rmPageNum != -1){
		if(leaf->child[leaf->keycount] != -1){	//continua procurando o predecessor
			int parentPageNum = leaf->pageNum;	//guardando o numero da pagina para volta da recursao
			int underflow = removeKey(buffer, searchPage(buffer, leaf->child[leaf->keycount], ACCESSED), removePos, rmPageNum);
			//filho acessado tera sido sempre o apontado pelo ultimo ponteiro

			if(underflow > -1){
				btpage* parent = searchPage(buffer, parentPageNum, MODIFIED);
				int retVal = redistrib(buffer, parent, parent->keycount); 
				
				if(retVal == 0){
					//precisa garantir que parent nao foi liberado na redistribuicao
					parent = searchPage(buffer, parentPageNum, MODIFIED);
					return concat(buffer, parent, parent->keycount);
				}
				 //as opcoes de retorno restantes sao -1 se concatenou e resolveu, ou -2 erro
				return retVal > 0 ? -1 : -2;
			}
			
			return underflow;	//0 se nao houve nenhum underflow. -1 se houve erro
		}
		else{

			btpage* auxpage = createPage();

			memcpy(auxpage, leaf, sizeof(btpage));

			//armazenando dados para troca		
			treekey successorkey;

			btpage* successor = searchPage(buffer, rmPageNum, MODIFIED);

			successorkey.codINEP = successor->key[removePos].codINEP;
			successorkey.rrn = successor->key[removePos].rrn;

			//fazendo swap. A pagina ja eh alterada direto no buffer
			successor->key[removePos].codINEP = auxpage->key[auxpage->keycount - 1].codINEP;
			successor->key[removePos].rrn = auxpage->key[auxpage->keycount - 1].rrn;

			auxpage->key[auxpage->keycount - 1].codINEP = successorkey.codINEP;
			auxpage->key[auxpage->keycount - 1].rrn = successorkey.rrn;
			
			//a auxiliar precisa ser salva no buffer. 
			insertBuffer(buffer, auxpage, MODIFIED);

			return removeKey(buffer, auxpage, auxpage->keycount - 1, -1);
		}
	}
	
	leaf->keycount--;
	for(int i = removePos; i < leaf->keycount; i++)
		leaf->key[i] = leaf->key[i+1];
	
	//verifica se o estado final do nodo satisfaz ocupacao minima. retorna dizendo se deu underflow.
	return leaf->keycount < (TREEORDER/2) - 1 ? 1 : -1;
}

/*
* Funcao recursiva principal da remocao. Realiza busca da chave a remover, chamando a removeKey para ela
* ou retornando caso nao encontre a chave. Trata o underflow na volta da recursao, chamando redistribuicao e 
* concatenacao.
*
* \param btpage *root : pagina de disco onde sera feita a pesquisa pela chave
* \param int codINEP : chave do indice que se deseja remover
* \param unsignedChar removeType : acionara remocao do registro do arquivo de dados quando for passado valor diferente de 0.
*				Fara a remocao apenas do arquivo de indice se for passado o valor 0
* \return int : -1 quando terminou com underflow na raiz. -2 quando houve erro na remocao. -3 quando a chave nao foi encontrada na arvore.
*/
int findRemovePos(bufferpool* buffer, btpage* root, int codINEP, unsigned char removeType){

	int i = 0, halt = 0, rootPageNum = root->pageNum, underflow;
	
	while(i < root->keycount && !halt){

		if(codINEP <= root->key[i].codINEP)
			halt = 1;
		else
			i++;
	}
	
	if(halt && codINEP == root->key[i].codINEP){	//encontrou a posicao de remocao
		
		if (removeType == REMOVE_FROM_DISK){
			int funcResult = removeReg(root->key[i].rrn);//faz remocao logica do registro no arquivo de dados
			if( !funcResult ) return -2; //falha no processamento do arquivo de dados. retorna erro.
		}
		btpage* parent = NULL;

		if(root->child[i] == -1){	//ja eh folha
			 return removeKey(buffer, root, i, -1);
		}
		else{	//nao eh folha, vai buscar substituicao pelo predecessor
			int predecPageNum = root->child[i];
			underflow = removeKey(buffer, searchPage(buffer, root->child[i], ACCESSED), i, rootPageNum);
			//precisa tratar esse primeiro retorno a parte
				
			if(underflow > -1){	//deu underflow
				
				btpage* parent = searchPage(buffer, rootPageNum, MODIFIED);
				int retVal = redistrib(buffer, parent, i);
				
				if(!retVal){	//redistribuicao nao funcionou. vai concatenar.
					parent = searchPage(buffer, rootPageNum, MODIFIED);
					return  concat(buffer, parent, i); //se propagou, retorna nmr da pagina atual
				}
				return retVal > 0 ? -1 : -2;
				//se retVal retornou -2, houve erro.
			}
			return underflow;	//0 se nao houve underflow, retorna 0. se for < 0 , deu erro, retorna -1.
		}
	}

	else if(root->child[i] == -1) return -3;	//chave não encontrada
	
	//vai continuar buscando pela chave
	underflow = findRemovePos(buffer, searchPage(buffer, root->child[i], ACCESSED), codINEP, removeType);

	//retorno da recursao entra aqui
	
	if(underflow > -1){	//deu underflow
		btpage *parent = searchPage(buffer, rootPageNum, MODIFIED);
		int retVal = redistrib(buffer, parent, i);

		if(retVal == 0){ //nao conseguiu redistribuir, entao concatena
			
			parent = searchPage(buffer, rootPageNum, MODIFIED);
			return concat(buffer, parent, i);	//se propagou, retorna nmr da pagina atual
		}
		
		// se fez resditribuicao nao precisa fazer nada(retorna -1). se deu erro retorna -2
		return retVal > 0 ? -1 : -2;
	}
	if (!underflow) return 0;	//retorno da recursao apos remocao e ajustes
	return underflow;

}

/*
* Funcao que da inicio a remocao. Recupera a raiz do buffer iniciando a busca recursiva pelo
* no a ser removido. Ela trata o retorno da funcao indicando se houve underflow na raiz.
*/
int BtreeRemove(int codINEP, unsigned char removeType){

	bufferpool* buffer = loadBuffer();
	
	btpage* root = getRootPage(buffer);
	int rootPageNum = root->pageNum;
	
	int underflow = findRemovePos(buffer, root, codINEP, removeType);
	
	if(underflow > -1){	//diminuiu a altura da arvore. Nova arvore eh o valor retornado em underflow
		if(buffer->treeHeight > 0){	//mas somente se a raiz nao for a folha
		
			//remove raiz do buffer
			free(buffer->page[0]);
			
			//escolhendo menor numero pra nova raiz
			rootPageNum = ( rootPageNum < underflow ) ? rootPageNum : underflow; 
			
			//recuperando e criando copia da nova raiz
			btpage *newRoot = createPage();
			memcpy(newRoot, searchPage(buffer, underflow, ACCESSED), sizeof(btpage));
			//nova raiz pode ja estar no buffer. necessario remover duplicata
			deletePage(buffer, underflow);
			newRoot->pageNum = rootPageNum;

			//atribuindo nova raiz
			buffer->page[0] = newRoot;
			buffer->pageNums[0] = rootPageNum;
			buffer->treeHeight--;
		}
	}
	
	saveAllPages(buffer);
	free(buffer);

	if ( underflow == -2) return -1;//erro na remocao
	if ( underflow == -3) return 0;//chave nao encontrada
	return 1;//remocao bem-sucedida
}


