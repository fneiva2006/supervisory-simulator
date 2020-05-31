#include <stdio.h>
#include <stdlib.h> // calloc()
#include <string.h>
#include <math.h>
#include <windows.h> // winapi
#include <process.h> // _begin e _endthreadex()
#include <conio.h>  // _getch()
#include <time.h>  
#include <wchar.h>
#include <locale.h> //necessário para usar setlocale

#define _CHECKERROR	1
#include "CheckForError.h"

// definição de casts auxiliares
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

/* ================================================================== */

/* Estrutura para armazenar mensagem de status formatada */
typedef struct 
{
	char op[7];
	char linha[2];
	char tipo[2];
	char nseq[6];
	char tbat[3];
	char nbat[3];
	char estagio[3];
	char prod[8];
	char timeStamp[9];

} MensagemStatus_t;

/* Estrutura para armazenar mensagem de Ordens de Produção formatada */

typedef struct 
{
	char nseq[6];
	char op[7];
	char tbat[3];
	char linha[2];
	char tipo[2];
	char receita[3];

} MensagemOP_t;

/* ================================================================== */

/* Definição de Valores Globais */

/* Numero Linhas de produção (CLP's) */
#define N_LINHAS 2

/* Status */
#define TAMANHO_LISTA_STATUS 500 
#define TAMANHO_MSG_STATUS 42

/* Ordens de Produção */
#define TAMANHO_LISTA_OP 200
#define TAMANHO_MSG_ORDEM_PRODUCAO 22

/* ================================================================== */

/* Definição dos nomes que serão usados nos objetos globais desse módulo */

/* Evento Exit Global */
#define GLOBAL_EXIT_EVENT TEXT("GLOBAL_EXIT_EVENT") 

/* P1 Control Objects */
#define LEITURA_MES_SEM TEXT("LEITURA_MES_SEM") // Semaforo On/Off MES
const LPCWSTR LEITURA_CLP_SEM[] = {TEXT("LEITURA_CLP_SEM_1"),TEXT("LEITURA_CLP_SEM_2")};// Semaforo On/Off CLPs

/* P2 Control Objects */
#define CAP_MSG_SEM TEXT("CAP_MSG_SEM")

/* P3 Control Objects */
#define EXIBE_STATUS_SEM TEXT("EXIBE_STATUS_SEM")

/* P4 Control Objects */
#define EXIBE_OP_SEM TEXT("EXIBE_OP_SEM")

/* ================================================================== */

/* Pipes PATHS */
#define CAMINHO_PIPE_DISP_MES TEXT("\\\\.\\pipe\\dispMES") // pipe MES
const LPCWSTR CAMINHO_PIPE_DISP_CLPS[] = {TEXT("\\\\.\\pipe\\dispCLP1"), 
		TEXT("\\\\.\\pipe\\dispCLP2")}; // pipe CLP 1 e 2

/* ================================================================== */

/* Status List NAMES */
#define STATUS_LIST_MAPPED_FILE TEXT("STATUS_LIST_MAPPED_FILE")
#define PERMISSAO_LEITURA_STATUS_SEM TEXT("PERMISSAO_LEITURA_STATUS_SEM")
#define PERMISSAO_ESCRITA_STATUS_SEM TEXT("PERMISSAO_ESCRITA_STATUS_SEM")
#define LISTA_STATUS_MUTEX TEXT("LISTA_STATUS_MUTEX")

/* OP List NAMES */
#define OP_LIST_MAPPED_FILE TEXT("OP_LIST_MAPPED_FILE")
#define PERMISSAO_LEITURA_OP_SEM TEXT("PERMISSAO_LEITURA_OP_SEM")
#define PERMISSAO_ESCRITA_OP_SEM TEXT("PERMISSAO_ESCRITA_OP_SEM")
#define LISTA_OP_MUTEX TEXT("LISTA_OP_MUTEX")

/* ================================================================== */

/* P4 - OP DISK List NAMES */
#define OP_DISK_LIST TEXT("..\\ListaOP\\op_disk_list.txt")
#define COPIA_OP_DISK_LIST TEXT("..\\CopiaListaOP\\copia_op_disk_list.txt")

#define OP_DISK_LIST_DIR TEXT("..\\ListaOP")
#define COPIA_OP_DISK_LIST_DIR TEXT("..\\CopiaListaOP")


