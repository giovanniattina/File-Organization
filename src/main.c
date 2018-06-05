/* Nome
 * Danilo Henrique Cordeiro           - 6791651
 * Giovanni Attina do Nascimento      - 9286614
 * Lucas Noriyuki Yoshida             - 10262586
 * Sergio Ricardo Gomes Barbosa Filho - 10408386
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "arquivos.h"
#include "BtreeIndex.h"

int main(int argc, char **argv){
	int opt = 0, RRN, funcResult, codInep;
	char *fileName;
	char campo[70], *valor;
	int valorCampo1;
	int sizeStack = 0, *stack;
	char valorCampo2[13], valorCampo3[5], valorCampo4[70], valorCampo5[70], valorCampo6[70];

	if(argc < 2){
		printf("Uso: %s funcionalidade nParametros\n", argv[0]);
		return EXIT_FAILURE;
	}

	opt = atoi(argv[1]);
	
	//funcionalidade 10 equivalente a 1 e funcionalidade 11 equivalente a 6
	opt = (opt == 10) ? 1 : opt;
	opt = (opt == 11) ? 6 : opt;

	switch(opt){

		case 1:		//carregar arquivos csv
			if(argc < 3){
				printf("Uso: %s 1 'arquivo.csv'\n", argv[0]);
				return EXIT_FAILURE;
			}

			fileName = argv[2];
			if (!readFile(fileName))
				printf("Falha no carregamento do arquivo.\n");
			else
				printf("Arquivo carregado.\n");
		break;

		case 2:		//ler arquivo binario
			funcResult = showAll();

			if(!funcResult)
				printf("Falha no processamento do arquivo.\n");
			else if (funcResult < 0)
       	 		printf("Registro inexistente.\n");
			break;

		case 3:		//recuperar registros por busca de campo

			if(argc < 4){
				printf("Uso: %s 3 'NomeDoCampo' valorDoCampo\n", argv[0]);
				return EXIT_FAILURE;
			}
			strcpy(campo, argv[2]);
			valor = argv[3];

			funcResult = search(campo, valor);
			if(!funcResult)			//retornar 0
				printf("Falha no processamento do arquivo.\n");
			else if (funcResult < 0)		//retornar negativo
				printf("Registro inexistente.\n");
			break;

		case 4:		//recuperar registro por rrn
			if(argc < 3){
				printf("Uso: %s 4 RRN\n", argv[0]);
				return EXIT_FAILURE;
			}
			RRN = atoi(argv[2]);

			funcResult = findRRN(RRN);
			if(!funcResult)
				printf("Falha no processamento do arquivo.\n");
			else if (funcResult < 0)
				printf("Registro inexistente.\n");
			break;


	    case 5:		//remocao logica de registro com uso de pilha

		    if(argc < 3){
		    	printf("Uso: %s 5 RRN\n", argv[0]);
		    	return EXIT_FAILURE;
		    }
		    RRN = atoi(argv[2]);

			funcResult = removeReg(RRN);	//<--- inserir func aqui
			if(!funcResult)
				printf("Falha no processamento do arquivo.\n");
			else if (funcResult < 0)
				printf("Registro inexistente.\n");
			else
				printf("Registro removido com sucesso.\n");
			break;

		case 6:		//insercao de registro com reaproveitamento de espaco

			if(argc < 8){
				printf("Uso: %s 6 codInep 'DD/MM/AAAA' 'UF' 'nomeEscola' 'municipio' 'prestadora'\n", argv[0]);
				return EXIT_FAILURE;
			}

			valorCampo1 = atoi(argv[2]);
			strcpy(valorCampo2, argv[3]);
			strcpy(valorCampo3, argv[4]);
			strcpy(valorCampo4, argv[5]);
			strcpy(valorCampo5, argv[6]);
			strcpy(valorCampo6, argv[7]);

			if(!insertReg(valorCampo1, valorCampo2, valorCampo3, valorCampo4, valorCampo5, valorCampo6))	
				printf("Falha no processamento do arquivo.\n");
			else
				printf("Registro inserido com sucesso.\n");

      		break;

		case 7:		//alteracao de registro

			RRN = atoi(argv[2]);
			valorCampo1 = atoi(argv[3]);
			strcpy(valorCampo2, argv[4]);
			strcpy(valorCampo3, argv[5]);
			strcpy(valorCampo4, argv[6]);
			strcpy(valorCampo5, argv[7]);
			strcpy(valorCampo6, argv[8]);

			funcResult = updateReg(RRN, valorCampo1, valorCampo2, valorCampo3, valorCampo4, valorCampo5, valorCampo6);

			if(!funcResult)
				printf("Falha no processamento do arquivo.\n");
			else if(funcResult < 0)
				printf("Registro inexistente.\n");
			else
				printf("Registro alterado com sucesso.\n");
			break;

		case 8:		//compactacao

			if(compact())
				printf("Arquivo de dados compactados com sucesso.\n");
			else
				printf("Falha no processamento do arquivo.\n");

			break;

		case 9:		//impressao da pilha
			stack = showStack(&sizeStack);

			if(stack == NULL)
				printf("Falha no processamento do arquivo.\n");

			else{
				if(stack[0] == -1)
					printf("Pilha vazia.\n");
				else{
					for(int cont = 0; cont < sizeStack; cont++)
						printf("%d ", stack[cont]);
					printf("\n");
        		}
      		}
			if(stack != NULL)
				free(stack);
			break;
	case 12://recuperacao de registro com base numa chave do arquivo de indice
		codInep = atoi(argv[2]);
		//RRN = BtreeSearch(codInep); <---- implementar
		//verifica se o RRN retornado eh valido
		if(!RRN)
			printf("Falha no processamento do arquivo.\n");
		else if (RRN < 0)
			printf("Registro inexistente.\n");
		else//recupera registro por RRN
			findRRN(RRN);
		break;

	case 13:	//remocao a partir de uma chave do indice primario
		
		codInep = atoi(argv[2]);
		//RRN = BtreeSearch(codInep); <---- implementar
		if(!RRN)
			printf("Falha no processamento do arquivo.\n");
		else if (RRN < 0)
			printf("Registro inexistente.\n");
		else{

			//provavelmente mais eficiente fazer a busca e as remocoes numa funcao so dentro do indice

			removeReg(RRN);
			//removeKeyFromIndex(codInep)
				printf("Registro removido com sucesso.\n");
		}
		break;
	case 14:	//atualizacao dos campos
		
		//pegando parametros
		codInep = atoi(argv[2]);
		valorCampo1 = atoi(argv[3]);
		strcpy(valorCampo2, argv[4]);
		strcpy(valorCampo3, argv[5]);
		strcpy(valorCampo4, argv[6]);
		strcpy(valorCampo5, argv[7]);
		strcpy(valorCampo6, argv[8]);
		
		//encontra rrn no registro
		// RRN = BtreeSearch(codInep);

		if(!RRN)
			printf("Falha no processamento do arquivo.\n");
		else if(RRN < 0)
			printf("Registro inexistente.\n");
		else{ //existe
			
			updateReg(RRN, valorCampo1, valorCampo2, valorCampo3, valorCampo4, valorCampo5, valorCampo6);
			//verificando se mudou a chave
			if (codInep != valorCampo1){
				//remove do indice e insere novamente
				
				//<--- chamar remocao aqui
				insertKeyToIndex(valorCampo1, RRN);
			}
			
			printf("Registro alterado com sucesso.\n");
		}
		break;
	
	default:
		printf("Opcao invalida.\n");
	}

	return EXIT_SUCCESS;	
}
