#include "common\tp_common.h"

/* ================================================================== */

/* Definição de Valores Globais */

/*	Timeout de operações de espera de Semáforos/Mutexes 
*	(Evita travamento) do programa */
#define TIMEOUT_MS 400

/* ================================================================== */

/* Cabeçalho de funções */

/*	Inicializa todos os objetos que serão utilizados no processo.
*	Os objetos inicializados são associados aos handles declarados
*	como variáveis globais. */
void inicializaObjetosP3(void);

/*	Fecha todos os handles de objetos abertos pela função de inicialização. */
void fechaHandlesP3(void);

/* Exibe mensagem formatada de status no console */
void exibeMensagemStatus(char* msg);