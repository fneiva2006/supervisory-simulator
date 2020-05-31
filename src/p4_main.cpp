#include "p4_ops.h"

/* Control HANDLES */
HANDLE ghExitEvent = NULL;					// Evento encerramento do programa
HANDLE ghExibeOpSem = NULL; 			// Sem�foro liga/desliga m�dulo

/* OP List HANDLES */
HANDLE ghListaOPMappedFile = NULL;			//
HANDLE ghPermissaoLeituraOPSem = NULL;
HANDLE ghPermissaoEscritaOPSem = NULL;
HANDLE ghListaOPMutex = NULL;

/* OP Disk List Handles */
HANDLE ghOPDiskList = NULL;


int main(int argc, char* argv[])
{
	setlocale(LC_ALL,"");

	/* Adquire Handle de janela de console */
	HWND ptr = GetConsoleWindow();

	/* Posiciona janela do console em local inicial */
	MoveWindow(ptr,670,0,720,400, true);

	/* Desativa Bot�o de Fechar da Janela do Console */
	EnableMenuItem(GetSystemMenu(ptr, FALSE), SC_CLOSE,
                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	inicializaObjetosP4();	
	
	DWORD indice_leitura = 0; // Posi��o da lista de Status a ser lida

	char *sm_ptr; // Ponteiro auxiliar para captura de mensagens da lista

	/* Buffer auxiliar para armazenar as mensagens lidas da lista */
	char buffer[TAMANHO_MSG_ORDEM_PRODUCAO+1]; 

	/* Endere�o base relativo ao in�cio da lista de status compartilhada em mem�ria */
	const char* base_sm_addr = (char*) MapViewOfFile(ghListaOPMappedFile,
			FILE_MAP_READ | FILE_MAP_WRITE,
			0,
			0,
			(TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + sizeof(DWORD) );

	DWORD* n_entradas_lista = (DWORD*) &base_sm_addr[TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO];

	/* Array auxiliar de handles */
	HANDLE hSinc[] = { ghExitEvent, ghExibeOpSem };

	while(1)
	{
		/*	Aguarda Evento de Encerramento Global ou Estado de Exibi��o Ligado
			(Semaforo de Controle on/off do m�dulo) */
		if(WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0) break;

		/*	Aguarda Permiss�o de Leitura da Lista (Sem�foro de Recurso Compartilhado
			da lista). Opera��o com timeout */
		if( WaitForSingleObject(ghPermissaoLeituraOPSem,TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			/*	Libera Semaforo de Controle on/off do m�dulo de Exibi��o de Mensagens de Status*/
			ReleaseSemaphore(ghExibeOpSem,1,NULL); continue;
		}

		/*	Aguarda Condi��o de exclus�o m�tua para Leitura da Lista 
			(Mutex de Recurso Compartilhado da lista). Opera��o com timeout */
		if( WaitForSingleObject(ghListaOPMutex,TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			/*	Libera Semaforo de Controle on/off do m�dulo de Exibi��o de Mensagens de Status*/
			ReleaseSemaphore(ghExibeOpSem,1,NULL); continue;
		}

		/*	Adquire posi��o da mem�ria da pr�xima mensagem da lista ser lida */
		sm_ptr = (char*) &base_sm_addr[indice_leitura*TAMANHO_MSG_ORDEM_PRODUCAO];

		/*	Extrai e salva no buffer mensagem de status localizada no endere�o 
			de memoria obtido anteriormente*/
		strncpy(buffer,sm_ptr,TAMANHO_MSG_ORDEM_PRODUCAO);
		buffer[TAMANHO_MSG_ORDEM_PRODUCAO] = '\0';

		/*	Exibe Mensagem Extra�da da Lista e a salva em Disco */
		salvaOPEmDisco(buffer);
		exibeMensagemOP(buffer);

		/*	Decrementa n�mero de entradas da lista que ainda n�o foram lidas */
		*n_entradas_lista = *n_entradas_lista - 1;

		/*	Atualiza �ndice para a leitura da pr�xima mensagem da lista*/
		indice_leitura = (indice_leitura+1)%TAMANHO_LISTA_OP;

		/*	Libera 1 permiss�o de escrita na lista */
		ReleaseSemaphore(ghPermissaoEscritaOPSem,1,NULL);

		/*	Libera Mutex de controle do acesso a lista */
		ReleaseMutex(ghListaOPMutex);

		/*	Libera Semaforo de Controle on/off do m�dulo de Exibi��o de Mensagens de Status */
		ReleaseSemaphore(ghExibeOpSem,1,NULL);

	}

	FlushFileBuffers(ghOPDiskList);					 
	UnmapViewOfFile(sm_ptr); /*  Fecha vis�o da lista compartilhada em mem�ria */
	fechaHandlesP4();

	/*	Salva copia da lista de OP's em disco */
	CopyFile(OP_DISK_LIST,COPIA_OP_DISK_LIST,FALSE);
	SetFileAttributes(COPIA_OP_DISK_LIST,FILE_ATTRIBUTE_NORMAL);

	return 0;
}

void inicializaObjetosP4(void)
{
	/* Control HANDLES */

	/* Abre o evento de encerramento global da aplica��o */
	ghExitEvent = (HANDLE) OpenEvent(EVENT_ALL_ACCESS,FALSE,GLOBAL_EXIT_EVENT);
	CheckForError(ghExitEvent);

	/* Abre o semaforo de controle on/off do modulo de exibi��o das mensagens de status*/
	ghExibeOpSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,EXIBE_OP_SEM);
	CheckForError(ghExibeOpSem);

	/* ----------------------------------------------------------------------- */

	/* OP List HANDLES */

	/* Abre o handle para a lista de status mapeada em memoria*/
	do
	{
		ghListaOPMappedFile = (HANDLE) OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,OP_LIST_MAPPED_FILE);
	
	} while ( ghListaOPMappedFile == INVALID_HANDLE_VALUE);


	/* Abre os semaforos de permissao de leitura/escrita da lista de OP's compartilhada em memoria */
	do
	{	
		 ghPermissaoLeituraOPSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,
													FALSE,
													PERMISSAO_LEITURA_OP_SEM);
		 ghPermissaoEscritaOPSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,
													FALSE,
													PERMISSAO_ESCRITA_OP_SEM);

	} while( ( ghPermissaoLeituraOPSem == INVALID_HANDLE_VALUE) || ( ghPermissaoEscritaOPSem == INVALID_HANDLE_VALUE ));
	
	
	/* Abre Mutex da Lista em de OP's em Memoria Compartilhada */
	do
	{
		ghListaOPMutex= (HANDLE) OpenMutex(MUTEX_ALL_ACCESS,FALSE, LISTA_OP_MUTEX);

	} while(ghListaOPMutex == INVALID_HANDLE_VALUE);

	/* OP Disk List Handles */
	ghOPDiskList = (HANDLE) CreateFile(OP_DISK_LIST,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN,
		NULL);
	if(GetLastError() != 0 && GetLastError() != ERROR_ALREADY_EXISTS )
		printf("Erro ao abrir lista em disco. Error ID %d\n", GetLastError());

	inicializaListaOPEmDisco();
	exibeListaOPEmDisco();
}

void fechaHandlesP4(void)
{
	/*	Fechamento de todos os handles utilizados no c�digo */

	/* Control HANDLES */
	CloseHandle(ghExitEvent);
	CloseHandle(ghExibeOpSem);
	
	/* Status List HANDLES */
	CloseHandle(ghListaOPMappedFile);
	CloseHandle(ghPermissaoLeituraOPSem);
	CloseHandle(ghPermissaoEscritaOPSem);
	CloseHandle(ghListaOPMutex);

	/* OP Disk List Handles */
	CloseHandle(ghOPDiskList);
}

void exibeMensagemOP(const char* msg)
{
	MensagemOP_t Mensagem;
	
	static char copia[32];
	strcpy(copia,msg);

	strcpy(Mensagem.nseq, strtok(copia,"/"));
	strcpy(Mensagem.op, strtok(NULL,"/"));
	strcpy(Mensagem.tbat, strtok(NULL,"/"));
	strcpy(Mensagem.linha, strtok(NULL,"/"));
	strcpy(Mensagem.tipo, strtok(NULL,"/"));
	strcpy(Mensagem.receita, strtok(NULL,"/"));

	printf("NSEQ: %s OP: %s TBAT: %s LINHA: %s TIPO: %s RECEITA: %s\n",
		Mensagem.nseq, Mensagem.op, Mensagem.tbat, Mensagem.linha, Mensagem.tipo,
		Mensagem.receita);
}

void salvaOPEmDisco(const char* msg)
{
	static char buffer[256] = {0};
	DWORD bytes_escritos;
	DWORD pointer = 0;
	DWORD idx_proxima_entrada = 0;
	BOOL lista_completa = true;

	LockFile(ghOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);

	SetFilePointer(ghOPDiskList,-(LONG) OVERHEAD_CONTROLE,0,FILE_END);
	ReadFile(ghOPDiskList,&idx_proxima_entrada,sizeof(DWORD),&bytes_escritos,NULL);
	
	SetFilePointer(ghOPDiskList,idx_proxima_entrada*TAMANHO_MSG_ORDEM_PRODUCAO,0,FILE_BEGIN);
	WriteFile(ghOPDiskList,msg,TAMANHO_MSG_ORDEM_PRODUCAO,&bytes_escritos,NULL);

	printf("Atribuindo � Entrada %d -> \n\t",idx_proxima_entrada+1);

	idx_proxima_entrada = (idx_proxima_entrada+1)%TAMANHO_LISTA_OP;

	SetFilePointer(ghOPDiskList,-(LONG) OVERHEAD_CONTROLE,0,FILE_END);
	WriteFile(ghOPDiskList,&idx_proxima_entrada,sizeof(DWORD),&bytes_escritos,NULL);

	if(idx_proxima_entrada == 0)
		WriteFile(ghOPDiskList,&lista_completa,sizeof(BOOL),&bytes_escritos,NULL);

	UnlockFile(ghOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);
	
}

void inicializaListaOPEmDisco()
{
	/*	Inicializa��o do tamanho da lista em disco */
	_LARGE_INTEGER liTamanhoListaDisco = {0}, pointer = {0};
	liTamanhoListaDisco.LowPart =  ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE);

	/*	Valores de campos de controle a serem inicializados */
	DWORD init_idx = 0;
	BOOL init_lista_completa = 0;

	DWORD bytes_escritos = 0;

	/* Sob regime de exclus�o m�tua, escreve valores de incializa��o nos campos de controle da lista */
	LockFile(ghOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);

	if(SetFilePointer(ghOPDiskList,0,0,FILE_END) != liTamanhoListaDisco.LowPart)
	{
		puts("Lista de OP's em Disco Inexistente. Criando Lista Agora ...");
		SetFilePointerEx(ghOPDiskList,liTamanhoListaDisco, &pointer, FILE_BEGIN );
		SetEndOfFile(ghOPDiskList);

		SetFilePointer(ghOPDiskList,-(LONG) OVERHEAD_CONTROLE,0,FILE_END);
		WriteFile(ghOPDiskList,&init_idx,sizeof(DWORD),&bytes_escritos,NULL);
		WriteFile(ghOPDiskList,&init_lista_completa,sizeof(BOOL),&bytes_escritos,NULL);
	}

	UnlockFile(ghOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);

}

void exibeListaOPEmDisco()
{
	char buffer[256];
	DWORD bytes_lidos;
	DWORD idx_proxima_entrada = 0;
	BOOL lista_completa = FALSE;
	DWORD i;

	LockFile(ghOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);

	SetFilePointer(ghOPDiskList,-(LONG)OVERHEAD_CONTROLE,0,FILE_END);
	ReadFile(ghOPDiskList,&idx_proxima_entrada,sizeof(DWORD),&bytes_lidos,NULL);
	ReadFile(ghOPDiskList,&lista_completa,sizeof(BOOL),&bytes_lidos,NULL);

	if(lista_completa) 
	{
		idx_proxima_entrada = TAMANHO_LISTA_OP;
		printf("Lista est� completa (%3d Registros)\n\n",idx_proxima_entrada);
	}
	else
		printf("Lista n�o est� completa e cont�m %3d Registros \n\n",idx_proxima_entrada);

	SetFilePointer(ghOPDiskList,0,0,FILE_BEGIN);

	for(i = 0; i < idx_proxima_entrada; i++)
	{
		ReadFile(ghOPDiskList,buffer,TAMANHO_MSG_ORDEM_PRODUCAO,&bytes_lidos,NULL);
		buffer[bytes_lidos] = '\0';

		if(bytes_lidos == TAMANHO_MSG_ORDEM_PRODUCAO)
		{
			printf("%3d) ",i+1);
			exibeMensagemOP(buffer);
		}
		
	} 
	UnlockFile(ghOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);
	puts("");

}