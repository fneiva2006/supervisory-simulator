#include "common\tp_common.h"

/* ================================================================== */

/* Defini��o de Valores Globais */

/*	Timeout de opera��es de espera de Sem�foros/Mutexes 
*	(Evita travamento) do programa */
#define TIMEOUT_MS 400

/*	Tamanho do campo de controle contido no arquivo de lista em disco 
*	Campo DWORD contendo n�mero de entradas n�o lidas na lista */
#define OVERHEAD_CONTROLE (sizeof(DWORD)+sizeof(BOOL))

/* ================================================================== */

/* Cabe�alho de fun��es */

/*	Inicializa todos os objetos que ser�o utilizados no processo.
*	Os objetos inicializados s�o associados aos handles declarados
*	como vari�veis globais. */
void inicializaObjetosP4(void);

/*	Fecha todos os handles de objetos abertos pela fun��o de inicializa��o. */
void fechaHandlesP4(void);

/* Exibe mensagem de OP formatada */
void exibeMensagemOP(const char* msg);

/* Salva OP em disco */
void salvaOPEmDisco(const char* msg);

/*	Inicializa campos de controle da Lista de OP's salva em disco caso o arquivo tenha
*	tenha acabado de ser criado */
void inicializaListaOPEmDisco();

/* Exibe lista completa de OP's contida na lista circular em disco */
void exibeListaOPEmDisco();