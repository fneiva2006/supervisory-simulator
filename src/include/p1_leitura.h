#include "common\tp_common.h"

/* Cabeçalho de funções */

/* Thread de geração de mensagens de status dos CLP's. */
DWORD WINAPI le_clp(LPVOID id_clp);

/* Thread de geração de mensagens de Ordens de Produção do MES. */
DWORD WINAPI le_MES();

/*	Inicializa todos os objetos que serão utilizados no processo.
*	Os objetos inicializados são associados aos handles declarados
*	como variáveis globais. */
void inicializaObjetosP1(void);

/*	Fecha todos os handles de objetos abertos pela função de inicialização. */
void fechaHandlesP1(void);
