#include "common\tp_common.h"

/* ================================================================== */

/* Definição de Valores Globais */

/*	Timeout de operações de espera de Semáforos/Mutexes 
*	(Evita travamento) do programa */
#define TIMEOUT_MS 400

/*	Tamanho do campo de controle contido no arquivo de lista em disco 
*	Campo DWORD contendo número de entradas não lidas na lista */
#define OVERHEAD_CONTROLE (sizeof(DWORD)+sizeof(BOOL))

/* ================================================================== */

/* Cabeçalho de funções */

/*	Inicializa todos os objetos que serão utilizados no processo.
*	Os objetos inicializados são associados aos handles declarados
*	como variáveis globais. */
void inicializaObjetosP4(void);

/*	Fecha todos os handles de objetos abertos pela função de inicialização. */
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