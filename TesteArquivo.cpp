//
//	Programação Multithreaded em ambiente Windows NT© - uma visão de  Automação
// 
//	Autores: Constantino Seixas Filho/ Marcelo Szuster
//
//	Capítulo 6 - Exemplo 5	- Memória Compartilhada
//
//	Versão: 1.2	25/12/1999
//


#define WNT_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <process.h>	// _beginthreadex() e _endthreadex()
#include <conio.h>		// _getch
#include <iostream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <ctime>

#define MSG_SIZE	20

#define	_CHECKERROR			1	// Ativa função CheckForError
#include "CheckForError.h"

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

DWORD WINAPI MESFunc(LPVOID);	// declaração da função do MES

/*--------DEFINES GERAIS---------*/
#define SP			0x20	//Tecla Espaço
#define RAM_LIST_SIZE 200	//Lista encadeada da RAM
#define SETUP_MSG 35		//Tamanho da mensagem de setup
#define N_THREAD_MES 20		//Numero de threads MES
#define NUM_THREADS 1

//Definindo as teclas
#define	ESC		0x1B
#define keyM	0x4d
#define keyC	0x63
#define keyR	0x72
#define keyP	0x70
#define keyL	0x6C

/*--------DEFINES DE HANDLES---------*/
HANDLE hEventBlockMES;			// Handle para evento que bloqueia o MES
HANDLE hEventExecutionEnd;		// Handle para Evento que sinaliza o término da execução
HANDLE hlivreSemRAM;			// Posição livre na ram
HANDLE hocupadoSemRAM;			// Posição ocupada na ram

HANDLE hEscEvent;				// Handle para Evento Aborta
HANDLE hKeyMEvent, hKeyCEvent, hKeyREvent, hKeyPEvent, hKeyLEvent; //Demais teclas


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
	SYSTEMTIME local;
	GetLocalTime(&local);
	char timestamp[9];
	sprintf_s(timestamp, "%02d:%02d:%02d", local.wHour, local.wMinute, local.wSecond);

	// Construir mensagem
	msg << std::setw(5) << std::setfill('0') << reinterpret_cast<size_t>(id) << "|"  // ID formatado
		<< linha << "|"
		<< std::fixed << std::setprecision(2) << sp_vel << "|"
		<< sp_ench << "|"
		<< sp_sep << "|"
		<< timestamp;

	// Exibir mensagem no console (usar c_str para conversão segura)
	std::cout << "Dado FINAL: " << msg.str() << std::endl;

	return msg.str();
}



int main()
{
	DWORD dwThreadId;
	DWORD dwExitCode = 0;
	DWORD dwRet;
	int nTecla;
	int i;

	/* -------- THREADS ----------*/
	HANDLE hThreadMES;

	/* -------- SEMAFOROS E MUTEXES ----------*/
	hlivreSemRAM = CreateSemaphore(NULL, RAM_LIST_SIZE, RAM_LIST_SIZE, "ListaRAMLivre");
	hocupadoSemRAM = CreateSemaphore(NULL, RAM_LIST_SIZE, RAM_LIST_SIZE, "ListaRAMOcupada");

	/* -------- EVENTOS ----------*/
	// Todas as threads são acordadas a cada pulso
	// apenas uma thread é acordada a cada pulso
	hKeyMEvent = CreateEvent(NULL, FALSE, FALSE, "Evento tecla M");
	CheckForError(hKeyMEvent);
	hKeyCEvent = CreateEvent(NULL, FALSE, FALSE, "Evento tecla C");
	CheckForError(hKeyCEvent);
	hEscEvent = CreateEvent(NULL, TRUE, FALSE, "EscEvento");
	CheckForError(hEscEvent);

	/* ---------- CRIANDO THREADS -------------*/

	//for (i = 0; i < N_THREAD_MES; ++i) {	// cria threads
	//	hThreadsMES[i] = (HANDLE)_beginthreadex(
	//		NULL,
	//		0,
	//		(CAST_FUNCTION)MESFunc,	// casting necessário
	//		(LPVOID)i,
	//		0,
	//		(CAST_LPDWORD)&dwThreadId	// cating necessário
	//	);
	//	if (hThreadsMES[i]) printf("Thread %d criada Id= %0x \n", i, dwThreadId);
	//}  // for

	hThreadMES = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)MESFunc,	// casting necessário
		(LPVOID)0,
		0,
		(CAST_LPDWORD)&dwThreadId	// cating necessário
	);
	if (hThreadMES) printf("Thread MES criada Id= %0x \n", dwThreadId);

	/* ---------- CRIANDO EVENTOS -------------*/

	do {
		printf("Tecle <M> para gerar evento ou <Esc> para terminar\n");
		nTecla = _getch();
		if (nTecla == keyM) PulseEvent(hKeyMEvent); // Gera 1 evento
		else if (nTecla == ESC) PulseEvent(hEscEvent); // Termina threads
	} while (nTecla != ESC);

	/* ---------- ENCERRANDO THREADS -------------*/

	//// Espera todas as threads terminarem
	//dwRet = WaitForMultipleObjects(N_THREAD_MES, hThreadsMES, TRUE, INFINITE);
	//CheckForError(dwRet == WAIT_OBJECT_0);
	//
	//for (i = 0; i < N_THREAD_MES; ++i) {
	//	GetExitCodeThread(hThreadsMES[i], &dwExitCode);
	//	CloseHandle(hThreadsMES[i]);	// apaga referência ao objeto
	//}  // for

	HANDLE hThreads[NUM_THREADS] = { hThreadMES };

	dwRet = WaitForMultipleObjects(NUM_THREADS, hThreads, TRUE, INFINITE);
	CheckForError(dwRet == WAIT_OBJECT_0);

	for (i = 0; i < NUM_THREADS; ++i) {
		GetExitCodeThread(hThreads[i], &dwExitCode);
		CloseHandle(hThreads[i]);	// apaga referência ao objeto
	}  // for


	/* -------- FECHAR HANDLES ----------*/
	CloseHandle(hlivreSemRAM);
	CloseHandle(hocupadoSemRAM);

	CloseHandle(hKeyMEvent);
	CloseHandle(hKeyCEvent);
	CloseHandle(hEscEvent);

	printf("\nAcione uma tecla para terminar\n");
	_getch();

	return EXIT_SUCCESS;
}	// main

/*------------------------------------------- FUNÇÕES THREADS --------------------------------------------------*/

DWORD WINAPI MESFunc(LPVOID id)
{
	std::string dadoMES;
	LONG lOldValue;

	HANDLE Events[2] = {hKeyCEvent, hEscEvent};
	DWORD ret;
	int nTipoEvento;

	do{

		Sleep(100);
		printf("Thread %d esperando evento\n", id);
		ret = WaitForMultipleObjects(2, Events, FALSE, INFINITE);
		nTipoEvento = ret - WAIT_OBJECT_0;

		if (nTipoEvento == 0) {
			printf("Evento %d \n", id);

			// for (int i = 0; i < N_THREAD_MES; ++i) {  // Loop MES

				//printf("Dado recebido da thread MES %d ... \n", id);
				//Sleep(500);

				printf("Dado recebido do MES %d ... \n", id);

				WaitForSingleObject(hlivreSemRAM, INFINITE);

				dadoMES = gerarDadoMES(id);
				printf("Dado recebido do MES %s ... \n", dadoMES);

				listaCircular[posicaoLivre] = dadoMES;
				printf("Dado armazenado na posição %d\n", posicaoLivre);

				posicaoLivre = (posicaoLivre + 1) % 5;
				ReleaseSemaphore(hlivreSemRAM, 1, &lOldValue);

				printf("Dado do MES %d depositado... \n", id);

				Sleep(1000 * (rand() % 10 + 1));	// dorme por um tempo
			//} // for 
		}

	} while (nTipoEvento == 0);	// Esc foi escolhido

	printf("Thread %d terminando...\n", id);
	_endthreadex(0);
	return(0);
} // BoxFunc

