#include "common\tp_common.h"

/* ================================================================== */

/* Defini��o de Valores Globais */

/*	Timeout de opera��es de espera de Sem�foros/Mutexes 
*	(Evita travamento) do programa */
#define TIMEOUT_MS 400

/* ================================================================== */

/* Cabe�alho de fun��es */

/*	Inicializa todos os objetos que ser�o utilizados no processo.
*	Os objetos inicializados s�o associados aos handles declarados
*	como vari�veis globais. */
void inicializaObjetosP3(void);

/*	Fecha todos os handles de objetos abertos pela fun��o de inicializa��o. */
void fechaHandlesP3(void);

/* Exibe mensagem formatada de status no console */
void exibeMensagemStatus(char* msg);