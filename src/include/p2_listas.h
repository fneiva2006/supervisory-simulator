#include "common\tp_common.h"

/* Defini��o de Valores Globais */

/* Timeout em ms Padrao Opera��es Wait */
#define TIMEOUT_MS 400 

/* --------------------------------------------------------------------------------- */

/* Cabe�alho de fun��es */

/*	Thread de captura de mensagens de status dos CLP's. Salva as Mensagens Capturadas
*	numa lista em mem�ria compartilhada. */
DWORD WINAPI captura_status(LPVOID linha);

/*	Thread de captura de mensagens de Ordens de Produ��o do MES. Salva as Mensagens 
*	Capturadas numa lista em mem�ria compartilhada. */
DWORD WINAPI captura_OPs();

/*	Inicializa (abre e/ou cria) todos os objetos que ser�o utilizados no processo.
*	Os objetos inicializados s�o associados aos handles declarados como vari�veis 
*	globais. */
void inicializaObjetosP2(void);

/*	Fecha todos os handles de objetos abertos pela fun��o de inicializa��o. */
void fechaHandlesP2(void);