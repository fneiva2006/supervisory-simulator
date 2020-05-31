#include "p5_user.h"

/* Control HANDLES */
HANDLE	ghExitEvent = NULL;

/* P1 */
HANDLE ghLeituraMESSem = NULL;
HANDLE ghLeituraCLPSem[N_LINHAS] = {NULL};

/* P2 */
HANDLE	ghCapMsgSem = NULL;

/* P3 */
HANDLE	ghExibeStatusSem = NULL;

/* P4 */
HANDLE	ghExibeOpSem = NULL;

/* Threads HANDLES */
HANDLE ghThreadTeclado;

/* Process HANDLES */
HANDLE ghProcessos[N_PROCESSOS];

int main(int argc, char* argv[])
{
	setlocale(LC_ALL,"");

	/*	Adquire handler de janela do console */ 
	HWND ptr = GetConsoleWindow();	
	/*	Move console para posição padrão */
	MoveWindow(ptr,0,0,670,400, true);
	SetWindowText(ptr,TEXT("TP ATR - Simulador de Sistema Supervisório Industrial"));

	printf("TP ATR - Simulador de Sistema Supervisório Industrial\n");
	printf("Desenvolvido por Felipe Neiva Montalvão Melo\n\n");
	printf("Sistema Ativo\n");

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	/*	Altera Cor de Texto para Vermelho */
	SetConsoleTextAttribute(hConsole,12);

	printf("Todos os dispositivos e funções encontram-se inicialmente %ss(as)\n",
		LIG_DESL_TEXT[DEVICES_INITIAL_STATE]);
	/*	Volta Cor de Texto para o normal */
	SetConsoleTextAttribute(hConsole,7);

	exibe_comandos();

	inicializaObjetosP5();

	/*	Espera um tempo a fim de permitir que todos outros processos/threads da aplicação tenham se aberto */
	Sleep(300);
	/*	Coloca janela do console em primeiro plano e traz foco para ela */
	SwitchToThisWindow(ptr,true);

	/*	Aguarda Término de todas threads e processos da aplicação */
	WaitForSingleObject(ghThreadTeclado,INFINITE);
	WaitForMultipleObjects(2,ghProcessos,TRUE, INFINITE);

	fechaHandlesP5();

	return EXIT_SUCCESS;
}

BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType)
{
	if (dwCtrlType == CTRL_CLOSE_EVENT)
	{
		printf("Desligando Sistema...\n");
		SetEvent(ghExitEvent);
		return TRUE;
	}
	else
	// Return TRUE if handled this message, further handler functions won't be called.
	// Return FALSE to pass this message to further handlers until default handler calls ExitProcess().
		return FALSE;
}


DWORD WINAPI le_teclado()
{
	SetConsoleCtrlHandler(console_ctrl_handler,TRUE);

	char key;

	/*	Variáveis de controle para lembrar o estado de execução (on/off) 
		dos módulos da aplicação que é representado por um semáforo */
	BOOL	clp1_on			=	DEVICES_INITIAL_STATE, 
			clp2_on			=	DEVICES_INITIAL_STATE,
			mes_on			=	DEVICES_INITIAL_STATE, 
			cap_msg_on		=	DEVICES_INITIAL_STATE,
			exibe_status_on =	DEVICES_INITIAL_STATE, 
			exibe_op_on		=	DEVICES_INITIAL_STATE;

	
	DWORD fdwSaveOldMode, fdwMode, dwEventsRead;
	INPUT_RECORD irInBuf;

	/*	Adquire Handle do Console */
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
   
    // Salva modo de captura de eventos do console para ser restaurado no término do processo 
	GetConsoleMode(hStdin, &fdwSaveOldMode);

    /*	Habilita captura de eventos da janela de console */
    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT; 
    SetConsoleMode(hStdin, fdwMode);

	HANDLE hSinc[] = { ghExitEvent, hStdin };

	while(1)
	{
		/*	Aguarda evento de sinalização de término OU evento do console */
		if(WaitForMultipleObjects(2,hSinc,FALSE,INFINITE) == WAIT_OBJECT_0) 
		{
			/*	Restaura modo de captura de eventos do console */
			SetConsoleMode(hStdin,fdwSaveOldMode);
			_endthreadex(0);	
		}

		/*	Captura evento de entrada do console */
		if(!ReadConsoleInput(hStdin,&irInBuf,1,&dwEventsRead))
		{
			printf("Error Opening Console Input Buffer"); _endthreadex(1);
		}

		key = 0;

		/*	Verfica se evento de entrada do console foi gerado por tecla pressionada */
		if(irInBuf.EventType == KEY_EVENT)
		{
			/*	Captura valor de tecla pressionada */
			if(irInBuf.Event.KeyEvent.bKeyDown)
				key = irInBuf.Event.KeyEvent.uChar.AsciiChar;	
		}

		switch (key) // tratamento do comando do teclado
		{
			case '1':
				//sinaliza/libera semaforo on/off leitura clp 1
				switch_device_sem(ghLeituraCLPSem[0],"CLP 1",&clp1_on,1);
				break;

			case '2':
				//sinaliza/libera semaforo on/off leitura clp 2
				switch_device_sem(ghLeituraCLPSem[1],"CLP 2",&clp2_on,1);
				break;

			case 'm':
				//sinaliza semaforo on/off leitura mes
				switch_device_sem(ghLeituraMESSem,"MES",&mes_on,1);
				break;

			case 'r':
				//sinaliza/libera semaforo on/off modulo captura de mensagens
				switch_device_sem(ghCapMsgSem,"Captura de Mensagens",&cap_msg_on,3);
				break;

			case 's':
				//sinaliza/libera semaforo on/off modulo exibicao bateladas
				switch_device_sem(ghExibeStatusSem,"Exibição de Status de Bateladas",&exibe_status_on,1);
				break;
				
			case 'o':
				//sinaliza/libera semaforo on/off modulo exibicao OPs
				switch_device_sem(ghExibeOpSem,"Exibição de Ordens de Produção",&exibe_op_on,1);
				break;

			case 'd':
				//le a mensage de OP mais antiga do arquivo circular e exibe no console
				exibeOPMaisAntigaDisco();
				break;

			case 'c':
				exibe_comandos();
				break;
				
			case ESC:
				//sinaliza desligamento de todas as tarefas por meio de evento
				printf("Desligando Sistema...\n");
				SetEvent(ghExitEvent);
				break;

			default: 
				break;
		}		
	}
	return 0;
}

void inicializaObjetosP5(void)
{
	int i;
	DWORD dwThreadID;

	// Evento Sinalização termino programa
	ghExitEvent = (HANDLE) CreateEvent(NULL,TRUE,FALSE, GLOBAL_EXIT_EVENT );
	CheckForError(ghExitEvent);

	/* P1 */
	// Semaforos Sinalização on/off leitura clps 
	for(i=0; i<N_LINHAS; i++)
	{
		ghLeituraCLPSem[i] = (HANDLE) CreateSemaphore(NULL, DEVICES_INITIAL_STATE, 1, LEITURA_CLP_SEM[i]);
		CheckForError(ghLeituraCLPSem[i]);
	}

	// Semaforo Sinalização on/off leitura MES 
	ghLeituraMESSem  = (HANDLE) CreateSemaphore(NULL, DEVICES_INITIAL_STATE, 1, LEITURA_MES_SEM);
	CheckForError(ghLeituraMESSem);

	/* P2 */
	ghCapMsgSem = (HANDLE) CreateSemaphore(NULL, 3*DEVICES_INITIAL_STATE, 3, CAP_MSG_SEM);
	CheckForError(ghCapMsgSem);

	/* P3 */
	ghExibeStatusSem = (HANDLE) CreateSemaphore(NULL, DEVICES_INITIAL_STATE, 1, EXIBE_STATUS_SEM);
	CheckForError(ghExibeStatusSem);

	/* P4 */
	ghExibeOpSem = (HANDLE) CreateSemaphore(NULL, DEVICES_INITIAL_STATE, 1, EXIBE_OP_SEM);
	CheckForError(ghExibeOpSem);

	/* Criação do diretório de salvamento da lista de Ordens de Produção */
	CreateDirectory(OP_DISK_LIST_DIR,NULL);
	SetFileAttributes(OP_DISK_LIST_DIR,FILE_ATTRIBUTE_HIDDEN);
	CreateDirectory(COPIA_OP_DISK_LIST_DIR,NULL);

/* ----------------------------------------------------------------------- */

	/* Threads HANDLES */
	// Handle Thread de tartamento de entrada do teclado
	ghThreadTeclado = (HANDLE)  _beginthreadex(NULL,0, (CAST_FUNCTION)	le_teclado, NULL, 0, (CAST_LPDWORD) &dwThreadID);
	//if(ghThreadTeclado) 
	//	printf("Modulo de Captura de teclado inicalizado com sucesso. Id = %0x\n", dwThreadID);

/* ----------------------------------------------------------------------- */

	/* Process HANDLES */
	STARTUPINFO si[N_PROCESSOS];				    // StartUpInformation para novo processo
	PROCESS_INFORMATION NewProcess[N_PROCESSOS];	// Informações sobre novo processo criado
	BOOL status;
	
	

	for(i = 0; i<= P4_OPS; i++)
	{
		ZeroMemory(&si[i], sizeof(si[i]));
		si[i].cb = sizeof(si[i]);	// Tamanho da estrutura em bytes
		si[i].lpTitle = (LPWSTR) PROCESSES_TITLES[i];

		status = CreateProcess(
				CAMINHO_EXECUTAVEIS[i],		// Caminho do arquivo executável
	            NULL,                       // Apontador p/ parâmetros de linha de comando
                NULL,                       // Apontador p/ descritor de segurança
				NULL,                       // Idem, threads do processo
				TRUE,						// Herança de handles
				CRIA_CONSOLE_PROCESSO[i],	// Flags de criação
				NULL,	                    // Herança do ambiente de execução
				CAMINHO_WDIR[i],            // Diretório do arquivo executável
		        &si[i],						// lpStartUpInfo
		        &NewProcess[i]);			// lpProcessInformation

		if (!status) CheckForError(status);
		//AttachThreadInput(NewProcess[i].dwThreadId,GetCurrentThreadId(),TRUE);
	}
	
	for(i = 0; i< N_PROCESSOS; i++)
		ghProcessos[i] = NewProcess[i].hProcess;

}

void fechaHandlesP5(void)
{
	int i;

	CloseHandle(ghExitEvent);

	/* P1 */
	CloseHandle(ghLeituraMESSem);
	for(i=0;i<N_LINHAS;i++) CloseHandle(ghLeituraCLPSem[i]);
	
	/* P2 */
	CloseHandle(ghCapMsgSem);

	/* P3 */
	CloseHandle(ghExibeStatusSem);

	/* P4 */
	CloseHandle(ghExibeOpSem);

	/* Threads HANDLES */
	CloseHandle(ghThreadTeclado);

	/* Process HANDLES */
	for(i=0;i<N_PROCESSOS;i++) CloseHandle(ghProcessos[i]);

}

void switch_device_sem(HANDLE sem, const char* device_str, BOOL* on, const DWORD count)
{
	DWORD i;
	/*	Adquire Handle da janela do console */
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	if(*on == TRUE)
	{
		/*	Altera cor de texto do console para vermelho */
		SetConsoleTextAttribute(hConsole, 12);
		printf("Desligando o Módulo %s ... ", device_str);
		/*	Espera semáforo count vezes com timeout. A geração de uma saída por timeout
		*	indica que há algum problema no módulo que se tentou desligar	*/
		for( i=0; i < count ; i++)
			if(WaitForSingleObject(sem,ON_OFF_TIMEOUT_MS) == WAIT_TIMEOUT)
				printf("Erro no desligamento do dispositivo %s\n",device_str);
		*on = FALSE;
		printf("Modulo %s desligado !\n", device_str);
		
	}
	else
	{
		/*	Altera cor de texto do console para verde */
		SetConsoleTextAttribute(hConsole, 10);
		printf("Ligando o Módulo %s ... ", device_str);
		/*	Sinaliza semáforo count vezes*/
		for( i=0; i < count ; i++)
			if( ReleaseSemaphore(sem,1,NULL) == 0)
				printf("Dispositivo %s com numero excedido de sinalizações de semaforo (%d)\n",device_str,count);
		*on = TRUE;
		printf("Modulo %s ligado !\n", device_str);
	}
	/*	Restaura Cor padrão de texto da janela do console */
	SetConsoleTextAttribute(hConsole, 7);
}

void exibeOPMaisAntigaDisco()
{
	/*	Adquire handle da janela do console */
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	/*	Altera cor do texto da janela de console para azul */
	SetConsoleTextAttribute(hConsole,9);

	/*	Abre arquivo que contém lista de Ordens de Produção salvas em disco */
	HANDLE hOPDiskList = CreateFile(OP_DISK_LIST,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_HIDDEN,
		NULL);
	if(hOPDiskList == INVALID_HANDLE_VALUE)
	{
		printf("Lista de OP's nao pode ser aberta. Erro %d\n",GetLastError());
		/*	Restaura Cor padrão de texto da janela do console */
		SetConsoleTextAttribute(hConsole,7);
		return;
	}

	DWORD idx = 0;
	BOOL lista_completa = FALSE;
	DWORD bytes_lidos = 0;
	char buffer[256];

	/*  Inicia processo de leitura do arquivo sob condição de exclusão mútua */
	LockFile(hOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);

	/*	Posiciona ponteiro no início do campo de dados de controle da lista em disco */
	SetFilePointer(hOPDiskList,-(LONG) OVERHEAD_CONTROLE,0,FILE_END);

	/*	Adquire do campo de controle da lista em disco o índice da próxima entrada da lista */
	ReadFile(hOPDiskList,&idx,sizeof(DWORD),&bytes_lidos,NULL);

	/*	Adquire do campo de controle da lista em disco a indicação se lista já foi totalmente preenchida 
	*	(todas as entradas já contiveram dados pelo menos uma vez) */
	ReadFile(hOPDiskList,&lista_completa,sizeof(BOOL),&bytes_lidos,NULL);

	/*	Verifica se lista é nova (recém-criada e ainda está vazia) */
	if( (!lista_completa) && (idx == 0) )
	{
		printf("Lista de OP's em disco esta vazia!\n"); 
		UnlockFile(hOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);
		SetConsoleTextAttribute(hConsole,7);
		return;
	}

	/*	Se lista nunca foi inteiramente preenchida então entrada mais antiga é a primeira;
	*	Caso contrário, entrada mais antiga é aquela referida pelo índice adquirido no campo de controle */
	if(!lista_completa)
	{
		SetFilePointer(hOPDiskList,0,0,FILE_BEGIN);
	}
	else
	{
		SetFilePointer(hOPDiskList,TAMANHO_MSG_ORDEM_PRODUCAO*idx,0,FILE_BEGIN);
	}

	/*	Adquire OP da lista na posição idx extraída do campo de controle */
	ReadFile(hOPDiskList,buffer,TAMANHO_MSG_ORDEM_PRODUCAO,&bytes_lidos,NULL);

	UnlockFile(hOPDiskList,0,0, ( (TAMANHO_LISTA_OP*TAMANHO_MSG_ORDEM_PRODUCAO) + OVERHEAD_CONTROLE),0);

	buffer[bytes_lidos] = '\0';

	if(!lista_completa)
		printf("OP Mais Antiga Salva em Disco -> Entrada %d\n\t", 1);
	else
		printf("OP Mais Antiga Salva em Disco -> Entrada %d\n\t", idx+1);

	/*	Restaura Cor padrão de texto da janela do console */
	SetConsoleTextAttribute(hConsole,7);

	/*	Exibe OP mais antiga com formatação na janela do console */
	exibeMensagemOP(buffer);

	CloseHandle(hOPDiskList);
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

void exibe_comandos()
{
	int i;
	
	puts("");
	for(i=0;i<70;i++)
		printf("-");
	printf("\n");
	printf("Lista de Comandos\n\n");
	printf("%-35s%-35s\n", "1 -> Liga/Desliga CLP 1", "2 -> Liga/Desliga CLP 2");
	printf("%-35s\n\n%-35s\n", "m -> Liga/Desliga MES", 
		"r -> Liga/Desliga Módulo de Captura de Mensagens");
	printf("%-35s\n%-35s\n", "s -> Liga/Desliga Módulo de Exibição de Status de Bateladas", 
		"o -> Liga/Desliga Módulo de Exibição de Ordens de Produção");
	printf("%-35s\n%-35s\n", "d -> Exibe OP mais antiga em disco", 
		"c -> Exibe menu de comandos");
	printf("\n%-35s\n","Esc -> Encerra programa" );
	for(i=0;i<70;i++)
		printf("-");
	puts("");

}