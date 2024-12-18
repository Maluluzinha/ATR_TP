//
//	Programação Multithreaded em ambiente Windows NT© - uma visão de  Automação
// 
//	Autores: 
//
//	Trabalho Prático
//
//	Versão: ?
//


#define WNT_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>	// _beginthreadex() e _endthreadex()
#include <conio.h>		// _getch
#include <iostream>
#include <sstream>

#define MSG_SIZE	20

#define	_CHECKERROR			1	// Ativa função CheckForError
#include "CheckForError.h"

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

DWORD WINAPI MESFunc(LPVOID);	// declaração da função do MES

/*--------DEFINES GERAIS---------*/
#define SP			0x20	//Tecla Espaço
#define	ESC			0x1B	//Tecla Esc
#define RAM_LIST_SIZE 200	//Lista encadeada da RAM
#define SETUP_MSG 35		//Tamanho da mensagem de setup
#define N_THREAD_MES 20		//Numero de threads MES

/*--------DEFINES DE HANDLES---------*/
HANDLE hEventBlockMES;			// Handle para evento que bloqueia o MES
HANDLE hEventExecutionEnd;		// Handle para Evento que sinaliza o término da execução
//HANDLE hEscEvent;				// Handle para Evento Aborta
HANDLE hlivreSemRAM;			// Posição livre na ram
HANDLE hocupadoSemRAM;			// Posição ocupada na ram


/*--------- VAR GLOBAIS ------------*/
std::string listaCircular[RAM_LIST_SIZE];
std::string dadoMES;
int posicaoLivre;
int posicaoOcupado;

/*------------------ FUNÇÃO GERA DADO DO MES ------------------*/
std::string gerarDadoMES(LPVOID id) {

	std::ostringstream msg;
	int linha = rand() % 2 + 1; // Número da linha (1 ou 2)
	double sp_vel = (rand() % 5000) / 100.0;   // Velocidade (cm/s)
	double sp_ench = (rand() % 10000) / 10.0;  // Enchimento (m3/min)
	double sp_sep = (rand() % 1000) / 10.0;    // Separação (cm/s)

	// Obter timestamp
	time_t now = time(nullptr);
	tm localTime;
	localtime_s(&localTime, &now);
	char timestamp[9];
	strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &localTime);

	// time stamp
	/*SYSTEMTIME local;
	GetLocalTime(&local);
	int hora = local.wHour;
	int min = local.wMinute;
	int sec = local.wSecond;
	sprintf(aux, "%02d:%02d:%02d", hora, min, sec);*/

	// Construir mensagem
	/*msg << std::setw(5) << std::setfill('0') << nseq << "|"
		<< std::setw(2) << linha << "|"
		<< std::fixed << std::setprecision(2) << sp_vel << "|"
		<< std::fixed << std::setprecision(2) << sp_ench << "|"
		<< std::fixed << std::setprecision(2) << sp_sep << "|"
		<< timestamp;*/

	return msg.str();
}

int Nseq_dados_otm = 1;
std::string GeraMensagemDadosOTM() {
	srand(time(NULL));
	std::string mensagem;
	char aux[20];
	float dado;
	if (Nseq_dados_otm > 999999) {
		Nseq_dados_otm = 1;
	}
	sprintf(aux, "%06d|11|", Nseq_dados_otm);
	mensagem = aux;
	//set point injeção de gas
	dado = rand() % 99999;
	dado /= 10;
	sprintf(aux, "%06.1f|", dado);
	mensagem = mensagem + aux;
	//set point da temperatura
	dado = rand() % 99999;
	dado /= 10;
	sprintf(aux, "%06.1f|", dado);
	mensagem = mensagem + aux;
	//volume do gas
	int dadoint = rand() % 99999;
	sprintf(aux, "%05d|", dadoint);
	mensagem = mensagem + aux;
	// time stamp
	SYSTEMTIME local;
	GetLocalTime(&local);
	int hora = local.wHour;
	int min = local.wMinute;
	int sec = local.wSecond;
	sprintf(aux, "%02d:%02d:%02d", hora, min, sec);
	mensagem = mensagem + aux;
	Nseq_dados_otm++;
	return mensagem;
}

/*------------------ FUNÇÃO GERA DADO DO CLP ------------------*/


/*------------------ MAIN ------------------*/
	int main()	
	{

	HANDLE hThreadsMES[N_THREAD_MES];
	DWORD dwThreadId;
	DWORD dwExitCode = 0;
	DWORD dwRet;
	int i, nTecla;

	/* -------- SEMAFOROS E MUTEXES ----------*/
	hlivreSemRAM = CreateSemaphore(NULL, RAM_LIST_SIZE, RAM_LIST_SIZE, "ListaRAMLivre");
	hocupadoSemRAM = CreateSemaphore(NULL, RAM_LIST_SIZE, RAM_LIST_SIZE, "ListaRAMOcupada");

	/* ---------- CRIANDO THREADS -------------*/

	for (i = 0; i < N_THREAD_MES; ++i) {	// cria threads
		hThreadsMES[i] = (HANDLE)_beginthreadex(
			NULL,
			0,
			(CAST_FUNCTION)MESFunc,	// casting necessário
			(LPVOID)i,
			0,
			(CAST_LPDWORD)&dwThreadId	// cating necessário
		);
		if (hThreadsMES[i]) printf("Thread %d criada Id= %0x \n", i, dwThreadId);
	}  // for

	/* ---------- ENCERRANDO THREADS -------------*/

	// Espera todas as threads terminarem
	dwRet = WaitForMultipleObjects(N_THREAD_MES, hThreadsMES, TRUE, INFINITE);
	CheckForError(dwRet == WAIT_OBJECT_0);

	for (i = 0; i < N_THREAD_MES; ++i) {
		GetExitCodeThread(hThreadsMES[i], &dwExitCode);
		CloseHandle(hThreadsMES[i]);	// apaga referência ao objeto
	}  // for

	/* -------- FECHAR HANDLES ----------*/
	CloseHandle(hlivreSemRAM);
	CloseHandle(hocupadoSemRAM);


	return EXIT_SUCCESS;
}

DWORD WINAPI tarefaMES(LPVOID id){
	
	std::string dadoMES;
	LONG lOldValue;

	while (true) {

	dadoMES = gerarDadoMES(id); //Gera dado
	printf("Dado recebido do MES %d ... \n", id);

	for (int i = 0; i < N_THREAD_MES; ++i) {  // Loop MES
		dadoMES = gerarDadoMES(id);
		Sleep(500);

		printf("Dado recebido do MES %d ... \n", id);
		
		WaitForSingleObject(hlivreSemRAM, INFINITE);
		printf("Posicao livre pro dado do MES %d ... \n", id);
		listaCircular[posicaoLivre] = dadoMES;
		posicaoLivre = (posicaoLivre + 1) % 5;
		ReleaseSemaphore(hlivreSemRAM, 1, &lOldValue);
		
		printf("Dado do MES %d depositado... \n", id);

		Sleep(1000 * (rand() % 10));	// dorme por um tempo
	} // for 

	}

	_endthreadex(0);
	return(0);
} // Atividade da MES

