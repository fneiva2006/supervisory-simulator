#include "common\tp_common.h"

#define ESC 0x1B // definição tecla ESC

/*	Timeout de operação de Liga/Desliga dos semáforos de controle de operação dos módulos */	
#define ON_OFF_TIMEOUT_MS 10000 

// definição do estado inicial dos modulos
#define DEVICES_INITIAL_STATE FALSE 

const char* LIG_DESL_TEXT[] = {"desligado","ligado"};

/* Tamanho do campo de controle contido no arquivo de lista em disco */
#define OVERHEAD_CONTROLE (sizeof(DWORD)+sizeof(BOOL))

/* ================================================================== */

/* PROCESSOS  */

#define N_PROCESSOS 4
enum { P1_LEITURA = 0, P2_LISTAS, P3_STATUS, P4_OPS};

/* PATHS */

/*	Caminho dos executáveis dos processos chamados */
const LPCWSTR CAMINHO_EXECUTAVEIS[] = {	TEXT("p1_leitura.exe"),
										TEXT("p2_listas.exe"),
										TEXT("p3_status.exe"),
										TEXT("p4_ops.exe") };

/*	Working Dir dos processos chamados */
const LPCWSTR CAMINHO_WDIR[] = {	TEXT("."),
									TEXT("."),
									TEXT("."),
									TEXT(".")	};


/*	Títulos das janelas dos processos da aplicação */
const LPCWSTR PROCESSES_TITLES[] = {	TEXT(""), 
										TEXT(""),
										TEXT("TP ATR - Exibição de Mensagens de Status"), 
										TEXT("TP ATR - Exibição de Ordens de Produção")		};


/* Console Inicializaçao Processos */
const DWORD CRIA_CONSOLE_PROCESSO[] = {	0,					0,
										CREATE_NEW_CONSOLE, CREATE_NEW_CONSOLE};

/* ================================================================== */

/* Cabeçalho de funções */

/*	Inicializa todos os objetos que serão utilizados no processo.
*	Os objetos inicializados são associados aos handles declarados
*	como variáveis globais. */
void inicializaObjetosP5(void);


/*	Fecha todos os handles de objetos abertos pela função de inicialização. */
void fechaHandlesP5(void);

/*	Thread de módulo de leitura do teclado */
DWORD WINAPI le_teclado();

/*	Espera ou sinaliza semáforos 
*	Utilizada para controle dos semáforos Liga/Desliga dos módulos da aplicação
*	Estado Ligado/Desligado é salvo na variável booleana passada no segundo argumento */
void switch_device_sem(HANDLE sem, const char* device_str, BOOL* on, const DWORD count);

/*	Adquire e exibe OP mais antiga salva em disco */
void exibeOPMaisAntigaDisco();

/*	Exibe OP formatada na tela */
void exibeMensagemOP(const char* msg);

/*	Imprime comandos da aplicação na tela */
void exibe_comandos();

/*	Rotina WINAPI de tratamento de eventos de janelas
*	Utilizada para detectar o evento de botão de fechar janela pressionado 
*	Caso o botão de janela seja pressionado, essa rotina sinaliza o evento de
*	encerramento global da aplicação, permitindo que a mesma se encerre de maneira
*	correta, isto é, fechando todos os objetos de maneira não-abrupta */
BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType);

