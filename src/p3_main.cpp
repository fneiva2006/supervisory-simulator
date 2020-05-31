#include "p3_status.h"

/* Control HANDLES */
HANDLE ghExitEvent = NULL;					// Evento encerramento do programa
HANDLE ghExibeStatusSem = NULL;				// Sem�foro liga/desliga m�dulo

/* Status List HANDLES */
HANDLE ghListaStatusMappedFile = NULL;		// Arquivo Mapeado em Mem�ria
HANDLE ghPermissaoLeituraStatusSem = NULL;	// Sem�foro Permiss�o de Leitura Lista
HANDLE ghPermissaoEscritaStatusSem = NULL;	// Sem�foro Permiss�o de Escrita Lista
HANDLE ghListaStatusMutex = NULL;			// Mutex lista


int main(int argc, char* argv[])
{
	setlocale(LC_ALL,"");

	/* Adquire Handle de janela de console */
	HWND ptr = GetConsoleWindow();

	/* Posiciona janela do console em local inicial */
	MoveWindow(ptr,670,400,720,400, true);

	/* Desativa Bot�o de Fechar da Janela do Console */
	EnableMenuItem(GetSystemMenu(ptr, FALSE), SC_CLOSE,
		MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	inicializaObjetosP3(); // inicializa objetos

	DWORD indice_leitura = 0; // Posi��o da lista de Status a ser lida

	char *sm_ptr; // Ponteiro auxiliar para captura de mensagens da lista

	/* Buffer auxiliar para armazenar as mensagens lidas da lista */
	char buffer[TAMANHO_MSG_STATUS+1]; 

	/* Endere�o base relativo ao in�cio da lista de status compartilhada em mem�ria */
	const char* base_sm_addr = (char*) MapViewOfFile(	ghListaStatusMappedFile,
														FILE_MAP_READ | FILE_MAP_WRITE,
														0,
														0,
														(TAMANHO_LISTA_STATUS*TAMANHO_MSG_STATUS) + sizeof(DWORD)	);
	
	/*	Adquire ponteiro para campo da lista que indica quantas entradas da mesma
	*	ainda n�o foram lidas */
	DWORD* n_entradas_lista = (DWORD*) &base_sm_addr[TAMANHO_LISTA_STATUS*TAMANHO_MSG_STATUS];
	
	/* Array auxiliar de handles */
	HANDLE hSinc[] = { ghExitEvent, ghExibeStatusSem };
	
	while(1)
	{
		/*	Aguarda Evento de Encerramento Global ou Estado de Exibi��o Ligado
			(Semaforo de Controle on/off do m�dulo) */
		if(WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0) break;

		/*	Aguarda Permiss�o de Leitura da Lista (Sem�foro de Recurso Compartilhado
			da lista). Opera��o com timeout */
		if( WaitForSingleObject(ghPermissaoLeituraStatusSem,TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			/*	Libera Semaforo de Controle on/off do m�dulo de Exibi��o de Mensagens de Status*/
			ReleaseSemaphore(ghExibeStatusSem,1,NULL); continue;
		}

		/*	Aguarda Condi��o de exclus�o m�tua para Leitura da Lista 
			(Mutex de Recurso Compartilhado da lista). Opera��o com timeout */
		if( WaitForSingleObject(ghListaStatusMutex,TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			/*	Libera Semaforo de Controle on/off do m�dulo de Exibi��o de Mensagens de Status*/
			ReleaseSemaphore(ghExibeStatusSem,1,NULL); continue;
		}

		/*	Adquire posi��o da mem�ria da pr�xima mensagem da lista ser lida */
		sm_ptr = (char*) &base_sm_addr[indice_leitura*TAMANHO_MSG_STATUS];

		/*	Extrai e salva no buffer mensagem de status localizada no endere�o 
			de memoria obtido anteriormente*/
		strncpy(buffer,sm_ptr,TAMANHO_MSG_STATUS);
		buffer[TAMANHO_MSG_STATUS] = '\0';

		/*	Exibe Mensagem Extra�da da Lista */
		exibeMensagemStatus(buffer);

		/*	Decrementa n�mero de entradas da lista que ainda n�o foram lidas */
		*n_entradas_lista = *n_entradas_lista - 1;

		/*	Atualiza �ndice para a leitura da pr�xima mensagem da lista*/
		indice_leitura = (indice_leitura+1)%TAMANHO_LISTA_STATUS;	

		/*	Libera 1 permiss�o de escrita na lista */
		ReleaseSemaphore(ghPermissaoEscritaStatusSem,1,NULL);

		/*	Libera Mutex de controle do acesso a lista */
		ReleaseMutex(ghListaStatusMutex);

		/*	Libera Semaforo de Controle on/off do m�dulo de Exibi��o de Mensagens de Status */
		ReleaseSemaphore(ghExibeStatusSem,1,NULL);
	}

	UnmapViewOfFile(sm_ptr); /*  Fecha vis�o da lista compartilhada em mem�ria */

	fechaHandlesP3(); // fecha handles

	return 0;
}

void inicializaObjetosP3(void)
{
	/* Control HANDLES */

	/* Abre o evento de encerramento global da aplica��o */
	ghExitEvent = (HANDLE) OpenEvent(EVENT_ALL_ACCESS,FALSE,GLOBAL_EXIT_EVENT);
	CheckForError(ghExitEvent);

	/* Abre o semaforo de controle on/off do modulo de exibi��o das mensagens de status*/
	ghExibeStatusSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,EXIBE_STATUS_SEM);
	CheckForError(ghExibeStatusSem);

/* ----------------------------------------------------------------------- */

	/* Status List HANDLES */

	/* Abre o handle para a lista de status mapeada em memoria*/
	do
	{
		ghListaStatusMappedFile = (HANDLE) OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,STATUS_LIST_MAPPED_FILE);
	
	} while ( ghListaStatusMappedFile == INVALID_HANDLE_VALUE);


	/* Abre os semaforos de permissao de leitura/escrita da lista de status compartilhada em memoria */
	do
	{	
		 ghPermissaoLeituraStatusSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,
													FALSE,
													PERMISSAO_LEITURA_STATUS_SEM);
		 ghPermissaoEscritaStatusSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,
													FALSE,
													PERMISSAO_ESCRITA_STATUS_SEM);

	} while( ( ghPermissaoEscritaStatusSem == INVALID_HANDLE_VALUE) || ( ghPermissaoLeituraStatusSem == INVALID_HANDLE_VALUE ));
	
	
	/* Abre Mutex da Lista em de Status em Memoria Compartilhada */
	do
	{
		ghListaStatusMutex= (HANDLE) OpenMutex(MUTEX_ALL_ACCESS,FALSE, LISTA_STATUS_MUTEX);

	} while(ghListaStatusMutex == INVALID_HANDLE_VALUE);
}

void fechaHandlesP3(void)
{
	/*	Fechamento de todos os handles utilizados no c�digo */

	/* Control HANDLES */
	CloseHandle(ghExitEvent);
	CloseHandle(ghExibeStatusSem);
	
	/* Status List HANDLES */
	CloseHandle(ghListaStatusMappedFile);
	CloseHandle(ghPermissaoLeituraStatusSem);
	CloseHandle(ghPermissaoEscritaStatusSem);
	CloseHandle(ghListaStatusMutex);
}

void exibeMensagemStatus(char* msg)
{
	MensagemStatus_t Mensagem;
	
	strcpy(Mensagem.op, strtok ( msg, ";"));
	strcpy(Mensagem.linha, strtok ( NULL, ";"));
	strcpy(Mensagem.tipo, strtok ( NULL, ";"));
	strcpy(Mensagem.nseq, strtok ( NULL, ";"));
	strcpy(Mensagem.tbat, strtok ( NULL, ";"));
	strcpy(Mensagem.nbat, strtok ( NULL, ";"));
	strcpy(Mensagem.estagio, strtok ( NULL, ";"));
	strcpy(Mensagem.prod, strtok ( NULL, ";"));
	strcpy(Mensagem.timeStamp, strtok ( NULL, ";"));

	printf("NSEQ: %s OP: %s LINHA: %s TIPO: %s TB: %s NB: %s EST: %s PROD: %s %s\n",
		Mensagem.nseq, Mensagem.op, Mensagem.linha, Mensagem.tipo, Mensagem.tbat,
		Mensagem.nbat, Mensagem.estagio, Mensagem.prod, Mensagem.timeStamp);

}