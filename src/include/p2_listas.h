#include "common\tp_common.h"

/* Definição de Valores Globais */

/* Timeout em ms Padrao Operações Wait */
#define TIMEOUT_MS 400 

/* --------------------------------------------------------------------------------- */

/* Cabeçalho de funções */

/*	Thread de captura de mensagens de status dos CLP's. Salva as Mensagens Capturadas
*	numa lista em memória compartilhada. */
DWORD WINAPI captura_status(LPVOID linha);

/*	Thread de captura de mensagens de Ordens de Produção do MES. Salva as Mensagens 
*	Capturadas numa lista em memória compartilhada. */
DWORD WINAPI captura_OPs();

/*	Inicializa (abre e/ou cria) todos os objetos que serão utilizados no processo.
*	Os objetos inicializados são associados aos handles declarados como variáveis 
*	globais. */
void inicializaObjetosP2(void);

/*	Fecha todos os handles de objetos abertos pela função de inicialização. */
void fechaHandlesP2(void);