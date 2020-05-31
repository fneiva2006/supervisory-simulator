#include "p2_listas.h"

/* Control HANDLES */
HANDLE ghExitEvent = NULL;					// Evento Encerramento Global
HANDLE ghCapMsgSem = NULL;					// Semaforo Captura de Mensagem on/off

/* Status List HANDLES */
HANDLE ghListaStatusMappedFile = NULL;		// Arquivo Mapeado em memoria
HANDLE ghPermissaoLeituraStatusSem = NULL;	// Semaforo Controle escrita em lista circular
HANDLE ghPermissaoEscritaStatusSem = NULL;	// Semaforo Controle leitura de lista circular
HANDLE ghListaStatusMutex = NULL;			// Mutex de proteção leitura/escrita lista circular

/* OP List HANDLES */
HANDLE ghListaOPMappedFile = NULL;			// Arquivo Mapeado em memoria
HANDLE ghPermissaoLeituraOPSem = NULL;		// Semaforo Controle escrita em lista circular
HANDLE ghPermissaoEscritaOPSem = NULL;		// Semaforo Controle leitura de lista circular
HANDLE ghListaOPMutex = NULL;				// Mutex de proteção leitura/escrita lista circular

/* Pipes HANDLES */
HANDLE ghPipeOPs = NULL;					// MES
HANDLE ghPipeCLP[N_LINHAS] = {NULL};		// CLP 1 e 2 

/* Threads HANDLES */
HANDLE ghThreadCapCLPs[N_LINHAS] = {NULL};	// CLP 1 e 2 
HANDLE ghThreadCapMES = NULL;				// MES


int main(int argc, char* argv[])
{
	setlocale(LC_ALL,"");

	inicializaObjetosP2();

	/* Aguarda fim das threads */
	WaitForMultipleObjects(2,ghThreadCapCLPs,TRUE,INFINITE);
	WaitForSingleObject(ghThreadCapMES,INFINITE);

	fechaHandlesP2();

	return 0;
}


DWORD WINAPI captura_status(LPVOID linha)
{
	static DWORD counter = 0;

	/*	Mutex para acesso a variável de controle de lista cheia */
	HANDLE hListaCheiaMutex = CreateMutex(NULL,false,TEXT("M") );
	if( hListaCheiaMutex == NULL)
		hListaCheiaMutex = OpenMutex(MUTEX_ALL_ACCESS,false,TEXT("M") );

	int n_linha = (int) linha;
	char* sm_ptr;

	char buffer[TAMANHO_MSG_STATUS+1];
	DWORD bytes_lidos;

	char* base_sm_addr = (char*) MapViewOfFile( ghListaStatusMappedFile,FILE_MAP_READ | FILE_MAP_WRITE,0,0,
											(TAMANHO_LISTA_STATUS*TAMANHO_MSG_STATUS) + sizeof(DWORD));

	DWORD* n_entradas_lista = (DWORD*) &base_sm_addr[TAMANHO_LISTA_STATUS*TAMANHO_MSG_STATUS];
	*n_entradas_lista = 0;

	HANDLE hSinc[] = {ghExitEvent, ghCapMsgSem};
	static BOOL lista_cheia = FALSE;

	// Inicialização timer
	LARGE_INTEGER liDueTime = {0};
	liDueTime.QuadPart = -1000000LL; // tempo de espera 100 ms
	HANDLE hTimer = (HANDLE) CreateWaitableTimer(NULL,FALSE,NULL);

	while(1)
	{
		if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0 ) break;

		SetWaitableTimer(hTimer,&liDueTime,0,NULL,NULL,FALSE);
		WaitForSingleObject(hTimer,250);

		ReadFile(ghPipeCLP[n_linha],buffer,TAMANHO_MSG_STATUS,&bytes_lidos, NULL);

		if(bytes_lidos == TAMANHO_MSG_STATUS)
		{
			if( WaitForSingleObject(ghPermissaoEscritaStatusSem,TIMEOUT_MS) == WAIT_TIMEOUT)
			{
				if( *n_entradas_lista == TAMANHO_LISTA_STATUS )
				{
					WaitForSingleObject(hListaCheiaMutex,INFINITE);
					if(!lista_cheia)
						printf("\nLista Status Cheia.\nFavor ligar Módulo de Exibição de Mensagens de Status para liberar espaço de escrita\n\n");

					lista_cheia = TRUE;
					ReleaseMutex(hListaCheiaMutex);

					ReleaseSemaphore(ghCapMsgSem,1,NULL);
					hSinc[1] = ghPermissaoEscritaStatusSem;
					if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0 ) break;
					lista_cheia = FALSE;
					hSinc[1] = ghCapMsgSem;
				}
				else continue;
			}

			if( WaitForSingleObject(ghListaStatusMutex,TIMEOUT_MS) == WAIT_TIMEOUT)
			{
				ReleaseSemaphore(ghCapMsgSem,1,NULL); continue;
			}
			
			sm_ptr = &base_sm_addr[counter*TAMANHO_MSG_STATUS];
			
			counter = (counter +1)%TAMANHO_LISTA_STATUS;
			*n_entradas_lista = *n_entradas_lista + 1;

			buffer[TAMANHO_MSG_STATUS] = '\0';
			
			strncpy(sm_ptr,buffer,TAMANHO_MSG_STATUS);

			ReleaseMutex(ghListaStatusMutex);
			ReleaseSemaphore(ghPermissaoLeituraStatusSem,1,NULL);
		}
		
		ReleaseSemaphore(ghCapMsgSem,1,NULL);
	}

	UnmapViewOfFile(sm_ptr);
	_endthreadex(EXIT_SUCCESS);
	return 0;
}

DWORD WINAPI captura_OPs()
{
	DWORD bytes_lidos;
	static DWORD counter = 0;
	char buffer[TAMANHO_MSG_ORDEM_PRODUCAO+1];
	char* sm_ptr;
	char* base_sm_addr = (char*)MapViewOfFile( ghListaOPMappedFile,
		FILE_MAP_WRITE | FILE_MAP_READ ,
		0,
		0,
		(TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + sizeof(DWORD));

	DWORD* n_entradas_lista = (DWORD*) &base_sm_addr[TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO];

	HANDLE hSinc[] = { ghExitEvent, ghCapMsgSem };

	// Inicialização timer
	LARGE_INTEGER liDueTime = {0};
	liDueTime.QuadPart = -1000000LL; // tempo de espera 100 ms
	HANDLE hTimer = (HANDLE) CreateWaitableTimer(NULL,FALSE,NULL);

	while(1)
	{
		if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0 ) break;

		SetWaitableTimer(hTimer,&liDueTime,0,NULL,NULL,FALSE);
		WaitForSingleObject(hTimer,250);

		ReadFile(ghPipeOPs,buffer,TAMANHO_MSG_ORDEM_PRODUCAO,&bytes_lidos, NULL);

		if(bytes_lidos == TAMANHO_MSG_ORDEM_PRODUCAO)
		{

			if( WaitForSingleObject(ghPermissaoEscritaOPSem,TIMEOUT_MS) == WAIT_TIMEOUT)
			{
				if(*n_entradas_lista == TAMANHO_LISTA_OP)
				{
					printf("\nLista OP's Cheia.\nFavor ligar Módulo de Exibição de Ordens de Produção para liberar espaço de escrita\n\n");
					
					ReleaseSemaphore(ghCapMsgSem,1,NULL);
					hSinc[1] = ghPermissaoEscritaOPSem;
					if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0 ) break;
					hSinc[1] = ghCapMsgSem;
				}
				else continue;
			}

			if( WaitForSingleObject(ghListaOPMutex,TIMEOUT_MS) == WAIT_TIMEOUT)
			{
				ReleaseSemaphore(ghCapMsgSem,1,NULL); continue;
			}
			
			sm_ptr = &base_sm_addr[counter*TAMANHO_MSG_ORDEM_PRODUCAO];
		
			counter = (counter +1)%TAMANHO_LISTA_OP;

			strncpy(sm_ptr,buffer,TAMANHO_MSG_ORDEM_PRODUCAO);

			*n_entradas_lista = *n_entradas_lista + 1;

			buffer[TAMANHO_MSG_ORDEM_PRODUCAO] = '\0';

			ReleaseMutex(ghListaOPMutex);
			ReleaseSemaphore(ghPermissaoLeituraOPSem,1,NULL);
		}

		ReleaseSemaphore(ghCapMsgSem,1,NULL);
	}
	CloseHandle(hTimer);
	_endthreadex(EXIT_SUCCESS);
	return 0;
}

void inicializaObjetosP2(void)
{
	/* Control HANDLES */
	ghExitEvent = (HANDLE) OpenEvent(EVENT_ALL_ACCESS, FALSE, GLOBAL_EXIT_EVENT);
	CheckForError(ghExitEvent);

	ghCapMsgSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,CAP_MSG_SEM);
	CheckForError(ghCapMsgSem);

/* ----------------------------------------------------------------------- */

	/* Status List HANDLES */
	ghListaStatusMappedFile = CreateFileMapping(
		(HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,		// tipo de acesso
		0,					// dwMaximumSizeHigh
		(TAMANHO_LISTA_STATUS*TAMANHO_MSG_STATUS) + sizeof(DWORD),					// dwMaximumSizeLow
		STATUS_LIST_MAPPED_FILE);			// Escolha o seu nome preferido
	CheckForError(ghListaStatusMappedFile);

	ghPermissaoLeituraStatusSem = (HANDLE) CreateSemaphore(NULL,0,TAMANHO_LISTA_STATUS,PERMISSAO_LEITURA_STATUS_SEM);
	CheckForError(ghPermissaoLeituraStatusSem);

	ghPermissaoEscritaStatusSem = (HANDLE) CreateSemaphore(NULL,TAMANHO_LISTA_STATUS,TAMANHO_LISTA_STATUS,PERMISSAO_ESCRITA_STATUS_SEM);
	CheckForError(ghPermissaoEscritaStatusSem);

	ghListaStatusMutex = (HANDLE) CreateMutex(NULL,FALSE,LISTA_STATUS_MUTEX);
	CheckForError(ghListaStatusMutex);

/* ----------------------------------------------------------------------- */

	/* OP List HANDLES */
	 ghListaOPMappedFile = CreateFileMapping(
		(HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,		// tipo de acesso
		0,					// dwMaximumSizeHigh
		(TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + sizeof(DWORD),					// dwMaximumSizeLow
		OP_LIST_MAPPED_FILE);			// Escolha o seu nome preferido
	CheckForError(ghListaOPMappedFile);

	ghPermissaoLeituraOPSem = (HANDLE) CreateSemaphore(NULL, 0,TAMANHO_LISTA_OP, PERMISSAO_LEITURA_OP_SEM);
	CheckForError(ghPermissaoLeituraOPSem);

	ghPermissaoEscritaOPSem = (HANDLE) CreateSemaphore(NULL, TAMANHO_LISTA_OP,TAMANHO_LISTA_OP, PERMISSAO_ESCRITA_OP_SEM);
	CheckForError(ghPermissaoEscritaOPSem);
	
	ghListaOPMutex = (HANDLE) CreateMutex(NULL,FALSE,LISTA_OP_MUTEX);
	CheckForError(ghListaOPMutex);

/* ----------------------------------------------------------------------- */
	
	/* Pipes HANDLES */
	int i = 0;
	DWORD dwThreadID;

	ghPipeOPs = (HANDLE) CreateNamedPipe(CAMINHO_PIPE_DISP_MES,
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT,
		1,
		0,
		0,
		0,
		NULL);
	if(ghPipeOPs == INVALID_HANDLE_VALUE) printf("Pipe nao pode ser criado\n");

	for (i=0;i<N_LINHAS;i++)
	{
		ghPipeCLP[i] = (HANDLE) CreateNamedPipe(CAMINHO_PIPE_DISP_CLPS[i],
			PIPE_ACCESS_INBOUND,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT,
			1,
			0,
			0,
			NULL,
			NULL);
		if(ghPipeCLP[i] == INVALID_HANDLE_VALUE) printf("Pipe nao pode ser criado\n");
	}

/* ----------------------------------------------------------------------- */

	/* Threads HANDLES */
	for ( i = 0; i < N_LINHAS; i++)
	{
		ghThreadCapCLPs[i] = (HANDLE) _beginthreadex(NULL,0, (CAST_FUNCTION)	captura_status,
								(LPVOID) i, 0, (CAST_LPDWORD) &dwThreadID);
		//if(ghThreadCapCLPs[i]) printf("Thread de Captura Mensgaens Status (CLPs) da Linha %d criada com sucesso. Id = %0x\n",i+1,dwThreadID);
	}

	ghThreadCapMES = (HANDLE) _beginthreadex(NULL,0, (CAST_FUNCTION)	captura_OPs,
								NULL, 0, (CAST_LPDWORD) &dwThreadID);
	//if(ghThreadCapMES) 
		//printf("Thread de Captura Mensagens OP (MES) criada com sucesso. Id = %0x\n",dwThreadID);

}

void fechaHandlesP2(void)
{
	int i;

	/* Control HANDLES */
	CloseHandle(ghExitEvent);
	CloseHandle(ghCapMsgSem);

	/* Status List HANDLES */
	CloseHandle(ghListaStatusMappedFile);
	CloseHandle(ghPermissaoLeituraStatusSem);
	CloseHandle(ghPermissaoEscritaStatusSem);
	CloseHandle(ghListaStatusMutex);

	/* OP List HANDLES */
	CloseHandle(ghListaOPMappedFile);
	CloseHandle(ghPermissaoLeituraOPSem);
	CloseHandle(ghPermissaoEscritaOPSem);
	CloseHandle(ghListaOPMutex);

	/* Pipes HANDLES */
	CloseHandle(ghPipeOPs);
	for(i=0; i<N_LINHAS; i++) CloseHandle(ghPipeCLP[i]);

	/* Threads HANDLES */
	CloseHandle(ghThreadCapMES);
	for(i=0; i<N_LINHAS; i++) CloseHandle(ghThreadCapCLPs[i]);

}