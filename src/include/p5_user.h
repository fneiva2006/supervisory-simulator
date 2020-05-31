#include "common\tp_common.h"

#define ESC 0x1B // defini��o tecla ESC

/*	Timeout de opera��o de Liga/Desliga dos sem�foros de controle de opera��o dos m�dulos */	
#define ON_OFF_TIMEOUT_MS 10000 

// defini��o do estado inicial dos modulos
#define DEVICES_INITIAL_STATE FALSE 

const char* LIG_DESL_TEXT[] = {"desligado","ligado"};

/* Tamanho do campo de controle contido no arquivo de lista em disco */
#define OVERHEAD_CONTROLE (sizeof(DWORD)+sizeof(BOOL))

/* ================================================================== */

/* PROCESSOS  */

#define N_PROCESSOS 4
enum { P1_LEITURA = 0, P2_LISTAS, P3_STATUS, P4_OPS};

/* PATHS */

/*	Caminho dos execut�veis dos processos chamados */
const LPCWSTR CAMINHO_EXECUTAVEIS[] = {	TEXT("p1_leitura.exe"),
										TEXT("p2_listas.exe"),
										TEXT("p3_status.exe"),
										TEXT("p4_ops.exe") };

/*	Working Dir dos processos chamados */
const LPCWSTR CAMINHO_WDIR[] = {	TEXT("."),
									TEXT("."),
									TEXT("."),
									TEXT(".")	};


/*	T�tulos das janelas dos processos da aplica��o */
const LPCWSTR PROCESSES_TITLES[] = {	TEXT(""), 
										TEXT(""),
										TEXT("TP ATR - Exibi��o de Mensagens de Status"), 
										TEXT("TP ATR - Exibi��o de Ordens de Produ��o")		};


/* Console Inicializa�ao Processos */
const DWORD CRIA_CONSOLE_PROCESSO[] = {	0,					0,
										CREATE_NEW_CONSOLE, CREATE_NEW_CONSOLE};

/* ================================================================== */

/* Cabe�alho de fun��es */

/*	Inicializa todos os objetos que ser�o utilizados no processo.
*	Os objetos inicializados s�o associados aos handles declarados
*	como vari�veis globais. */
void inicializaObjetosP5(void);


/*	Fecha todos os handles de objetos abertos pela fun��o de inicializa��o. */
void fechaHandlesP5(void);

/*	Thread de m�dulo de leitura do teclado */
DWORD WINAPI le_teclado();

/*	Espera ou sinaliza sem�foros 
*	Utilizada para controle dos sem�foros Liga/Desliga dos m�dulos da aplica��o
*	Estado Ligado/Desligado � salvo na vari�vel booleana passada no segundo argumento */
void switch_device_sem(HANDLE sem, const char* device_str, BOOL* on, const DWORD count);

/*	Adquire e exibe OP mais antiga salva em disco */
void exibeOPMaisAntigaDisco();

/*	Exibe OP formatada na tela */
void exibeMensagemOP(const char* msg);

/*	Imprime comandos da aplica��o na tela */
void exibe_comandos();

/*	Rotina WINAPI de tratamento de eventos de janelas
*	Utilizada para detectar o evento de bot�o de fechar janela pressionado 
*	Caso o bot�o de janela seja pressionado, essa rotina sinaliza o evento de
*	encerramento global da aplica��o, permitindo que a mesma se encerre de maneira
*	correta, isto �, fechando todos os objetos de maneira n�o-abrupta */
BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType);

