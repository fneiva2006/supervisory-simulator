#include "p1_leitura.h"

/* Control HANDLES */
HANDLE ghExitEvent = NULL;					// Evento Encerramento Global
HANDLE ghLeituraMESSem = NULL;				// Semaforo MES on/off
HANDLE ghLeituraCLPSem[N_LINHAS] = {NULL};	// Semaforo CLP 1 e 2 on/off

/* Pipes HANDLES */
HANDLE ghPipeOPs = NULL;					// MES
HANDLE ghPipeCLP[N_LINHAS] = {NULL};		// CLP 1 e 2 

/* Threads HANDLES */
HANDLE ghThreadCLPs[N_LINHAS] = {NULL};		// CLP 1 e 2
HANDLE ghThreadMES = NULL;					// MES

int main(int argc, char* argv[])
{ 
	inicializaObjetosP1();

	/* Aguarda fim das threads */
	WaitForSingleObject(ghThreadMES,INFINITE); 
	WaitForMultipleObjects(2,ghThreadCLPs,TRUE,INFINITE); 

	fechaHandlesP1();

	return 0;
}

DWORD WINAPI le_clp(LPVOID id_clp)
{
	srand((unsigned int) time(NULL)); // altera a semente do gerador de numeros aleatorios

	const int n_linha = (int) id_clp;

	if( n_linha != 0 && n_linha != 1)
	{
		printf("Erro ! Linha de Producao Inexistente\n");
		return 0;
	}

	char buffer[TAMANHO_MSG_STATUS + 1];

	// Inicialização campos de mensagens de ststus a serem enviadas
	int nseq = 99999;
	unsigned int op = 0, tipo = 0,  tbat = 0, 
				nbat = 0, estagio = 0,  hora = 0, min = 0, seg = 0;
	unsigned long int prod = 0;

	struct tm* tempo = (struct tm*) calloc(1,sizeof(struct tm));
	time_t rawtime;

	// Inicialização timer
	LARGE_INTEGER liDueTime = {0};
	liDueTime.QuadPart = -5000000LL*(n_linha+1); // tempo de espera 
	HANDLE hTimer = (HANDLE) CreateWaitableTimer(NULL,FALSE,NULL);

	DWORD bytes_escritos;
	OVERLAPPED ov = {0};

	/*	Variável auxiliar usada para configurar a espera de 2 objetos do kernel simultaneamente */
	HANDLE hSinc[] = { ghExitEvent, ghLeituraCLPSem[n_linha] };

	while(1)
	{
		/*	Atualiza segundo objeto a ser esperado -> semáforo Liga/Desliga do módulo */
		hSinc[1] = ghLeituraCLPSem[n_linha];

		/*	Aguarda sinalização do evento de encerramento do programa OU semáforo
		*	Liga/Desliga. Caso o evento de encerramento seja ativado, o loop de
		*	execução é quebrado */
		if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0) break;

		// processa dados a serem enviados na mensagem de status
		time(&rawtime);
		localtime_s(tempo, &rawtime); 
		hora =	(unsigned int) tempo->tm_hour;
		min =	(unsigned int) tempo->tm_min;
		seg =	(unsigned int) tempo->tm_sec;
		
		tipo = 1 + rand()%9;
		op = op + (rand()%51)%1000000;
		tipo = 1 + rand()%9;
		nseq = 1 + (nseq)%99999;
		tbat = 1 + rand()%99;
		nbat = 1 + rand()%99;
		estagio = 1 + rand()%99;
		prod = (prod + rand()%300)%10000000;

		// Prepara mensagem de status e a armazena num buffer para ser enviada 
				//				    6    1    1   5    2    2    2    7     8
		sprintf_s(buffer,"%06d;%1d;%1d;%05d;%02d;%02d;%02d;%07d;%02d:%02d:%02d",
			op, n_linha+1, tipo, nseq, tbat, nbat, estagio, prod, hora,min,seg);

		SetWaitableTimer(hTimer,&liDueTime,0,NULL,NULL,FALSE);

		/*	Atualiza segundo objeto a ser esperado -> timer de geração de mensagens  */
		hSinc[1] = hTimer; // Aguarda timer
		if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0) break;

		/*	Cancelamento de envio de mensagens de status enfileiradas 
		*	Mensagens de status dos CLP's que não são lidas pelo módulo de 
		*	captura de mensagens são propositalmente descartadas */
		CancelIo(ghPipeCLP[n_linha]); 

		// envia mensagem de status pelo pipe
		WriteFile(ghPipeCLP[n_linha],buffer,TAMANHO_MSG_STATUS,&bytes_escritos,&ov);

		ReleaseSemaphore(ghLeituraCLPSem[n_linha],1,NULL);
	}
	
	_endthreadex(0); 
	return 0;
}

DWORD WINAPI le_MES()
{
	srand((unsigned int) time(NULL)); // altera a semente do gerador de numeros aleatorios

	char buffer[TAMANHO_MSG_ORDEM_PRODUCAO + 1];

	// Inicialização de campos de mensagens de Ordem de Produção
	int nseq = 99999;
	int op = 0, tbat = 0, linha = 0, tipo = 0, receita = 0;

	// Inicialização de Timer
	LARGE_INTEGER liDueTime = {0};
	HANDLE hTimer = (HANDLE) CreateWaitableTimer(NULL,FALSE,NULL);

	/*	Variável auxiliar usada para configurar a espera de 2 objetos do kernel simultaneamente */
	HANDLE hSinc[] = { ghExitEvent, ghLeituraMESSem };

	DWORD bytes_escritos;
	OVERLAPPED ov = {0};

	while(1)
	{
		/*	Atualiza segundo objeto a ser esperado -> semáforo Liga/Desliga do módulo */
		hSinc[1] = ghLeituraMESSem;

		/*	Aguarda sinalização do evento de encerramento do programa OU semáforo
		*	Liga/Desliga. Caso o evento de encerramento seja ativado, o loop de
		*	execução é quebrado */
		if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0) break;

		/*	Geração dos valores dos dados de OP a serem enviados */
		nseq = 1 + (nseq)%99999;
		op = op + (rand()%51)%1000000;
		tipo = 1 + rand()%9;
		linha = 1 + rand()%2;
		tbat = 1 + rand()%99;
		receita = rand()%100;

		liDueTime.QuadPart = -10000000LL*(1 + rand()%5); // tempo de espera 
		SetWaitableTimer(hTimer,&liDueTime,0,NULL,NULL,FALSE);

		/*	Atualiza segundo objeto a ser esperado -> timer de geração de mensagens  */
		hSinc[1] = hTimer;
		if( WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0) break;

		sprintf(buffer,"%05d/%06d/%02d/%1d/%1d/%02d",nseq,op,tbat,linha,tipo,receita);

		/*	Escreve no Pipe as OP's geradas. As OP's não lidas de imediato pelo módulo
		*	de captura são salvas em buffer e despachadas à medida que o módulo de captura
		*	de OP's le as mensagens do pipe */
		WriteFile(ghPipeOPs,buffer,TAMANHO_MSG_ORDEM_PRODUCAO,&bytes_escritos,&ov);
		ReleaseSemaphore(ghLeituraMESSem,1,NULL);
	}

	_endthreadex(0);
	return 0;
}

void inicializaObjetosP1(void)
{
	int i;
	DWORD dwThreadID;

	/* Control HANDLES */

	// abertura objeto de evento global exit
	ghExitEvent = (HANDLE) OpenEvent( EVENT_ALL_ACCESS, FALSE, GLOBAL_EXIT_EVENT);
	if(ghExitEvent == NULL) printf("ERRO EVENT: %d\n", GetLastError());

	ghLeituraMESSem = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,LEITURA_MES_SEM);
	CheckForError(ghLeituraMESSem);

	// Semaforo Sinalização on/off leitura CLP n_linha
	for(i=0; i<N_LINHAS; i++)
	{
		ghLeituraCLPSem[i] = (HANDLE) OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE, LEITURA_CLP_SEM[i]);
		CheckForError(ghLeituraCLPSem[i]);
	}

/* ----------------------------------------------------------------------- */	
	
	/* Pipes HANDLES */
	for(i = 0; i<N_LINHAS ; i++)
	{
		do
		{
			ghPipeCLP[i] = (HANDLE) CreateFile(CAMINHO_PIPE_DISP_CLPS[i],
				GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
				NULL);
			//printf("CLPSPIPE %d %d\n",i,ghPipeCLP[i]);
		} while(GetLastError() != ERROR_SUCCESS);

	}

	do
	{
		ghPipeOPs = (HANDLE) CreateFile(CAMINHO_PIPE_DISP_MES,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL);
	} while(GetLastError() != ERROR_SUCCESS);
	
/* ----------------------------------------------------------------------- */	
	/* Threads HANDLES */

	// abertura semaforo de controle on/off dos dispositivos clp
	for ( i = 0; i < N_LINHAS; i++)
	{
		ghThreadCLPs[i] = (HANDLE) _beginthreadex(NULL,0, (CAST_FUNCTION)	le_clp, (LPVOID) i, 0, (CAST_LPDWORD) &dwThreadID);
		//if(ghThreadCLPs[i])
		//	printf("Thread de CLP da Linha %d criada com sucesso. Id = %0x\n",i+1,dwThreadID);
	}

	// abertura da thread de processamento do dispositivo MES
	ghThreadMES = (HANDLE) _beginthreadex(NULL,0, (CAST_FUNCTION)	le_MES, NULL, 0, (CAST_LPDWORD) &dwThreadID);
	//if(ghThreadMES) printf("Thread de MES criada com sucesso. Id = %0x\n",dwThreadID);

}

void fechaHandlesP1(void)
{
	int i;

	/* Control HANDLES */
	CloseHandle(ghExitEvent);
	CloseHandle(ghLeituraMESSem);
	for (i=0;i<N_LINHAS;i++) CloseHandle(ghLeituraCLPSem[i]);
	
	/* Pipes HANDLES */
	CloseHandle(ghPipeOPs);
	for (i=0;i<N_LINHAS;i++) CloseHandle(ghPipeCLP[i]);

	/* Threads HANDLES */
	for ( i = 0; i < N_LINHAS; i++)	CloseHandle(ghThreadCLPs[i]);	
	CloseHandle(ghThreadMES);

}