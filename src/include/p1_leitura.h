#include "common\tp_common.h"

/* Cabe�alho de fun��es */

/* Thread de gera��o de mensagens de status dos CLP's. */
DWORD WINAPI le_clp(LPVOID id_clp);

/* Thread de gera��o de mensagens de Ordens de Produ��o do MES. */
DWORD WINAPI le_MES();

/*	Inicializa todos os objetos que ser�o utilizados no processo.
*	Os objetos inicializados s�o associados aos handles declarados
*	como vari�veis globais. */
void inicializaObjetosP1(void);

/*	Fecha todos os handles de objetos abertos pela fun��o de inicializa��o. */
void fechaHandlesP1(void);
