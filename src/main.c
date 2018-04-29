/* Nomes
 * Danilo Henrique Cordeiro - 6791651
 * Giovanni
 * Lucas
 * Sergio
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <arquivos.h>

int main(int argc, char **argv){
  int opt = 0, RRN;
  char *fileName;
  char *campo, *valor;
  char *valorCampo1, *valorCampo2, *valorCampo3, *valorCampo4, *valorCampo5, *valorCampo6;

  fileName = "turmaB-dadosPBLE.csv";
  opt = atoi(argv[1]);

  switch(opt){
    case 1:
      if(argc == 3){
        fileName = argv[2];
        if (readFile(fileName)) {
          printf("Arquivo carregado.\n");
        }
      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 2:
      if(argc == 2){
        showAll();
      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 3:
      if(argc == 4){
        campo = argv[2];
        valor = argv[3];
      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 4:
      if(argc == 3){
        RRN = atoi(argv[2]);
        findRRN(RRN);
      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 5:
      if(argc == 3){
        RRN = atoi(argv[2]);
      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 6:
      if(argc == 8){
        valorCampo1 = argv[2];
        valorCampo2 = argv[3];
        valorCampo3 = argv[4];
        valorCampo4 = argv[5];
        valorCampo5 = argv[6];
        valorCampo6 = argv[7];
      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 7:
      if(argc == 9){
        RRN = atoi(argv[2]);
        valorCampo1 = argv[3];
        valorCampo2 = argv[4];
        valorCampo3 = argv[5];
        valorCampo4 = argv[6];
        valorCampo5 = argv[7];
        valorCampo6 = argv[8];
      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 8:
      if(argc == 2){

      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    case 9:
      if(argc == 2){

      } else {
        printf("Numero de parametros invalido.");
      }
      break;
    default:
      printf("Opcao invalida.\n");
  }

  return 0;
}
